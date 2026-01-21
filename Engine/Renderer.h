#pragma once
#include "Singleton.h"

class Renderer : public Singleton<Renderer>
{
	friend class Singleton<Renderer>;

	const std::array<D3D_FEATURE_LEVEL, 3> m_featureLevels = // 지원할 Direct3D 버전
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	float m_aspectRatio = 16.0f / 9.0f; // 화면 종횡비
	DXGI_SWAP_CHAIN_DESC1 m_swapChainDesc = // 스왑 체인 설정
	{
		.Width = 1280,
		.Height = 720,
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.Stereo = FALSE, // VR용 입체 영상용 // 사용 안함
		.SampleDesc = {.Count = 1, .Quality = 0 }, // 멀티샘플링 설정 // 스왑 체인 자체에는 멀티샘플링 적용 못함
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT, // 렌더 타겟으로 사용
		.BufferCount = 2, // 더블 버퍼링
		.Scaling = DXGI_SCALING_STRETCH, // 창 크기에 맞게 스트레칭
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD, // 플립 모드 // 잘 모르겠는데 이게 빠르다 함
		.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED, // 알파 블렌딩 없음
		.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING // 전체 화면 전환 허용
	};
	// TODO: 나중에 전체 화면 설정도 멤버 변수로 추가 DXGI_SWAP_CHAIN_FULLSCREEN_DESC

	com_ptr<ID3D11Device> m_device = nullptr; // 디바이스
	com_ptr<ID3D11DeviceContext> m_deviceContext = nullptr; // 디바이스 컨텍스트
	com_ptr<IDXGISwapChain1> m_swapChain = nullptr; // 스왑 체인
	
	DirectX::XMVECTOR m_renderSortPoint = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f); // 랜저 정렬 기준점
	std::array<std::pair<RenderTarget, std::array<std::vector<std::pair<float, std::function<void()>>>, static_cast<size_t>(BlendState::Count)>>, static_cast<size_t>(RenderStage::Count)> m_renderPass = {};
	
	DirectX::SpriteBatch* m_spriteBatch = nullptr; // 스프라이트 배치 // UI 렌더링용
	std::vector<std::function<void()>> m_UIRenderFunctions = {}; // ImGui 렌더링 함수 목록

	// 백 버퍼 렌더 타겟 관련 리소스
	struct BackBufferVertex
	{
		DirectX::XMFLOAT4 position = {};
		DirectX::XMFLOAT2 UV = {};
	};
	com_ptr<ID3D11Buffer> m_backBufferVertexBuffer = nullptr; // 백 버퍼용 정점 버퍼
	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_backBufferVertexShaderAndInputLayout = {}; // 백 버퍼용 정점 셰이더 및 입력 레이아웃
	com_ptr<ID3D11PixelShader> m_backBufferPixelShader = nullptr; // 백 버퍼용 후처리 픽셀 셰이더

	// 씬 렌더 타겟 관련 리소스
	DXGI_SAMPLE_DESC m_sceneBufferSampleDesc = { 4, 0 }; // 씬 렌더 타겟용 샘플 설명 // 멀티샘플링 설정
	com_ptr<ID3D11Texture2D> m_sceneResultTexture = nullptr; // 씬 렌더 타겟의 결과 텍스처 // MSAA 다운샘플링 후 결과 저장
	com_ptr<ID3D11ShaderResourceView> m_sceneShaderResourceView = nullptr; // 씬 렌더 타겟의 셰이더 리소스 뷰 // 백 버퍼에 적용하면서 후처리됨

	// 그림자 맵 렌더 타겟 관련 리소스
	com_ptr<ID3D11ShaderResourceView> m_directionalLightShadowMapSRV = nullptr; // 방향성 광원 그림자 맵 셰이더 리소스 뷰

