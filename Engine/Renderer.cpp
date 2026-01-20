#include "stdafx.h"
#include "Renderer.h"

#include "ResourceManager.h"
#include "SceneManager.h"
#include "WindowManager.h"
#include "TimeManager.h"

using namespace std;
using namespace DirectX;

void Renderer::Initialize()
{
	CreateDeviceAndContext();
	CreateSwapChain();
	CreateBackBufferResources();
	CreateShadowMapRenderTargets();

	// 씬 매니저 초기화
	SceneManager::GetInstance().Initialize();
}

void Renderer::BeginFrame()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();
	resourceManager.SetAllConstantBuffers();
	resourceManager.SetAllSamplerStates();

	#ifdef _DEBUG
	// ImGui 프레임 시작
	BeginImGuiFrame();
	#endif

	// 방향성 광원 그림자 맵 셰이더 리소스 뷰 해제 명령어 등록
	RENDER_FUNCTION(RenderStage::Scene, BlendState::Opaque).emplace_back
	(
		numeric_limits<float>::lowest(), // 우선순위 가장 높음
		[&]() { m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::DirectionalLightShadow), 1, m_directionalLightShadowMapSRV.GetAddressOf()); }
	);

	// 백 버퍼 최종 렌더링 명령어 등록
	RENDER_FUNCTION(RenderStage::BackBuffer, BlendState::Opaque).emplace_back
	(
		0.0f,
		[&]()
		{
			// 씬 렌더 타겟 MSAA 해제 및 결과 텍스처 복사
			ResolveSceneMSAA();

			// 래스터 상태 변경
			ResourceManager::GetInstance().SetRasterState(RasterState::BackBuffer);

			// 백 버퍼로 씬 렌더링
			RenderSceneToBackBuffer();
		}
	);


	RENDER_FUNCTION(RenderStage::BackBuffer, BlendState::Opaque).emplace_back
	(
		numeric_limits<float>::max(), // XTK는 이런저런 리소스 바인딩을 해서 무조건 가장 나중에 실행
		[&]()
		{
			static UINT frameCount = 0;
			static float elapsedTime = 0.0f;
			static UINT FPS = 0;

			frameCount++;

			elapsedTime += TimeManager::GetInstance().GetDeltaTime();

			if (elapsedTime >= 1.0)
			{
				FPS = frameCount * static_cast<UINT>(elapsedTime);
				frameCount = 0;
				elapsedTime = 0.0;
			}

			DrawTextToBackBuffer((wstring(L"FPS: ") + to_wstring(FPS)).c_str(), XMFLOAT2{ 10.0f, 10.0f });
		}
	);
}

void Renderer::DrawTextToBackBuffer(const wchar_t* text, XMFLOAT2 position, const XMVECTOR& color, const wstring& fontName)
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();

	auto& [spriteFont, spriteBatch] = resourceManager.GetSpriteFont(fontName);
	if (!spriteFont || !spriteBatch) return;

	spriteBatch->Begin(SpriteSortMode_Deferred, nullptr, nullptr, nullptr, nullptr, nullptr, XMMatrixIdentity());
	spriteFont->DrawString(spriteBatch.get(), text, position, color, 0.0f, XMFLOAT2{ 0.0f, 0.0f }, 1.0f);
	spriteBatch->End();
}

