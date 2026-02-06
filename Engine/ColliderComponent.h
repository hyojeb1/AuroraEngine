#pragma once
#include "ComponentBase.h"

class ColliderComponent : public ComponentBase
{
	static std::vector<ColliderComponent*> s_colliders; // 모든 콜라이더 컴포넌트 배열

	// 로컬 좌표계 기준 경계 상자, 방향 상자, 절두체 쌍 배열 (로컬, 월드)
	std::vector<std::pair<DirectX::BoundingBox, DirectX::BoundingBox>> m_boundingBoxes = {};
	std::vector<std::pair<DirectX::BoundingOrientedBox, DirectX::BoundingOrientedBox>> m_boundingOrientedBoxes = {};
	std::vector<std::pair<DirectX::BoundingFrustum, DirectX::BoundingFrustum>> m_boundingFrustums = {};

	#ifdef _DEBUG
	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_boundingShapeVertexShaderAndInputLayout = {}; // 경계 상자 정점 셰이더 및 입력 레이아웃
	com_ptr<ID3D11PixelShader> m_boundingShapePixelShader = nullptr; // 경계 상자 픽셀 셰이더
	#endif

public:
	ColliderComponent() = default;
	virtual ~ColliderComponent() override = default;
	ColliderComponent(const ColliderComponent&) = default;
	ColliderComponent& operator=(const ColliderComponent&) = default;
	ColliderComponent(ColliderComponent&&) = default;
	ColliderComponent& operator=(ColliderComponent&&) = default;

	// 로컬 좌표계 기준 경계 상자 추가
	void AddBoundingBox(const DirectX::BoundingBox& box) { m_boundingBoxes.push_back({ box, {} }); }
	// 로컬 좌표계 기준 경계 방향 상자 추가
	void AddBoundingOrientedBox(const DirectX::BoundingOrientedBox& obb) { m_boundingOrientedBoxes.push_back({ obb, {} }); }
	// 로컬 좌표계 기준 경계 절두체 추가
	void AddBoundingFrustum(const DirectX::BoundingFrustum& frustum) { m_boundingFrustums.push_back({ frustum, {} }); }

	// 충돌 검사
	// 선 충돌 검사
	static GameObjectBase* CheckCollision(const DirectX::XMVECTOR& origin, const DirectX::XMVECTOR& direction, _Out_ float& distance);
	// 상자 충돌 검사
	static std::vector<GameObjectBase*> CheckCollision(const DirectX::BoundingBox& box);
	// 절두체 충돌 검사 // 화면 안에 있는 오브젝트
	static std::vector<GameObjectBase*> CheckCollision(const DirectX::BoundingFrustum& frustum);

	// 객체 충돌 검사
	bool CheckCollisionWithObject(ColliderComponent* otherCollider);

	bool NeedsFixedUpdate() const override { return true; }
	bool NeedsUpdate() const override { return true; }
	#ifdef _DEBUG
	bool NeedsRender() const override { return true; }
	#else
	bool NeedsRender() const override { return false; }
	#endif

protected:
	void Initialize() override;
	void FixedUpdate() override;
	void Update() override;
	#ifdef _DEBUG
	void Render() override;
	void RenderImGui() override;
	#endif
	void Finalize() override;

	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;

	void LoadFromModelMesh();
};