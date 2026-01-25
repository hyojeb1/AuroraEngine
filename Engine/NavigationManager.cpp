#include "stdafx.h"
#include "NavigationManager.h"

#include "Renderer.h"
#include "ResourceManager.h"

using namespace std;
using namespace DirectX;

void NavigationManager::Initialize()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();
	m_navMeshVertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout("VSLine.hlsl");
	m_navMeshPixelShader = resourceManager.GetPixelShader("PSColor.hlsl");

	ClearNavMesh();
}

void NavigationManager::AddTriangle(const XMVECTOR& a, const XMVECTOR& b, const XMVECTOR& c)
{
	const int base = static_cast<int>(m_vertices.size());

	m_vertices.emplace_back(a);
	m_vertices.emplace_back(b);
	m_vertices.emplace_back(c);

	NavPoly poly = {};
	poly.indexs = { base, base + 1, base + 2 };
	poly.centroid = XMVectorScale(XMVectorAdd(XMVectorAdd(a, b), c), 1.0f / 3.0f);
	m_navPolys.push_back(poly);
}

void NavigationManager::BuildAdjacency()
{
	struct PairHash
	{
		size_t operator()(const pair<int, int>& p) const noexcept
		{
			uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(p.first)) << 32) | static_cast<uint32_t>(p.second);
			return hash<uint64_t>{}(key);
		}
	};
	// 플리곤 인덱스, 에지 인덱스
	unordered_map<pair<int, int>, pair<int, int>, PairHash> edgeMap = {};

	for (int polygon = 0; polygon < static_cast<int>(m_navPolys.size()); ++polygon)
	{
		for (int edge = 0; edge < 3; ++edge)
		{
			pair<int, int> normalizedEdge = minmax(m_navPolys[polygon].indexs[edge], m_navPolys[polygon].indexs[(edge + 1) % 3]);
			auto it = edgeMap.find(normalizedEdge);

			if (it == edgeMap.end()) edgeMap[normalizedEdge] = { polygon, edge };
			else
			{
				m_navPolys[polygon].neighbors[edge] = it->second.first;
				m_navPolys[it->second.first].neighbors[it->second.second] = polygon;
			}
		}
	}
}

void NavigationManager::RenderNavMesh()
{
	if (m_vertices.empty() || m_navPolys.empty()) return;

	Renderer::GetInstance().RENDER_FUNCTION(RenderStage::Scene, BlendState::Opaque).emplace_back
	(
		numeric_limits<float>::max(),
		[&]()
		{
			ResourceManager& resourceManager = ResourceManager::GetInstance();
			com_ptr<ID3D11DeviceContext> deviceContext = Renderer::GetInstance().GetDeviceContext();

			deviceContext->IASetInputLayout(m_navMeshVertexShaderAndInputLayout.second.Get());
			deviceContext->VSSetShader(m_navMeshVertexShaderAndInputLayout.first.Get(), nullptr, 0);
			deviceContext->PSSetShader(m_navMeshPixelShader.Get(), nullptr, 0);

			resourceManager.SetRasterState(RasterState::SolidCullNone);
			resourceManager.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

			LineBuffer lineBufferData = {};

			for (const NavPoly& poly : m_navPolys)
			{
				// validate indices
				if (poly.indexs[0] < 0 || poly.indexs[1] < 0 || poly.indexs[2] < 0) continue;

				XMVECTOR a = m_vertices[poly.indexs[0]];
				XMVECTOR b = m_vertices[poly.indexs[1]];
				XMVECTOR c = m_vertices[poly.indexs[2]];

				XMFLOAT3 fa, fb, fc;
				XMStoreFloat3(&fa, a);
				XMStoreFloat3(&fb, b);
				XMStoreFloat3(&fc, c);

				// edge AB
				lineBufferData.linePoints[0] = XMFLOAT4{ fa.x, fa.y, fa.z, 1.0f };
				lineBufferData.linePoints[1] = XMFLOAT4{ fb.x, fb.y, fb.z, 1.0f };
				lineBufferData.lineColors[0] = XMFLOAT4{ 0.0f, 1.0f, 1.0f, 1.0f };
				lineBufferData.lineColors[1] = XMFLOAT4{ 0.0f, 1.0f, 1.0f, 1.0f };
				deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
				deviceContext->Draw(2, 0);

				// edge BC
				lineBufferData.linePoints[0] = XMFLOAT4{ fb.x, fb.y, fb.z, 1.0f };
				lineBufferData.linePoints[1] = XMFLOAT4{ fc.x, fc.y, fc.z, 1.0f };
				deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
				deviceContext->Draw(2, 0);

				// edge CA
				lineBufferData.linePoints[0] = XMFLOAT4{ fc.x, fc.y, fc.z, 1.0f };
				lineBufferData.linePoints[1] = XMFLOAT4{ fa.x, fa.y, fa.z, 1.0f };
				deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
				deviceContext->Draw(2, 0);
			}
		}
	);
}

