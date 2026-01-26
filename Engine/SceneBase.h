#pragma once
#include "Base.h"
#include "GameObjectBase.h"
#ifdef _DEBUG
#include "DebugCamera.h"
#endif

class SceneBase : public Base
{
	#ifdef _DEBUG
	std::unique_ptr<DebugCamera> m_debugCamera = nullptr; // 디버그 카메라 게임 오브젝트
	bool m_isNavMeshCreating = false; // 네비게이션 메시 생성 중 여부
	#endif

	com_ptr<ID3D11DeviceContext> m_deviceContext = nullptr; // 디바이스 컨텍스트 포인터

	std::vector<std::unique_ptr<Base>> m_gameObjects = {}; // 게임 오브젝트 배열

	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_skyboxVertexShaderAndInputLayout = {}; // 스카이박스 정점 셰이더
	com_ptr<ID3D11PixelShader> m_skyboxPixelShader = nullptr; // 스카이박스 픽셀 셰이더

	#ifdef _DEBUG
	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_debugCoordinateVertexShaderAndInputLayout = {}; // 디버그 좌표 정점 셰이더
	com_ptr<ID3D11PixelShader> m_debugCoordinatePixelShader = nullptr; // 디버그 좌표 픽셀 셰이더
	bool m_isRenderDebugCoordinates = true; // 디버그 좌표 렌더링 여부
	#endif

	std::string m_environmentMapFileName = "Skybox.dds"; // 환경 맵 파일 이름
	com_ptr<ID3D11ShaderResourceView> m_environmentMapSRV = nullptr; // 환경 맵 셰이더 리소스 뷰

	// 정점 셰이더용 상수 버퍼
	ViewProjectionBuffer m_viewProjectionData = {}; // 뷰-투영 상수 버퍼 데이터
	com_ptr<ID3D11Buffer> m_viewProjectionConstantBuffer = nullptr; // 뷰-투영 상수 버퍼

	SkyboxViewProjectionBuffer m_skyboxViewProjectionData = {}; // 스카이박스 뷰-투영 역행렬 상수 버퍼 데이터
	com_ptr<ID3D11Buffer> m_skyboxViewProjectionConstantBuffer = nullptr; // 스카이박스 뷰-투영 역행렬 상수 버퍼

	TimeBuffer m_timeData = {}; // 시간 상수 버퍼 데이터
	com_ptr<ID3D11Buffer> m_timeConstantBuffer = nullptr; // 시간 상수 버퍼

	// 픽셀 셰이더용 상수 버퍼
	CameraPositionBuffer m_cameraPositionData = {}; // 카메라 위치 상수 버퍼 데이터
	com_ptr<ID3D11Buffer> m_cameraPositionConstantBuffer = nullptr; // 카메라 위치 상수 버퍼

	GlobalLightBuffer m_globalLightData = {}; // 환경광, 방향광 상수 버퍼 데이터
	com_ptr<ID3D11Buffer> m_globalLightConstantBuffer = nullptr; // 환경광, 방향광 상수 버퍼
	com_ptr<ID3D11PixelShader> m_shadowMapPixelShader = nullptr; // 그림자 맵 생성용 픽셀 셰이더

	bool m_showFPS = true; // FPS 표시 여부
	DirectX::SpriteFont* m_spriteFont = nullptr; // FPS 표시용 스프라이트 폰트

public:
	SceneBase() = default;
	virtual ~SceneBase() = default;
	SceneBase(const SceneBase&) = delete; // 복사 금지
	SceneBase& operator=(const SceneBase&) = delete; // 복사 대입 금지
	SceneBase(SceneBase&&) = delete; // 이동 금지
	SceneBase& operator=(SceneBase&&) = delete; // 이동 대입 금지

	// 루트 게임 오브젝트 생성 // 게임 오브젝트 베이스 포인터 반환
	GameObjectBase* CreateRootGameObject(const std::string& typeName);

	template<typename T> requires std::derived_from<T, GameObjectBase>
	T* CreateRootGameObject(); // 루트 게임 오브젝트 생성 // 포인터 반환

	GameObjectBase* GetRootGameObject(const std::string& name); // 이름으로 루트 게임 오브젝트 검색 // 없으면 nullptr 반환
	GameObjectBase* GetGameObjectRecursive(const std::string& name); // 이름으로 게임 오브젝트 재귀 검색 // 없으면 nullptr 반환

private:
	// 씬 초기화 // 씬 사용 전 반드시 호출해야 함
	void BaseInitialize() override;
	// 씬 고정 업데이트 // 씬 매니저가 호출
	void BaseFixedUpdate() override;
	// 씬 업데이트 // 씬 매니저가 호출
	void BaseUpdate() override;
	// 씬 렌더링 // 씬 매니저가 호출
	void BaseRender() override;
	// ImGui 렌더링
	void BaseRenderImGui() override;
	// 씬 종료 // 씬 매니저가 씬을 교체할 때 호출
	void BaseFinalize() override;

	// 씬 직렬화
	nlohmann::json BaseSerialize() override;
	// 씬 역직렬화
	void BaseDeserialize(const nlohmann::json& jsonData) override;

	// 제거할 게임 오브젝트 제거 // Update에서 호출
	void RemovePending() override;

	// 리소스 매니저에서 필요한 리소스 얻기
	void GetResources();
	// 상수 버퍼 업데이트
	void UpdateConstantBuffers();
	// 스카이박스 렌더링
	void RenderSkybox();

	#ifdef _DEBUG
	// 디버그 카메라로 오브젝트 선택
	void PickObjectDebugCamera();
	// 디버그 좌표 렌더링
	void RenderDebugCoordinates();
	#endif
};

template<typename T> requires std::derived_from<T, GameObjectBase>
inline T* SceneBase::CreateRootGameObject()
{
	std::unique_ptr<Base> gameObject = std::make_unique<T>();

	gameObject->BaseInitialize();
	T* gameObjectPtr = static_cast<T*>(gameObject.get());
	m_gameObjects.push_back(std::move(gameObject));

	return gameObjectPtr;
}