void Renderer::EndFrame()
{
	HRESULT hr = S_OK;

	for (auto& [renderTarget, blendStates] : m_renderPass)
	{
		// 픽셀 셰이더의 셰이더 리소스 뷰 해제
		UnbindShaderResources();

		// 백 버퍼 렌더 타겟으로 설정
		m_deviceContext->OMSetRenderTargets(1, renderTarget.renderTargetView.GetAddressOf(), renderTarget.depthStencilView.Get());

		const RenderStage RENDER_STAGE = static_cast<RenderStage>(&renderTarget - &m_renderPass[0].first);

		// 렌더 타겟 클리어 // 백 버퍼는 클리어 안함
		if (RENDER_STAGE != RenderStage::BackBuffer) ClearRenderTarget(renderTarget);

		for (auto& blendState : blendStates)
		{
			const BlendState BLEND_STATE = static_cast<BlendState>(&blendState - &blendStates[0]);

			// 블렌드 상태 설정
			ResourceManager::GetInstance().SetBlendState(BLEND_STATE);

			// 정렬 // 알파 블렌딩은 뒤에서 앞으로 // 그 외는 앞에서 뒤로
			if (BLEND_STATE != BlendState::AlphaBlend) sort(blendState.begin(), blendState.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
			else sort(blendState.begin(), blendState.end(), [](const auto& a, const auto& b) { return a.first > b.first; });

			// 렌더 명령어 실행
			for (auto& [priority, command] : blendState) command();

			// 렌더 명령어 클리어
			blendState.clear();
		}
	}

	#ifdef _DEBUG
	ImGui::Begin("SRV");
	ImGui::Image
	(
		(ImTextureID)m_sceneShaderResourceView.Get(),
		ImVec2(500.0f, 500.0f)
	);
	ImGui::End();
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	ImGui::UpdatePlatformWindows();
	ImGui::RenderPlatformWindowsDefault();
	EndImGuiFrame();
	#endif

	// 스왑 체인 프레젠트
	hr = m_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	CheckResult(hr, "스왑 체인 프레젠트 실패.");
}

void Renderer::Finalize()
{
	// ImGui DirectX11 종료
	ImGui_ImplDX11_Shutdown();

	// RenderResourceManager 종료는 따로 필요 없음

	SceneManager::GetInstance().Finalize();
}

HRESULT Renderer::Resize(UINT width, UINT height)
{
	if (width <= 0 || height <= 0) return E_INVALIDARG;

	HRESULT hr = S_OK;

	// 렌더 타겟 및 셰이더 리소스 해제
	UnbindShaderResources();
	constexpr ID3D11RenderTargetView* nullRTV = nullptr;
	m_deviceContext->OMSetRenderTargets(1, &nullRTV, nullptr);

	m_deviceContext->Flush();

	// 백 버퍼 리소스 해제
	RENDER_TARGET(RenderStage::BackBuffer).renderTarget.Reset();
	RENDER_TARGET(RenderStage::BackBuffer).renderTargetView.Reset();

	// 씬 버퍼 리소스 해제
	RENDER_TARGET(RenderStage::Scene).renderTarget.Reset();
	RENDER_TARGET(RenderStage::Scene).renderTargetView.Reset();
	RENDER_TARGET(RenderStage::Scene).depthStencilTexture.Reset();
	RENDER_TARGET(RenderStage::Scene).depthStencilView.Reset();

	m_sceneResultTexture.Reset();
	m_sceneShaderResourceView.Reset();

	m_swapChainDesc.Width = width;
	m_swapChainDesc.Height = height;
	m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	// 스왑 체인 크기 조정
	hr = m_swapChain->ResizeBuffers
	(
		m_swapChainDesc.BufferCount,
		m_swapChainDesc.Width,
		m_swapChainDesc.Height,
		m_swapChainDesc.Format,
		m_swapChainDesc.Flags
	);
	CheckResult(hr, "스왑 체인 버퍼 크기 조정 실패.");

	// 새로운 백 버퍼, 씬 렌더 타겟 생성
	CreateBackBufferRenderTarget();
	CreateSceneRenderTarget();

	// 뷰포트 설정
	SetViewport();

	return hr;
}

void Renderer::SetViewport(FLOAT Width, FLOAT Height)
{
	const D3D11_VIEWPORT viewport =
	{
		.TopLeftX = 0.0f,
		.TopLeftY = 0.0f,
		.Width = Width,
		.Height = Height,
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f
	};
	m_deviceContext->RSSetViewports(1, &viewport);
}

void Renderer::CreateDeviceAndContext()
{
	HRESULT hr = S_OK;

	hr = D3D11CreateDevice
	(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		#ifdef _DEBUG
		D3D11_CREATE_DEVICE_DEBUG,
		#else
		0,
		#endif
		m_featureLevels.data(),
		static_cast<UINT>(m_featureLevels.size()),
		D3D11_SDK_VERSION,
		m_device.GetAddressOf(),
		nullptr,
		m_deviceContext.GetAddressOf()
	);
	CheckResult(hr, "디바이스 및 디바이스 컨텍스트 생성 실패.");

	// ImGui DirectX11 초기화
	ImGui_ImplDX11_Init(m_device.Get(), m_deviceContext.Get());
	// RenderResourceManager 초기화
	ResourceManager::GetInstance().Initialize(m_device, m_deviceContext);
}

void Renderer::CreateSwapChain()
{
	HRESULT hr = S_OK;

	com_ptr<IDXGIDevice> dxgiDevice = nullptr;
	com_ptr<IDXGIAdapter> dxgiAdapter = nullptr;
	com_ptr<IDXGIFactory2> dxgiFactory = nullptr;

	hr = m_device.As(&dxgiDevice);
	CheckResult(hr, "ID3D11Device 로부터 IDXGIDevice 얻기 실패.");
	hr = dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());
	CheckResult(hr, "IDXGIDevice 로부터 IDXGIAdapter 얻기 실패.");
	hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
	CheckResult(hr, "IDXGIAdapter 로부터 IDXGIFactory2 얻기 실패.");

	// 스왑 체인 생성
	hr = dxgiFactory->CreateSwapChainForHwnd
	(
		dxgiDevice.Get(),
		WindowManager::GetInstance().GetHWnd(),
		&m_swapChainDesc,
		nullptr,
		nullptr,
		m_swapChain.GetAddressOf()
	);
	CheckResult(hr, "스왑 체인 생성 실패.");
}

