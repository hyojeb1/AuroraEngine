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

public:
	NavigationManager() = default;
	~NavigationManager() = default;
	NavigationManager(const NavigationManager&) = delete;
	NavigationManager& operator=(const NavigationManager&) = delete;
	NavigationManager(NavigationManager&&) = delete;
	NavigationManager& operator=(NavigationManager&&) = delete;

	void ClearNavMesh() { m_vertices.clear(); m_navPolys.clear(); }

	// 삼각형 추가
	void AddTriangle(const DirectX::XMVECTOR& a, const DirectX::XMVECTOR& b, const DirectX::XMVECTOR& c);
	// 인접 정보 구축
	void BuildAdjacency();

	// 가장 가까운 폴리곤 찾기
	int FindNearestPoly(const DirectX::XMVECTOR& point) const;
	// 경로 찾기
	std::vector<DirectX::XMVECTOR> FindPath(const DirectX::XMVECTOR& start, const DirectX::XMVECTOR& end) const;

private:
	bool PointInTriangle(const DirectX::XMVECTOR& point, const std::array<int, 3>& indexs) const;
	std::vector<DirectX::XMVECTOR> StringPull(const std::vector<std::pair<DirectX::XMVECTOR, DirectX::XMVECTOR>>& portals, const DirectX::XMVECTOR& start, const DirectX::XMVECTOR& end) const;
};