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

			ResourceManager& resourceManager = ResourceManager::GetInstance();

			// 래스터 상태 변경
			resourceManager.SetRasterState(RasterState::BackBuffer);
			// 후처리용 상수 버퍼 업데이트
			m_deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(PSConstBuffers::PostProcessing).Get(), 0, nullptr, &m_postProcessingBuffer, 0, 0);

			// 백 버퍼로 씬 렌더링
			RenderSceneToBackBuffer();
		}
	);
}

void Renderer::RenderTextScreenPosition(const wchar_t* text, XMFLOAT2 position, float depth, const XMVECTOR& color, float scale, const wstring& fontName)
{
	ResourceManager::GetInstance().GetSpriteFont(fontName)->DrawString
	(
		m_spriteBatch,
		text,
		position,
		color,
		0.0f,
		XMFLOAT2(0.0f, 0.0f),
		scale,
		SpriteEffects_None,
		0.0f
	);
}

void Renderer::RenderTextUIPosition(const wchar_t* text, XMFLOAT2 position, float depth, const XMVECTOR& color, float scale, const wstring& fontName)
{
	RenderTextScreenPosition(text, { position.x * static_cast<float>(m_swapChainDesc.Width), position.y * static_cast<float>(m_swapChainDesc.Height) }, depth, color, scale, fontName);
}

void Renderer::RenderImageScreenPosition(com_ptr<ID3D11ShaderResourceView> texture, XMFLOAT2 position, XMFLOAT2 offset, float scale, const XMVECTOR& color, float depth)
{
	m_spriteBatch->Draw(texture.Get(), position, nullptr, color, 0.0f, offset, scale, SpriteEffects_None, depth);
}

void Renderer::RenderImageUIPosition(com_ptr<ID3D11ShaderResourceView> texture, XMFLOAT2 position, XMFLOAT2 offset, float scale, const XMVECTOR& color, float depth)
{
	RenderImageScreenPosition(texture, { position.x * static_cast<float>(m_swapChainDesc.Width), position.y * static_cast<float>(m_swapChainDesc.Height) }, offset, scale, color, depth);
}

