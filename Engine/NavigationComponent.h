#pragma once
#include "ComponentBase.h"

struct NavPoly
{
	std::array<int, 3> indexs = { -1, -1, -1 }; // 정점 인덱스
	std::array<int, 3> neighbors = { -1, -1, -1 }; // 인접 폴리곤 인덱스
	DirectX::XMVECTOR centroid = {}; // 중심점
};

class NavMesh
{
	std::vector<DirectX::XMVECTOR> m_vertices = {};
	std::vector<NavPoly> m_polys = {};

public:
	void AddTriangle(const DirectX::XMVECTOR& a, const DirectX::XMVECTOR& b, const DirectX::XMVECTOR& c);

	// 인접 정보 구축
	void BuildAdjacency();

	int FindNearestPoly(const DirectX::XMVECTOR& point) const;
	std::vector<DirectX::XMVECTOR> FindPath(const DirectX::XMVECTOR& start, const DirectX::XMVECTOR& end) const;

private:
	bool PointInTriangle(const DirectX::XMVECTOR& point, const std::array<int, 3>& indexs) const;
};

class NavigationComponent : public ComponentBase
{

public:
	NavigationComponent() = default;
	virtual ~NavigationComponent() override = default;
	NavigationComponent(const NavigationComponent&) = default;
	NavigationComponent& operator=(const NavigationComponent&) = default;
	NavigationComponent(NavigationComponent&&) = default;
	NavigationComponent& operator=(NavigationComponent&&) = default;
};