void Renderer::CreateBackBufferRenderTarget()
{
	HRESULT hr = S_OK;

	hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&RENDER_TARGET(RenderStage::BackBuffer).renderTarget));
	CheckResult(hr, "스왑 체인 버퍼 얻기 실패.");

	// 렌더 타겟 뷰 생성
	const D3D11_RENDER_TARGET_VIEW_DESC rtvDesc =
	{
		.Format = m_swapChainDesc.Format,
		.ViewDimension = m_swapChainDesc.SampleDesc.Count > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D
	};
	hr = m_device->CreateRenderTargetView(RENDER_TARGET(RenderStage::BackBuffer).renderTarget.Get(), &rtvDesc, RENDER_TARGET(RenderStage::BackBuffer).renderTargetView.GetAddressOf());
	CheckResult(hr, "렌더 타겟 뷰 생성 실패.");
}

void Renderer::CreateBackBufferResources()
{
	HRESULT hr = S_OK;

	constexpr array<BackBufferVertex, 3> backBufferVertices = // 전체 화면 삼각형 정점 데이터
	{
		BackBufferVertex{.position = { -1.0f, -1.0f, 0.0f, 1.0f }, .UV = { 0.0f, 1.0f } },
		BackBufferVertex{.position = { -1.0f, 3.0f, 0.0f, 1.0f }, .UV = { 0.0f, -1.0f } },
		BackBufferVertex{.position = { 3.0f, -1.0f, 0.0f, 1.0f }, .UV = { 2.0f, 1.0f } }
	};
	constexpr D3D11_BUFFER_DESC bufferDesc =
	{
		.ByteWidth = sizeof(backBufferVertices),
		.Usage = D3D11_USAGE_IMMUTABLE,
		.BindFlags = D3D11_BIND_VERTEX_BUFFER,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
		.StructureByteStride = 0
	};
	const D3D11_SUBRESOURCE_DATA initialData =
	{
		.pSysMem = backBufferVertices.data(),
		.SysMemPitch = 0,
		.SysMemSlicePitch = 0
	};

	hr = m_device->CreateBuffer(&bufferDesc, &initialData, m_backBufferVertexBuffer.GetAddressOf());
	CheckResult(hr, "백 버퍼 정점 버퍼 생성 실패.");

	ResourceManager& resourceManager = ResourceManager::GetInstance();
	// 정점 셰이더 및 입력 레이아웃 생성
	vector<InputElement> inputElements = { InputElement::Position, InputElement::UV };
	m_backBufferVertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout("VSPostProcessing.hlsl", inputElements);
	// 픽셀 셰이더 컴파일 및 생성
	m_backBufferPixelShader = resourceManager.GetPixelShader("PSPostProcessing.hlsl");
}