void Renderer::EndFrame()
{
	HRESULT hr = S_OK;

	ResourceManager& resourceManager = ResourceManager::GetInstance();

	for (auto& [renderTarget, blendStates] : m_renderPass)
	{
		// 픽셀 셰이더의 셰이더 리소스 뷰 해제
		UnbindShaderResources();

		// 백 버퍼 렌더 타겟으로 설정
		vector<ID3D11RenderTargetView*> rtvs = {};
		for (auto& [texture, rtv] : renderTarget.renderTargets) rtvs.push_back(rtv.Get());
		m_deviceContext->OMSetRenderTargets(static_cast<UINT>(rtvs.size()), rtvs.data(), renderTarget.depthStencil.second.Get());

		const RenderStage RENDER_STAGE = static_cast<RenderStage>(&renderTarget - &m_renderPass[0].first);

		// 렌더 타겟 클리어 // 백 버퍼는 클리어 안함
		if (RENDER_STAGE != RenderStage::BackBuffer) ClearRenderTarget(renderTarget);

		for (auto& blendState : blendStates)
		{
			const BlendState BLEND_STATE = static_cast<BlendState>(&blendState - &blendStates[0]);

			// 블렌드 상태 설정
			resourceManager.SetBlendState(BLEND_STATE);

			// 정렬 // 알파 블렌딩은 뒤에서 앞으로 // 그 외는 앞에서 뒤로
			if (BLEND_STATE != BlendState::AlphaBlend) sort(blendState.begin(), blendState.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
			else sort(blendState.begin(), blendState.end(), [](const auto& a, const auto& b) { return a.first > b.first; });

			// 렌더 명령어 실행
			for (auto& [priority, command] : blendState) command();

			// 렌더 명령어 클리어
			blendState.clear();
		}
	}

	// 2D UI 렌더링
	RenderXTKSpriteBatch();

	#ifdef _DEBUG
	ImGui::Begin("SRV");
	ImGui::Image
	(
		(ImTextureID)m_directionalLightShadowMapSRV.Get(),
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
	hr = m_swapChain->Present(1, 0);
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
	RENDER_TARGET(RenderStage::BackBuffer).renderTargets.clear();

	// 씬 버퍼 리소스 해제
	RENDER_TARGET(RenderStage::Scene).renderTargets.clear();
	RENDER_TARGET(RenderStage::Scene).depthStencil = {};

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
	m_spriteBatch = ResourceManager::GetInstance().GetSpriteBatch();
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

	com_ptr<ID3D11Texture2D> backBufferTexture = nullptr;
	hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBufferTexture));
	CheckResult(hr, "스왑 체인 버퍼 얻기 실패.");

	// 렌더 타겟 뷰 생성
	const D3D11_RENDER_TARGET_VIEW_DESC rtvDesc =
	{
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, // 감마 보정
		.ViewDimension = m_swapChainDesc.SampleDesc.Count > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D
	};
	com_ptr<ID3D11RenderTargetView> backBufferRTV = nullptr;
	hr = m_device->CreateRenderTargetView(backBufferTexture.Get(), &rtvDesc, backBufferRTV.GetAddressOf());
	CheckResult(hr, "렌더 타겟 뷰 생성 실패.");

	RENDER_TARGET(RenderStage::BackBuffer).renderTargets.emplace_back(backBufferTexture, backBufferRTV);
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
	D3D11_TEXTURE2D_DESC textureDesc =
	{
		.Width = m_swapChainDesc.Width,
		.Height = m_swapChainDesc.Height,
		.MipLevels = 1, // 단일 밉맵
		.ArraySize = 1, // 단일 텍스처
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.SampleDesc = { 4, 0 }, // 4x MSAA 샘플링
		.Usage = D3D11_USAGE_DEFAULT, // GPU 읽기/쓰기
		.BindFlags = D3D11_BIND_RENDER_TARGET, // 렌더 타겟 및 셰이더 리소스
		.CPUAccessFlags = 0, // CPU 접근 없음
		.MiscFlags = 0 // 기타 플래그 없음
	};
	com_ptr<ID3D11Texture2D> sceneRenderTargetTexture = nullptr;
	hr = m_device->CreateTexture2D(&textureDesc, nullptr, sceneRenderTargetTexture.GetAddressOf());
	CheckResult(hr, "씬 렌더 타겟 텍스처 생성 실패.");

	com_ptr<ID3D11Texture2D> sceneThresholdTexture = nullptr; // 임계값 텍스처 생성
	hr = m_device->CreateTexture2D(&textureDesc, nullptr, sceneThresholdTexture.GetAddressOf());
	CheckResult(hr, "씬 임계값 텍스처 생성 실패.");

	// 렌더 타겟 뷰 생성
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc =
	{
		.Format = textureDesc.Format,
		.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS
	};
	com_ptr<ID3D11RenderTargetView> sceneRenderTargetView = nullptr;
	hr = m_device->CreateRenderTargetView(sceneRenderTargetTexture.Get(), &rtvDesc, sceneRenderTargetView.GetAddressOf());
	CheckResult(hr, "씬 렌더 타겟 뷰 생성 실패.");

	com_ptr<ID3D11RenderTargetView> sceneThresholdView = nullptr;
	hr = m_device->CreateRenderTargetView(sceneThresholdTexture.Get(), &rtvDesc, sceneThresholdView.GetAddressOf());
	CheckResult(hr, "씬 임계값 뷰 생성 실패.");

	RENDER_TARGET(RenderStage::Scene).renderTargets.emplace_back(sceneRenderTargetTexture, sceneRenderTargetView); // 씬 렌더 타겟 등록
	RENDER_TARGET(RenderStage::Scene).renderTargets.emplace_back(sceneThresholdTexture, sceneThresholdView); // 씬 임계값 렌더 타겟 등록

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
	com_ptr<ID3D11Texture2D> depthStencilTexture = nullptr;
	hr = m_device->CreateTexture2D(&depthStencilDesc, nullptr, depthStencilTexture.GetAddressOf());
	CheckResult(hr, "씬 깊이-스텐실 텍스처 생성 실패.");

	// 깊이-스텐실 뷰 생성
	const D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc =
	{
		.Format = depthStencilDesc.Format,
		.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS,
		.Flags = 0
	};
	com_ptr<ID3D11DepthStencilView> depthStencilView = nullptr;
	hr = m_device->CreateDepthStencilView(depthStencilTexture.Get(), &dsvDesc, depthStencilView.GetAddressOf());
	CheckResult(hr, "씬 깊이-스텐실 뷰 생성 실패.");

	RENDER_TARGET(RenderStage::Scene).depthStencil = { depthStencilTexture, depthStencilView };

	const D3D11_TEXTURE2D_DESC sceneResultDesc =
	{
		.Width = m_swapChainDesc.Width,
		.Height = m_swapChainDesc.Height,
		.MipLevels = m_sceneResultMipLevels,
		.ArraySize = static_cast<UINT>(RENDER_TARGET(RenderStage::Scene).renderTargets.size()),
		.Format = textureDesc.Format,
		.SampleDesc = { 1, 0 }, // 결과 텍스처는 단일 샘플링
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, // 셰이더 리소스 용도
		.CPUAccessFlags = 0,
		.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS
	};
	hr = m_device->CreateTexture2D(&sceneResultDesc, nullptr, m_sceneResultTexture.GetAddressOf());
	CheckResult(hr, "씬 결과 텍스처 생성 실패.");

	// 씬 렌더 타겟의 셰이더 리소스 뷰 생성
	const D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc =
	{
		.Format = textureDesc.Format,
		.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY,
		.Texture2DArray = {.MostDetailedMip = 0, .MipLevels = static_cast<UINT>(-1), .FirstArraySlice = 0, .ArraySize = sceneResultDesc.ArraySize }
	};
	hr = m_device->CreateShaderResourceView(m_sceneResultTexture.Get(), &srvDesc, m_sceneShaderResourceView.GetAddressOf());
	CheckResult(hr, "씬 셰이더 리소스 뷰 생성 실패.");
}