int NavigationManager::FindNearestPoly(const XMVECTOR& point) const
{
	int best = -1;
	float bestDist = numeric_limits<float>::max();

	for (int i = 0; i < static_cast<int>(m_navPolys.size()); ++i)
	{
		if (PointInTriangle(point, m_navPolys[i].indexs)) return i;

		float dist = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(point, m_navPolys[i].centroid)));
		if (dist < bestDist) { bestDist = dist; best = i; }
	}

	return best;
}

vector<XMVECTOR> NavigationManager::FindPath(const XMVECTOR& start, const XMVECTOR& end) const
{
	vector<XMVECTOR> empty = {};

	if (m_navPolys.empty()) return empty;

	int startPoly = FindNearestPoly(start);
	int endPoly = FindNearestPoly(end);

	if (startPoly < 0 || endPoly < 0) return empty;
	if (startPoly == endPoly) return { start, end };

	// A*
	struct Node
	{
		int poly;
		float g;
		float f;
		int parent;
	};
	auto heuristic = [&](int a, int b) { return XMVectorGetX(XMVector3Length(XMVectorSubtract(m_navPolys[a].centroid, m_navPolys[b].centroid))); };

	unordered_map<int, Node> nodes = {};
	priority_queue<pair<float, int>, vector<pair<float, int>>, greater<>> openSet;

	nodes[startPoly] = { startPoly, 0.0f, heuristic(startPoly, endPoly), -1 };
	openSet.push({ nodes[startPoly].f, startPoly });

	bool found = false;
	while (!openSet.empty())
	{
		float poppedF = openSet.top().first;
		int currentPoly = openSet.top().second;
		openSet.pop();

		auto itNode = nodes.find(currentPoly);
		if (itNode == nodes.end()) continue;

		if (poppedF > itNode->second.f + numeric_limits<float>::epsilon() * max(1.0f, max(fabs(poppedF), fabs(itNode->second.f)))) continue;
		if (currentPoly == endPoly) { found = true; break; }

		for (int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
		{
			int neighborPoly = m_navPolys[currentPoly].neighbors[edgeIndex];
			if (neighborPoly < 0) continue;

			float tentative_g = itNode->second.g + heuristic(currentPoly, neighborPoly);
			auto itNeighbor = nodes.find(neighborPoly);

			if (itNeighbor == nodes.end() || tentative_g < itNeighbor->second.g)
			{
				Node next = {};
				next.poly = neighborPoly;
				next.g = tentative_g;
				next.f = tentative_g + heuristic(neighborPoly, endPoly);
				next.parent = currentPoly;
				nodes[neighborPoly] = next;

				openSet.push({ next.f, neighborPoly });
			}
		}
	}
	if (!found) return empty;

	vector<int> polyPath = {};
	int currentPoly = endPoly;
	while (currentPoly != -1)
	{
		polyPath.push_back(currentPoly);
		currentPoly = nodes[currentPoly].parent;
	}
	reverse(polyPath.begin(), polyPath.end());

	vector<pair<XMVECTOR, XMVECTOR>> portals = {};
	for (size_t i = 0; i + 1 < polyPath.size(); ++i)
	{
		bool foundEdge = false;
		for (int edgeIndexI = 0; edgeIndexI < 3 && !foundEdge; ++edgeIndexI)
		{
			pair<int, int> edgeA = { m_navPolys[polyPath[i]].indexs[edgeIndexI], m_navPolys[polyPath[i]].indexs[(edgeIndexI + 1) % 3] };

			for (int edgeIndexJ = 0; edgeIndexJ < 3 && !foundEdge; ++edgeIndexJ)
			{
				pair<int, int> edgeB = { m_navPolys[polyPath[i + 1]].indexs[edgeIndexJ], m_navPolys[polyPath[i + 1]].indexs[(edgeIndexJ + 1) % 3] };

				if ((edgeA.first == edgeB.first && edgeA.second == edgeB.second) || (edgeA.first == edgeB.second && edgeA.second == edgeB.first))
				{
					XMVECTOR edgeDirection = XMVectorSubtract(m_vertices[edgeA.second], m_vertices[edgeA.first]);
					XMVECTOR toCenter = XMVectorSubtract(m_navPolys[polyPath[i]].centroid, m_vertices[edgeA.first]);
					XMVECTOR cross = XMVector3Cross(edgeDirection, toCenter);

					if (XMVectorGetY(cross) >= 0) portals.emplace_back(m_vertices[edgeA.first], m_vertices[edgeA.second]);
					else portals.emplace_back(m_vertices[edgeA.second], m_vertices[edgeA.first]);

					foundEdge = true;
					break;
				}
			}
		}
	}

	return StringPull(portals, start, end);
}

bool NavigationManager::PointInTriangle(const XMVECTOR& point, const array<int, 3>& indexs) const
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

vector<XMVECTOR> NavigationManager::StringPull(const vector<pair<XMVECTOR, XMVECTOR>>& portals, const XMVECTOR& start, const XMVECTOR& end) const
{
	vector<XMVECTOR> smoothPath;
	if (portals.empty()) { smoothPath.push_back(start); smoothPath.push_back(end); return smoothPath; }

	vector<XMVECTOR> left = {};
	vector<XMVECTOR> right = {};
	left.push_back(start);
	right.push_back(start);
	for (const auto& portal : portals) { left.push_back(portal.first); right.push_back(portal.second); }
	left.push_back(end);
	right.push_back(end);

	int apex = 0;
	int leftIndex = 0;
	int rightIndex = 0;
	XMVECTOR apexPoint = left[0];
	XMVECTOR leftPoint = left[1];
	XMVECTOR rightPoint = right[1];

	smoothPath.push_back(apexPoint);

	for (int i = 1; i < static_cast<int>(left.size()); ++i)
	{
		XMVECTOR newLeft = left[i];
		XMVECTOR newRight = right[i];

		XMVECTOR crossR = XMVector3Cross(XMVectorSubtract(newRight, apexPoint), XMVectorSubtract(rightPoint, apexPoint));
		if (XMVectorGetX(XMVector3Dot(crossR, crossR)) > numeric_limits<float>::epsilon())
		{
			if (XMVectorGetY(XMVector3Cross(XMVectorSubtract(newRight, apexPoint), XMVectorSubtract(apexPoint, leftPoint))) > 0.0f)
			{
				apexPoint = leftPoint;
				smoothPath.push_back(apexPoint);
				apex = leftIndex;
				leftIndex = apex;
				rightIndex = apex;
				if (apex + 1 < static_cast<int>(left.size())) { leftPoint = left[apex + 1]; rightPoint = right[apex + 1]; }
				i = apex;
				continue;
			}
			rightPoint = newRight;
			rightIndex = i;
		}

		XMVECTOR crossL = XMVector3Cross(XMVectorSubtract(leftPoint, apexPoint), XMVectorSubtract(newLeft, apexPoint));
		if (XMVectorGetX(XMVector3Dot(crossL, crossL)) > numeric_limits<float>::epsilon())
		{
			if (XMVectorGetY(XMVector3Cross(XMVectorSubtract(leftPoint, apexPoint), XMVectorSubtract(newLeft, apexPoint))) > 0.0f)
			{
				apexPoint = rightPoint;
				smoothPath.push_back(apexPoint);
				apex = rightIndex;
				leftIndex = apex;
				rightIndex = apex;
				if (apex + 1 < static_cast<int>(left.size())) { leftPoint = left[apex + 1]; rightPoint = right[apex + 1]; }
				i = apex;
				continue;
			}
			leftPoint = newLeft;
			leftIndex = i;
		}
	}

	smoothPath.push_back(end);
	return smoothPath;
}