void Renderer::CreateSceneRenderTarget()
{
	HRESULT hr = S_OK;

	// 렌더 타겟 텍스처 생성
	const D3D11_TEXTURE2D_DESC textureDesc =
	{
		.Width = m_swapChainDesc.Width,
		.Height = m_swapChainDesc.Height,
		.MipLevels = 1, // 단일 밉맵
		.ArraySize = 1, // 단일 텍스처
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM, // 감마 보정 안함
		.SampleDesc = m_sceneBufferSampleDesc,
		.Usage = D3D11_USAGE_DEFAULT, // GPU 읽기/쓰기
		.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, // 렌더 타겟 및 셰이더 리소스
		.CPUAccessFlags = 0, // CPU 접근 없음
		.MiscFlags = 0 // 기타 플래그 없음
	};
	hr = m_device->CreateTexture2D(&textureDesc, nullptr, RENDER_TARGET(RenderStage::Scene).renderTarget.GetAddressOf());
	CheckResult(hr, "씬 렌더 타겟 텍스처 생성 실패.");

	// 렌더 타겟 뷰 생성
	const D3D11_RENDER_TARGET_VIEW_DESC rtvDesc =
	{
		.Format = textureDesc.Format,
		.ViewDimension = textureDesc.SampleDesc.Count > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D // 멀티샘플링 여부에 따른 뷰 차원
	};
	hr = m_device->CreateRenderTargetView(RENDER_TARGET(RenderStage::Scene).renderTarget.Get(), &rtvDesc, RENDER_TARGET(RenderStage::Scene).renderTargetView.GetAddressOf());
	CheckResult(hr, "씬 렌더 타겟 뷰 생성 실패.");

	// 깊이-스텐실 텍스처 및 뷰 생성
	const D3D11_TEXTURE2D_DESC depthStencilDesc =
	{
		.Width = m_swapChainDesc.Width,
		.Height = m_swapChainDesc.Height,
		.MipLevels = 1,
		.ArraySize = 1,
		.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT, // 깊이: 32비트 실수, 스텐실: 8비트 정수
		.SampleDesc = textureDesc.SampleDesc,
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_DEPTH_STENCIL,
		.CPUAccessFlags = 0,
		.MiscFlags = 0
	};
	hr = m_device->CreateTexture2D(&depthStencilDesc, nullptr, RENDER_TARGET(RenderStage::Scene).depthStencilTexture.GetAddressOf());
	CheckResult(hr, "씬 깊이-스텐실 텍스처 생성 실패.");

	// 깊이-스텐실 뷰 생성
	const D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc =
	{
		.Format = depthStencilDesc.Format,
		.ViewDimension = depthStencilDesc.SampleDesc.Count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D,
		.Flags = 0
	};
	hr = m_device->CreateDepthStencilView(RENDER_TARGET(RenderStage::Scene).depthStencilTexture.Get(), &dsvDesc, RENDER_TARGET(RenderStage::Scene).depthStencilView.GetAddressOf());
	CheckResult(hr, "씬 깊이-스텐실 뷰 생성 실패.");

	const D3D11_TEXTURE2D_DESC sceneResultDesc =
	{
		.Width = m_swapChainDesc.Width,
		.Height = m_swapChainDesc.Height,
		.MipLevels = 1,
		.ArraySize = 1,
		.Format = textureDesc.Format,
		.SampleDesc = { 1, 0 }, // 결과 텍스처는 단일 샘플링
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_SHADER_RESOURCE, // 셰이더 리소스 용도
		.CPUAccessFlags = 0,
		.MiscFlags = 0
	};
	hr = m_device->CreateTexture2D(&sceneResultDesc, nullptr, m_sceneResultTexture.GetAddressOf());
	CheckResult(hr, "씬 결과 텍스처 생성 실패.");

	// 씬 렌더 타겟의 셰이더 리소스 뷰 생성
	const D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc =
	{
		.Format = textureDesc.Format,
		.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
		.Texture2D = {.MostDetailedMip = 0, .MipLevels = 1 }
	};
	hr = m_device->CreateShaderResourceView(m_sceneResultTexture.Get(), &srvDesc, m_sceneShaderResourceView.GetAddressOf());
	CheckResult(hr, "씬 셰이더 리소스 뷰 생성 실패.");
}

