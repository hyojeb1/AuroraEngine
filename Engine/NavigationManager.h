#pragma once
#include "Singleton.h"

struct NavPoly
{
	std::array<int, 3> indexs = { -1, -1, -1 }; // 정점 인덱스
	std::array<int, 3> neighbors = { -1, -1, -1 }; // 인접 폴리곤 인덱스
	DirectX::XMVECTOR centroid = {}; // 중심점
};

class NavigationManager : public Singleton<NavigationManager>
{
	friend class Singleton<NavigationManager>;

	std::vector<DirectX::XMVECTOR> m_vertices = {};
	std::vector<NavPoly> m_navPolys = {};

	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_navMeshVertexShaderAndInputLayout = {};
	com_ptr<ID3D11PixelShader> m_navMeshPixelShader = {};

	bool m_hasPreview = false;
	DirectX::XMVECTOR m_previewPoint = DirectX::XMVectorZero();
	int m_previewEdgeVertexIndexA = -1;
	int m_previewEdgeVertexIndexB = -1;

	bool m_pathStartSet = false;
	DirectX::XMVECTOR m_pathStartPoint = DirectX::XMVectorZero();
	std::vector<DirectX::XMVECTOR> m_currentPath = {};

public:
	NavigationManager() = default;
	~NavigationManager() = default;
	NavigationManager(const NavigationManager&) = delete;
	NavigationManager& operator=(const NavigationManager&) = delete;
	NavigationManager(NavigationManager&&) = delete;
	NavigationManager& operator=(NavigationManager&&) = delete;

	void Initialize();

	void Deserialize(const nlohmann::json& jsonData);
	nlohmann::json Serialize() const;

	void ClearNavMesh();

	// 삼각형 추가
	void AddPolygon(const DirectX::XMVECTOR& a, const DirectX::XMVECTOR& b, const DirectX::XMVECTOR& c);
	// 인접 정보 구축
	void BuildAdjacency();

	// 네비메시 렌더링
	void RenderNavMesh();

	// 가장 가까운 폴리곤 찾기
	int FindNearestPoly(const DirectX::XMVECTOR& point) const;
	// 경로 찾기
	std::vector<DirectX::XMVECTOR> FindPath(const DirectX::XMVECTOR& start, const DirectX::XMVECTOR& end) const;

	void HandlePlaceLink();

private:
	bool PointInTriangle(const DirectX::XMVECTOR& point, const std::array<int, 3>& indexs) const;
};