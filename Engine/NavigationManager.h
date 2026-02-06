#pragma once

struct NavPoly
{
	std::array<int, 3> indices = { -1, -1, -1 }; // 정점 인덱스
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

	std::pair<int, int> m_previewLine = { -1, -1 };
	DirectX::XMVECTOR m_previewPoint = DirectX::XMVectorZero();

	bool m_pathStartSet = false;
	DirectX::XMVECTOR m_pathStartPoint = DirectX::XMVectorZero();
	std::deque<DirectX::XMVECTOR> m_currentPath = {};

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

	// 인접 정보 구축
	void BuildAdjacency();

	#ifdef _DEBUG
	// 삼각형 추가
	void AddPolygon(const std::vector<DirectX::XMVECTOR>& vertices, const std::array<int, 3>& indices);

	// 네비메시 렌더링
	void RenderNavMesh();

	// 링크 배치 처리
	void HandlePlaceLink(float height);
	#endif

	// 가장 가까운 폴리곤 찾기
	int FindNearestPoly(const DirectX::XMVECTOR& point, float maxDist = std::numeric_limits<float>::max()) const;
	// 경로 찾기
	std::deque<DirectX::XMVECTOR> FindPath(const DirectX::XMVECTOR& start, const DirectX::XMVECTOR& end) const;

private:
	bool PointInTriangle(const DirectX::XMVECTOR& point, const std::array<int, 3>& indexs) const;
	// Simple Stupid Funnel Algorithm
	std::deque<DirectX::XMVECTOR> StringPull(const DirectX::XMVECTOR& start, const DirectX::XMVECTOR& end, const std::vector<std::pair<DirectX::XMVECTOR, DirectX::XMVECTOR>>& portals) const;
	DirectX::XMVECTOR ClosestPointOnLineSegment(const std::pair<DirectX::XMVECTOR, DirectX::XMVECTOR>& targetLine, const std::pair<DirectX::XMVECTOR, DirectX::XMVECTOR>& sourceLine) const;
};