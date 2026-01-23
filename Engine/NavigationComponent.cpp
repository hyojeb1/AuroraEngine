#include "stdafx.h"
#include "NavigationComponent.h"

using namespace std;
using namespace DirectX;

void NavMesh::AddTriangle(const XMVECTOR& a, const XMVECTOR& b, const XMVECTOR& c)
{
	const int base = static_cast<int>(m_vertices.size());

	m_vertices.emplace_back(a);
	m_vertices.emplace_back(b);
	m_vertices.emplace_back(c);

	NavPoly poly;
	poly.indexs = { base, base + 1, base + 2 };
	poly.centroid = XMVectorScale(XMVectorAdd(XMVectorAdd(a, b), c), 1.0f / 3.0f);
	m_polys.push_back(poly);
}

void NavMesh::BuildAdjacency()
{
	// 플리곤 인덱스, 에지 인덱스
	unordered_map<pair<int, int>, pair<int, int>> edgeMap = {};

	for (int polygon = 0; polygon < static_cast<int>(m_polys.size()); ++polygon)
	{
		for (int edge = 0; edge < 3; ++edge)
		{
			pair<int, int> normalizedEdge = minmax(m_polys[polygon].indexs[edge], m_polys[polygon].indexs[(edge + 1) % 3]);
			auto it = edgeMap.find(normalizedEdge);

			if (it == edgeMap.end()) edgeMap[normalizedEdge] = { polygon, edge };
			else
			{
				m_polys[polygon].neighbors[edge] = it->second.first;
				m_polys[it->second.first].neighbors[it->second.second] = polygon;
			}
		}
	}
}

int NavMesh::FindNearestPoly(const XMVECTOR& point) const
{
	int best = -1;
	float bestDist = numeric_limits<float>::max();

	for (int i = 0; i < static_cast<int>(m_polys.size()); ++i)
	{
		if (PointInTriangle(point, m_polys[i].indexs)) return i;

		float dist = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(point, m_polys[i].centroid)));
		if (dist < bestDist) { bestDist = dist; best = i; }
	}

	return best;
}

vector<XMVECTOR> NavMesh::FindPath(const XMVECTOR& start, const XMVECTOR& end) const
{
	return std::vector<DirectX::XMVECTOR>();
}

bool NavMesh::PointInTriangle(const XMVECTOR& point, const array<int, 3>& indexs) const
{
	XMVECTOR v0 = XMVectorSubtract(m_vertices[indexs[2]], m_vertices[indexs[0]]);
	XMVECTOR v1 = XMVectorSubtract(m_vertices[indexs[1]], m_vertices[indexs[0]]);
	XMVECTOR v2 = XMVectorSubtract(point, m_vertices[indexs[0]]);

	float dot00 = XMVectorGetX(XMVector3Dot(v0, v0));
	float dot01 = XMVectorGetX(XMVector3Dot(v0, v1));
	float dot02 = XMVectorGetX(XMVector3Dot(v0, v2));
	float dot11 = XMVectorGetX(XMVector3Dot(v1, v1));
	float dot12 = XMVectorGetX(XMVector3Dot(v1, v2));

	float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
	float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	return (u >= 0) && (v >= 0) && (u + v < 1);
}