void Renderer::CreateShadowMapRenderTargets()
{
	// 방향성 광원 그림자 맵 텍스처 생성
	const D3D11_TEXTURE2D_DESC shadowMapDesc =
	{
		.Width = DIRECTIAL_LIGHT_SHADOW_MAP_SIZE,
		.Height = DIRECTIAL_LIGHT_SHADOW_MAP_SIZE,
		.MipLevels = 1,
		.ArraySize = 1,
		.Format = DXGI_FORMAT_R32_TYPELESS,
		.SampleDesc = { 1, 0 },
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
		.CPUAccessFlags = 0,
		.MiscFlags = 0
	};
	HRESULT hr = m_device->CreateTexture2D(&shadowMapDesc, nullptr, RENDER_TARGET(RenderStage::DirectionalLightShadow).depthStencilTexture.GetAddressOf());
	CheckResult(hr, "방향성 광원 그림자 맵 텍스처 생성 실패.");

	// 방향성 광원 그림자 맵 깊이-스텐실 뷰 생성
	const D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc =
	{
		.Format = DXGI_FORMAT_D32_FLOAT,
		.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
		.Flags = 0,
		.Texture2D = { 0 }
	};
	hr = m_device->CreateDepthStencilView(RENDER_TARGET(RenderStage::DirectionalLightShadow).depthStencilTexture.Get(), &dsvDesc, RENDER_TARGET(RenderStage::DirectionalLightShadow).depthStencilView.GetAddressOf());

	// 방향성 광원 그림자 맵 셰이더 리소스 뷰 생성
	const D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc =
	{
		.Format = DXGI_FORMAT_R32_FLOAT,
		.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
		.Texture2D = {.MostDetailedMip = 0, .MipLevels = 1 }
	};
	hr = m_device->CreateShaderResourceView(RENDER_TARGET(RenderStage::DirectionalLightShadow).depthStencilTexture.Get(), &srvDesc, m_directionalLightShadowMapSRV.GetAddressOf());
}

void Renderer::BeginImGuiFrame()
{
	ImGui_ImplDX11_NewFrame();
	ImGui::NewFrame();
	ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
}

void Renderer::UnbindShaderResources()
{
	constexpr ID3D11ShaderResourceView* nullSRV = nullptr;

	for (UINT i = 0; i < static_cast<UINT>(TextureSlots::Count); ++i) m_deviceContext->PSSetShaderResources(i, 1, &nullSRV);
}

void Renderer::ClearRenderTarget(RenderTarget& target)
{
	constexpr array<float, 4> clearColor = { 0.0f, 1.0f, 0.0f, 1.0f };

	if (target.renderTargetView) m_deviceContext->ClearRenderTargetView(target.renderTargetView.Get(), clearColor.data());
	if (target.depthStencilView) m_deviceContext->ClearDepthStencilView(target.depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void Renderer::ResolveSceneMSAA()
{
	if (m_sceneBufferSampleDesc.Count > 1) m_deviceContext->ResolveSubresource(m_sceneResultTexture.Get(), 0, RENDER_TARGET(RenderStage::Scene).renderTarget.Get(), 0, m_swapChainDesc.Format);
	else m_deviceContext->CopyResource(m_sceneResultTexture.Get(), RENDER_TARGET(RenderStage::Scene).renderTarget.Get());
}

void Renderer::RenderSceneToBackBuffer()
{
	// 전체 화면 삼각형 렌더링
	constexpr UINT STRIDE = sizeof(BackBufferVertex);
	constexpr UINT OFFSET = 0;

	m_deviceContext->IASetVertexBuffers(0, 1, m_backBufferVertexBuffer.GetAddressOf(), &STRIDE, &OFFSET);
	m_deviceContext->IASetInputLayout(m_backBufferVertexShaderAndInputLayout.second.Get());
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_deviceContext->VSSetShader(m_backBufferVertexShaderAndInputLayout.first.Get(), nullptr, 0);
	m_deviceContext->PSSetShader(m_backBufferPixelShader.Get(), nullptr, 0);

	m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::BackBuffer), 1, m_sceneShaderResourceView.GetAddressOf());

	m_deviceContext->Draw(3, 0);
}

void Renderer::EndImGuiFrame()
{
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}