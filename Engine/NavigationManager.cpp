#include "stdafx.h"
#include "NavigationManager.h"

#include "Renderer.h"
#include "ResourceManager.h"
#include "InputManager.h"
#include "CameraComponent.h"

using namespace std;
using namespace DirectX;

void NavigationManager::Initialize()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();
	m_navMeshVertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout("VSLine.hlsl");
	m_navMeshPixelShader = resourceManager.GetPixelShader("PSColor.hlsl");
}

void NavigationManager::Deserialize(const nlohmann::json& jsonData)
{
	ClearNavMesh();

	if (jsonData.find("navPolyIndices") == jsonData.end() || jsonData.find("navVertices") == jsonData.end()) return;

	const nlohmann::json& navPolyData = jsonData["navPolyIndices"];
	const nlohmann::json& navVertexData = jsonData["navVertices"];

	for (const auto& vertexData : navVertexData)
	{
		XMVECTOR vertex = XMVectorSet
		(
			vertexData[0].get<float>(),
			vertexData[1].get<float>(),
			vertexData[2].get<float>(),
			1.0f
		);
		m_vertices.emplace_back(vertex);
	}
	for (const auto& polyData : navPolyData)
	{
		array<int, 3> indices =
		{
			polyData[0].get<int>(),
			polyData[1].get<int>(),
			polyData[2].get<int>()
		};
		m_navPolys.emplace_back(NavPoly{ indices, { -1, -1, -1 }, XMVectorScale(XMVectorAdd(XMVectorAdd(m_vertices[indices[0]], m_vertices[indices[1]]), m_vertices[indices[2]]), 1.0f / 3.0f) });
	}

	BuildAdjacency();
}

nlohmann::json NavigationManager::Serialize() const
{
	nlohmann::json jsonData = {};
	nlohmann::json navPolyData = nlohmann::json::array();
	nlohmann::json navVertexData = nlohmann::json::array();

	for (const NavPoly& poly : m_navPolys) navPolyData.push_back({ poly.indices[0], poly.indices[1], poly.indices[2] });
	for (const XMVECTOR& vertex : m_vertices)
	{
		XMFLOAT3 float3;
		XMStoreFloat3(&float3, vertex);
		navVertexData.push_back({ float3.x, float3.y, float3.z });
	}

	jsonData["navPolyIndices"] = navPolyData;
	jsonData["navVertices"] = navVertexData;

	return jsonData;
}

void NavigationManager::ClearNavMesh()
{
	m_vertices.clear();
	m_navPolys.clear();

	m_previewLine = { -1, -1 };
	m_pathStartSet = false;
	m_currentPath.clear();
}