void Renderer::CreateShadowMapRenderTargets()
{
	HRESULT hr = S_OK;

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
	com_ptr<ID3D11Texture2D> directionalLightShadowMapTexture = nullptr;
	hr = m_device->CreateTexture2D(&shadowMapDesc, nullptr, directionalLightShadowMapTexture.GetAddressOf());
	CheckResult(hr, "방향성 광원 그림자 맵 텍스처 생성 실패.");

	// 방향성 광원 그림자 맵 깊이-스텐실 뷰 생성
	const D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc =
	{
		.Format = DXGI_FORMAT_D32_FLOAT,
		.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
		.Flags = 0,
		.Texture2D = { 0 }
	};
	com_ptr<ID3D11DepthStencilView> directionalLightShadowMapDSV = nullptr;
	hr = m_device->CreateDepthStencilView(directionalLightShadowMapTexture.Get(), &dsvDesc, directionalLightShadowMapDSV.GetAddressOf());
	CheckResult(hr, "방향성 광원 그림자 맵 깊이-스텐실 뷰 생성 실패.");

	RENDER_TARGET(RenderStage::DirectionalLightShadow).depthStencil = { directionalLightShadowMapTexture, directionalLightShadowMapDSV };

	// 방향성 광원 그림자 맵 셰이더 리소스 뷰 생성
	const D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc =
	{
		.Format = DXGI_FORMAT_R32_FLOAT,
		.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
		.Texture2D = {.MostDetailedMip = 0, .MipLevels = 1 }
	};
	hr = m_device->CreateShaderResourceView(directionalLightShadowMapTexture.Get(), &srvDesc, m_directionalLightShadowMapSRV.GetAddressOf());
	CheckResult(hr, "방향성 광원 그림자 맵 셰이더 리소스 뷰 생성 실패.");
}

