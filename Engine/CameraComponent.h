#pragma once
#include "ComponentBase.h"

extern class CameraComponent* g_mainCamera; // 전역 메인 카메라 컴포넌트 포인터

class CameraComponent : public ComponentBase
{
	float m_fovY = DirectX::XM_PIDIV4; // 수직 시야각 (라디안 단위)

	UINT m_screenWidth = 1280; // 화면 너비
	UINT m_screenHeight = 720; // 화면 높이

	float m_nearZ = 0.1f; // 근평면
	float m_farZ = 1000.0f; // 원평면

	DirectX::XMMATRIX m_viewMatrix = DirectX::XMMatrixIdentity(); // 뷰 행렬
	DirectX::XMMATRIX m_projectionMatrix = DirectX::XMMatrixIdentity(); // 투영 행렬
	DirectX::BoundingFrustum m_boundingFrustum = {}; // 카메라 절두체

	const DirectX::XMVECTOR* m_position = nullptr; // 카메라 위치
	DirectX::XMVECTOR m_forwardVector = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // 카메라 앞 방향 벡터

	#ifdef _DEBUG
	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_boundingFrustumVertexShaderAndInputLayout = {}; // 프러스텀 정점 셰이더 및 입력 레이아웃
	com_ptr<ID3D11PixelShader> m_boundingFrustumPixelShader = nullptr; // 프러스텀 픽셀 셰이더
	#endif

public:
	CameraComponent() = default;
	~CameraComponent() override = default;
	CameraComponent(const CameraComponent&) = default;
	CameraComponent& operator=(const CameraComponent&) = default;
	CameraComponent(CameraComponent&&) = default;
	CameraComponent& operator=(CameraComponent&&) = default;

	void SetFovY(float fovY) { m_fovY = fovY; }
	float GetFovY() const { return m_fovY; }
	void SetNearZ(float nearZ) { m_nearZ = nearZ; }
	float GetNearZ() const { return m_nearZ; }
	void SetFarZ(float farZ) { m_farZ = farZ; }
	float GetFarZ() const { return m_farZ; }

	const DirectX::XMMATRIX& GetViewMatrix() const { return m_viewMatrix; }
	const DirectX::XMMATRIX& GetProjectionMatrix() const { return m_projectionMatrix; }
	const DirectX::BoundingFrustum GetBoundingFrustum() const;

	const DirectX::XMVECTOR& GetPosition() const { return *m_position; }
	const DirectX::XMVECTOR& GetForwardVector() const { return m_forwardVector; }

	bool NeedsFixedUpdate() const override { return false; }
	bool NeedsUpdate() const override { return true; }
	#ifdef _DEBUG
	bool NeedsRender() const override { return true; }
	#else
	bool NeedsRender() const override { return false; }
	#endif

private:
	void Initialize() override;

	// 위치, 뷰 행렬 갱신
	void Update() override { UpdateViewMatrix(); UpdateProjectionMatrix(); }
	void Render() override; // 카메라 프러스텀 렌더링
	void RenderImGui() override;

	void Finalize() override;

	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;

	// 뷰 행렬 갱신
	void UpdateViewMatrix();
	// 투영 행렬 갱신
	void UpdateProjectionMatrix();
};