void NavigationManager::AddPolygon(const vector<XMVECTOR>& vertices, const array<int, 3>& indices)
{
	m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());

	m_navPolys.emplace_back(NavPoly{ indices, { -1, -1, -1 }, XMVectorScale(XMVectorAdd(XMVectorAdd(m_vertices[indices[0]], m_vertices[indices[1]]), m_vertices[indices[2]]), 1.0f / 3.0f) });
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
			pair<int, int> normalizedEdge = minmax(m_navPolys[polygon].indices[edge], m_navPolys[polygon].indices[(edge + 1) % 3]);
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
				if (poly.indices[0] < 0 || poly.indices[1] < 0 || poly.indices[2] < 0) continue;

				XMVECTOR a = m_vertices[poly.indices[0]];
				XMVECTOR b = m_vertices[poly.indices[1]];
				XMVECTOR c = m_vertices[poly.indices[2]];

				XMFLOAT3 fa, fb, fc;
				XMStoreFloat3(&fa, a);
				XMStoreFloat3(&fb, b);
				XMStoreFloat3(&fc, c);

				lineBufferData.linePoints[0] = XMFLOAT4{ fa.x, fa.y, fa.z, 1.0f };
				lineBufferData.linePoints[1] = XMFLOAT4{ fb.x, fb.y, fb.z, 1.0f };
				lineBufferData.lineColors[0] = XMFLOAT4{ 0.0f, 1.0f, 1.0f, 1.0f };
				lineBufferData.lineColors[1] = XMFLOAT4{ 0.0f, 1.0f, 1.0f, 1.0f };
				deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
				deviceContext->Draw(2, 0);

				lineBufferData.linePoints[0] = XMFLOAT4{ fb.x, fb.y, fb.z, 1.0f };
				lineBufferData.linePoints[1] = XMFLOAT4{ fc.x, fc.y, fc.z, 1.0f };
				deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
				deviceContext->Draw(2, 0);

				lineBufferData.linePoints[0] = XMFLOAT4{ fc.x, fc.y, fc.z, 1.0f };
				lineBufferData.linePoints[1] = XMFLOAT4{ fa.x, fa.y, fa.z, 1.0f };
				deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
				deviceContext->Draw(2, 0);
			}

			if (m_previewLine.first >= 0 && m_previewLine.second >= 0)
			{
				XMStoreFloat4(&lineBufferData.linePoints[0], m_previewPoint);
				XMStoreFloat4(&lineBufferData.linePoints[1], m_vertices[m_previewLine.first]);
				lineBufferData.lineColors[0] = XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f };
				lineBufferData.lineColors[1] = XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f };
				deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
				deviceContext->Draw(2, 0);

				XMStoreFloat4(&lineBufferData.linePoints[0], m_previewPoint);
				XMStoreFloat4(&lineBufferData.linePoints[1], m_vertices[m_previewLine.second]);
				lineBufferData.lineColors[0] = XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f };
				lineBufferData.lineColors[1] = XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f };
				deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
				deviceContext->Draw(2, 0);
			}

			if (m_pathStartSet && m_currentPath.empty())
			{
				XMFLOAT3 sp;
				XMStoreFloat3(&sp, m_pathStartPoint);
				lineBufferData.linePoints[0] = XMFLOAT4{ sp.x - 0.1f, sp.y, sp.z, 1.0f };
				lineBufferData.linePoints[1] = XMFLOAT4{ sp.x + 0.1f, sp.y, sp.z, 1.0f };
				lineBufferData.lineColors[0] = XMFLOAT4{ 1.0f, 1.0f, 0.0f, 1.0f };
				lineBufferData.lineColors[1] = XMFLOAT4{ 1.0f, 1.0f, 0.0f, 1.0f };
				deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
				deviceContext->Draw(2, 0);
			}

			if (!m_currentPath.empty())
			{
				for (size_t i = 0; i + 1 < m_currentPath.size(); ++i)
				{
					XMFLOAT3 p0, p1;
					XMStoreFloat3(&p0, m_currentPath[i]);
					XMStoreFloat3(&p1, m_currentPath[i + 1]);

					lineBufferData.linePoints[0] = XMFLOAT4{ p0.x, p0.y, p0.z, 1.0f };
					lineBufferData.linePoints[1] = XMFLOAT4{ p1.x, p1.y, p1.z, 1.0f };
					lineBufferData.lineColors[0] = XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f };
					lineBufferData.lineColors[1] = XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f };
					deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
					deviceContext->Draw(2, 0);
				}
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
		if (PointInTriangle(point, m_navPolys[i].indices)) return i;

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
			pair<int, int> edgeA = { m_navPolys[polyPath[i]].indices[edgeIndexI], m_navPolys[polyPath[i]].indices[(edgeIndexI + 1) % 3] };

			for (int edgeIndexJ = 0; edgeIndexJ < 3 && !foundEdge; ++edgeIndexJ)
			{
				pair<int, int> edgeB = { m_navPolys[polyPath[i + 1]].indices[edgeIndexJ], m_navPolys[polyPath[i + 1]].indices[(edgeIndexJ + 1) % 3] };

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

	vector<XMVECTOR> result;
	if (portals.empty())
	{
		result.push_back(start);
		result.push_back(end);
		return result;
	}

	result.reserve(portals.size() + 2);
	result.push_back(start);

	for (const auto& p : portals)
	{
		XMVECTOR mid = XMVectorScale(XMVectorAdd(p.first, p.second), 0.5f);
		mid = XMVectorSetY(mid, 0.0f);
		result.push_back(mid);
	}

	result.push_back(end);

	return result;
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

void NavigationManager::HandlePlaceLink()
{
	InputManager& input = InputManager::GetInstance();

	const POINT& mouse = input.GetMousePosition();
	const CameraComponent& camera = CameraComponent::GetMainCamera();
	pair<XMVECTOR, XMVECTOR> ray = camera.RayCast(static_cast<float>(mouse.x), static_cast<float>(mouse.y));

	m_previewPoint = XMPlaneIntersectLine(XMPlaneFromPointNormal(XMVectorZero(), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)), ray.first, ray.second);

	int bestVertexIndex = -1;
	if (m_previewLine.first >= 0 && m_previewLine.second >= 0)
	{
		float bestVertexDist = numeric_limits<float>::max();
		for (int i = 0; i < static_cast<int>(m_vertices.size()); ++i)
		{
			if (i == m_previewLine.first || i == m_previewLine.second) continue;

			float dist = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(m_vertices[i], m_previewPoint)));
			if (dist < bestVertexDist)
			{
				bestVertexDist = dist;
				bestVertexIndex = i;
			}
		}

		if (bestVertexDist < 5.0f) m_previewPoint = m_vertices[bestVertexIndex];
		else bestVertexIndex = -1;
	}

	if (input.GetKeyDown(KeyCode::E))
	{
		float bestLineDist = numeric_limits<float>::max();

		for (const NavPoly& poly : m_navPolys)
		{
			for (int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
			{
				float dist = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(XMVectorScale(XMVectorAdd(m_vertices[poly.indices[edgeIndex]], m_vertices[poly.indices[(edgeIndex + 1) % 3]]), 0.5f), m_previewPoint)));

				if (dist < bestLineDist)
				{
					bestLineDist = dist;
					m_previewLine = { poly.indices[edgeIndex], poly.indices[(edgeIndex + 1) % 3] };
				}
			}
		}
	}

	if (input.GetKeyDown(KeyCode::MouseLeft) && m_previewLine.first >= 0 && m_previewLine.second >= 0)
	{
		if (bestVertexIndex >= 0) AddPolygon({}, { m_previewLine.first, m_previewLine.second, bestVertexIndex });
		else AddPolygon({ m_previewPoint }, { m_previewLine.first, m_previewLine.second, static_cast<int>(m_vertices.size()) });

		BuildAdjacency();
		m_previewLine = { -1, -1 };
	}

	if (input.GetKeyDown(KeyCode::Q))
	{
		if (!m_pathStartSet)
		{
			m_pathStartSet = true;
			m_pathStartPoint = m_previewPoint;
			m_currentPath.clear();
		}
		else if (m_currentPath.empty())
		{
			m_currentPath = FindPath(m_pathStartPoint, m_previewPoint);
		}
		else
		{
			m_pathStartSet = false;
			m_currentPath.clear();
		}
	}

	if (input.GetKeyDown(KeyCode::R))
	{
		ClearNavMesh();

		const vector<XMVECTOR> VERTICES = { XMVECTOR{-5.0f, 0.0f, -5.0f, 1.0f}, XMVECTOR{5.0f, 0.0f, -5.0f, 1.0f}, XMVECTOR{0.0f, 0.0f, 5.0f, 1.0f} };
		constexpr array<int, 3> INDICES = { 0, 1, 2 };
		AddPolygon(VERTICES, INDICES);

		BuildAdjacency();
	}
}