public:
	Renderer() = default;
	~Renderer() = default;
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;
	Renderer& operator=(Renderer&&) = delete;

	// 렌더러 초기화 // WindowManager에서 윈도우 생성 후 호출
	void Initialize();

	// 프레임 시작
	void BeginFrame();

	// 렌더 패스 관련 매크로 함수
	void SetRenderSortPoint(const DirectX::XMVECTOR& point) { m_renderSortPoint = point; }
	const DirectX::XMVECTOR& GetRenderSortPoint() const { return m_renderSortPoint; }

	constexpr RenderTarget& RENDER_TARGET(RenderStage stage) { return m_renderPass[static_cast<size_t>(stage)].first; }
	constexpr std::vector<std::pair<float, std::function<void()>>>& RENDER_FUNCTION(RenderStage renderStage, BlendState blendState) { return m_renderPass[static_cast<size_t>(renderStage)].second[static_cast<size_t>(blendState)]; }
	
	// UI 렌더링 함수
	constexpr std::vector<std::function<void()>>& UI_RENDER_FUNCTIONS() { return m_UIRenderFunctions; }
	// UI 텍스트 렌더링
	// 스크린 좌표계
	void RenderTextScreenPosition(const wchar_t* text, DirectX::XMFLOAT2 position, float depth = 0.0f, const DirectX::XMVECTOR& color = DirectX::XMVECTOR{ 1.0f, 1.0f, 1.0f, 1.0f }, float scale = 1.0f, const std::wstring& fontName = L"Gugi");
	// UI 좌표계
	void RenderTextUIPosition(const wchar_t* text, DirectX::XMFLOAT2 position, float depth = 0.0f, const DirectX::XMVECTOR& color = DirectX::XMVECTOR{ 1.0f, 1.0f, 1.0f, 1.0f }, float scale = 1.0f, const std::wstring& fontName = L"Gugi");
	// UI 이미지 렌더링
	// 스크린 좌표계
	void RenderImageScreenPosition(com_ptr<ID3D11ShaderResourceView> texture, DirectX::XMFLOAT2 position, DirectX::XMFLOAT2 offset, float scale = 1.0f, DirectX::XMVECTOR color = DirectX::XMVECTOR{ 1.0f, 1.0f, 1.0f, 1.0f }, float depth = 0.0f);
	// UI 좌표계
	void RenderImageUIPosition(com_ptr<ID3D11ShaderResourceView> texture, DirectX::XMFLOAT2 position, DirectX::XMFLOAT2 offset, float scale = 1.0f, DirectX::XMVECTOR color = DirectX::XMVECTOR{ 1.0f, 1.0f, 1.0f, 1.0f }, float depth = 0.0f);

	// 프레임 종료 // 화면에 내용 출력
	void EndFrame();

	// 렌더러 종료 // WindowManager에서 윈도우 종료 전 호출
	void Finalize();

	// 디바이스 컨텍스트 조회
	com_ptr<ID3D11DeviceContext> GetDeviceContext() const { return m_deviceContext; }

	// 화면 크기 조정
	HRESULT Resize(UINT width, UINT height);
	// 뷰포트 설정
	void SetViewport(FLOAT Width, FLOAT Height);
	// 뷰포트 설정 (현재 스왑 체인 크기로)
	void SetViewport() { SetViewport(static_cast<FLOAT>(m_swapChainDesc.Width), static_cast<FLOAT>(m_swapChainDesc.Height)); }
	// 화면 종횡비 조회
	float GetAspectRatio() const { return m_aspectRatio; }

	// 스왑 체인 설정 변경
	void SetSwapChainDesc(const DXGI_SWAP_CHAIN_DESC1& desc) { m_swapChainDesc = desc; }
	// 스왑 체인 설정 조회
	const DXGI_SWAP_CHAIN_DESC1& GetSwapChainDesc() const { return m_swapChainDesc; }

private:
	// 초기화 함수
	// 디바이스 및 디바이스 컨텍스트 생성
	void CreateDeviceAndContext();
	// 스왑 체인 생성
	void CreateSwapChain();
	// 백 버퍼 렌더 타겟 생성
	void CreateBackBufferRenderTarget();
	// 백 버퍼 셰이더 및 상수 버퍼 생성
	void CreateBackBufferResources();
	// 씬 렌더 타겟 생성
	void CreateSceneRenderTarget();
	// 그림자 맵 렌더 타겟 생성
	void CreateShadowMapRenderTargets();

	// 랜더링 파이프라인 함수
	// ImGui 프레임 시작
	void BeginImGuiFrame();
	// 셰이더 리소스 해제
	void UnbindShaderResources();
	// 렌더 타겟 클리어
	void ClearRenderTarget(RenderTarget& target);
	// 씬 렌더 타겟 MSAA 다운샘플링 // MSAA 미적용시 그냥 복사
	void ResolveSceneMSAA();
	// 백 버퍼 랜더링
	void RenderSceneToBackBuffer();
	// ImGui 프레임 종료
	void EndImGuiFrame();
};