void Renderer::BeginImGuiFrame()
{
	ImGui_ImplDX11_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
	ImGuizmo::BeginFrame();
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

	for (size_t i = 0; i < target.renderTargets.size(); ++i) if (target.renderTargets[i].second) m_deviceContext->ClearRenderTargetView(target.renderTargets[i].second.Get(), clearColor.data());
	if (target.depthStencil.second) m_deviceContext->ClearDepthStencilView(target.depthStencil.second.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void Renderer::ResolveSceneMSAA()
{
	for (UINT slice = 0; slice < static_cast<UINT>(RENDER_TARGET(RenderStage::Scene).renderTargets.size()); ++slice)
	{
		m_deviceContext->ResolveSubresource
		(
			m_sceneResultTexture.Get(),
			D3D11CalcSubresource(0, slice, m_sceneResultMipLevels),
			RENDER_TARGET(RenderStage::Scene).renderTargets[slice].first.Get(),
			0,
			m_swapChainDesc.Format
		);
	}
	m_deviceContext->GenerateMips(m_sceneShaderResourceView.Get());
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

void Renderer::RenderXTKSpriteBatch()
{
	// 랜더 타겟과 깊이-스텐실 뷰 저장
	array<ID3D11RenderTargetView*, 1> savedRTV = { nullptr };
	ID3D11DepthStencilView* savedDSV = nullptr;
	m_deviceContext->OMGetRenderTargets(1, savedRTV.data(), &savedDSV);

	// 뷰포트 저장
	array<D3D11_VIEWPORT, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE> savedVP = {};
	UINT vpCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
	m_deviceContext->RSGetViewports(&vpCount, savedVP.data());

	// 랜스터, 블렌드, 깊이-스텐실 상태 저장
	ID3D11RasterizerState* savedRS = nullptr;
	ID3D11BlendState* savedBS = nullptr;
	array<FLOAT, 4> savedBlendFactor = { 0.0f, 0.0f, 0.0f, 0.0f };
	UINT savedSampleMask = 0;
	ID3D11DepthStencilState* savedDSS = nullptr;
	UINT savedStencilRef = 0;
	m_deviceContext->RSGetState(&savedRS);
	m_deviceContext->OMGetBlendState(&savedBS, savedBlendFactor.data(), &savedSampleMask);
	m_deviceContext->OMGetDepthStencilState(&savedDSS, &savedStencilRef);

	// Input assembler state 저장
	ID3D11InputLayout* savedIL = nullptr;
	array<ID3D11Buffer*, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> savedVB = { nullptr };
	array<UINT, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> savedVBStrides = { 0 };
	array<UINT, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> savedVBOffsets = { 0 };
	ID3D11Buffer* savedIB = nullptr;
	DXGI_FORMAT savedIBFormat = DXGI_FORMAT_UNKNOWN;
	UINT savedIBOffset = 0;
	m_deviceContext->IAGetInputLayout(&savedIL);
	m_deviceContext->IAGetVertexBuffers(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, savedVB.data(), savedVBStrides.data(), savedVBOffsets.data());
	m_deviceContext->IAGetIndexBuffer(&savedIB, &savedIBFormat, &savedIBOffset);
	D3D11_PRIMITIVE_TOPOLOGY savedTopo = {};
	m_deviceContext->IAGetPrimitiveTopology(&savedTopo);

	// VS/PS shader 및 클래스 인스턴스 저장
	ID3D11VertexShader* savedVS = nullptr;
	ID3D11PixelShader* savedPS = nullptr;
	array<ID3D11ClassInstance*, 1> savedVSCls = { nullptr };
	array<ID3D11ClassInstance*, 1> savedPSCls = { nullptr };
	UINT savedVSClsCount = 0, savedPSClsCount = 0;
	m_deviceContext->VSGetShader(&savedVS, savedVSCls.data(), &savedVSClsCount);
	m_deviceContext->PSGetShader(&savedPS, savedPSCls.data(), &savedPSClsCount);

	// 픽셀 셰이더 리소스 뷰 및 샘플러 상태 저장
	const UINT srvCount = static_cast<UINT>(TextureSlots::Count);
	std::vector<ID3D11ShaderResourceView*> savedPSSRVs(srvCount, nullptr);
	m_deviceContext->PSGetShaderResources(0, srvCount, savedPSSRVs.data());

	std::vector<ID3D11SamplerState*> savedPSSamplers(srvCount, nullptr);
	m_deviceContext->PSGetSamplers(0, srvCount, savedPSSamplers.data());

	// 콘스탄트 버퍼 저장
	const UINT cbSaveCount = 8;
	std::vector<ID3D11Buffer*> savedPSCB(cbSaveCount, nullptr);
	std::vector<ID3D11Buffer*> savedVSCB(cbSaveCount, nullptr);
	m_deviceContext->PSGetConstantBuffers(0, cbSaveCount, savedPSCB.data());
	m_deviceContext->VSGetConstantBuffers(0, cbSaveCount, savedVSCB.data());

	// XTK SpriteBatch 렌더링
	m_spriteBatch->Begin(SpriteSortMode_Deferred, nullptr, nullptr, nullptr, nullptr, nullptr, XMMatrixIdentity());
	for (function<void()>& uiRenderFunction : m_UIRenderFunctions) uiRenderFunction();
	m_UIRenderFunctions.clear();
	m_spriteBatch->End();

	// 이전 상태 복원
	m_deviceContext->OMSetRenderTargets(1, savedRTV.data(), savedDSV);
	m_deviceContext->RSSetViewports(vpCount, savedVP.data());
	m_deviceContext->RSSetState(savedRS);
	m_deviceContext->OMSetBlendState(savedBS, savedBlendFactor.data(), savedSampleMask);
	m_deviceContext->OMSetDepthStencilState(savedDSS, savedStencilRef);

	m_deviceContext->IASetInputLayout(savedIL);
	m_deviceContext->IASetVertexBuffers(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, savedVB.data(), savedVBStrides.data(), savedVBOffsets.data());
	m_deviceContext->IASetIndexBuffer(savedIB, savedIBFormat, savedIBOffset);
	m_deviceContext->IASetPrimitiveTopology(savedTopo);

	m_deviceContext->VSSetShader(savedVS, savedVSClsCount ? savedVSCls.data() : nullptr, savedVSClsCount);
	m_deviceContext->PSSetShader(savedPS, savedPSClsCount ? savedPSCls.data() : nullptr, savedPSClsCount);

	m_deviceContext->PSSetShaderResources(0, srvCount, savedPSSRVs.data());
	m_deviceContext->PSSetSamplers(0, srvCount, savedPSSamplers.data());
	m_deviceContext->PSSetConstantBuffers(0, cbSaveCount, savedPSCB.data());
	m_deviceContext->VSSetConstantBuffers(0, cbSaveCount, savedVSCB.data());

	// 리소스 해제
	for (UINT i = 0; i < srvCount; ++i) if (savedPSSRVs[i]) savedPSSRVs[i]->Release();
	for (UINT i = 0; i < srvCount; ++i) if (savedPSSamplers[i]) savedPSSamplers[i]->Release();
	for (UINT i = 0; i < cbSaveCount; ++i) if (savedPSCB[i]) savedPSCB[i]->Release();
	for (UINT i = 0; i < cbSaveCount; ++i) if (savedVSCB[i]) savedVSCB[i]->Release();
	if (savedRTV[0]) savedRTV[0]->Release();
	if (savedDSV) savedDSV->Release();
	if (savedRS) savedRS->Release();
	if (savedBS) savedBS->Release();
	if (savedDSS) savedDSS->Release();
	if (savedIL) savedIL->Release();
	for (auto& VB : savedVB) if (VB) VB->Release();
	if (savedIB) savedIB->Release();
	if (savedVS) savedVS->Release();
	if (savedPS) savedPS->Release();
	for (UINT i = 0; i < savedVSClsCount; ++i) if (savedVSCls[i]) savedVSCls[i]->Release();
	for (UINT i = 0; i < savedPSClsCount; ++i) if (savedPSCls[i]) savedPSCls[i]->Release();
}

void Renderer::EndImGuiFrame()
{
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
