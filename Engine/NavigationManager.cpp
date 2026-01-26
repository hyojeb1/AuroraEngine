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

	if (jsonData.contains("navPoly"))
	{
		for (const nlohmann::json& polyData : jsonData["navPoly"])
		{
			XMVECTOR a = XMVectorSet(polyData["a"][0], polyData["a"][1], polyData["a"][2], 1.0f);
			XMVECTOR b = XMVectorSet(polyData["b"][0], polyData["b"][1], polyData["b"][2], 1.0f);
			XMVECTOR c = XMVectorSet(polyData["c"][0], polyData["c"][1], polyData["c"][2], 1.0f);
			AddPolygon(a, b, c);
		}
	}
	else AddPolygon(XMVectorSet(-1.0f, 0.0f, -1.0f, 1.0f), XMVectorSet(1.0f, 0.0f, -1.0f, 1.0f), XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f));

	BuildAdjacency();
}

nlohmann::json NavigationManager::Serialize() const
{
	nlohmann::json jsonData = {};
	nlohmann::json navMeshData = nlohmann::json::array();

	for (const NavPoly& poly : m_navPolys)
	{
		if (poly.indexs[0] < 0 || poly.indexs[1] < 0 || poly.indexs[2] < 0) continue;

		XMFLOAT3 a, b, c;
		XMStoreFloat3(&a, m_vertices[poly.indexs[0]]);
		XMStoreFloat3(&b, m_vertices[poly.indexs[1]]);
		XMStoreFloat3(&c, m_vertices[poly.indexs[2]]);

		nlohmann::json polyData = {};
		polyData["a"] = { a.x, a.y, a.z };
		polyData["b"] = { b.x, b.y, b.z };
		polyData["c"] = { c.x, c.y, c.z };
		navMeshData.push_back(polyData);
	}

	jsonData["navPoly"] = navMeshData;
	return jsonData;
}

void NavigationManager::ClearNavMesh()
{
	m_vertices.clear();
	m_navPolys.clear();

	m_hasPreview = false;
	m_previewEdgeVertexIndexA = m_previewEdgeVertexIndexB = -1;
	m_pathStartSet = false;
	m_currentPath.clear();
}

void NavigationManager::AddPolygon(const XMVECTOR& a, const XMVECTOR& b, const XMVECTOR& c)
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
				if (poly.indexs[0] < 0 || poly.indexs[1] < 0 || poly.indexs[2] < 0) continue;

				XMVECTOR a = m_vertices[poly.indexs[0]];
				XMVECTOR b = m_vertices[poly.indexs[1]];
				XMVECTOR c = m_vertices[poly.indexs[2]];

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

			if (m_hasPreview && m_previewEdgeVertexIndexA >= 0 && m_previewEdgeVertexIndexB >= 0)
			{
				XMVECTOR va = m_vertices[m_previewEdgeVertexIndexA];
				XMVECTOR vb = m_vertices[m_previewEdgeVertexIndexB];
				XMFLOAT3 fa, fb, fm;
				XMStoreFloat3(&fa, va);
				XMStoreFloat3(&fb, vb);
				XMStoreFloat3(&fm, m_previewPoint);

				lineBufferData.linePoints[0] = XMFLOAT4{ fm.x, fm.y, fm.z, 1.0f };
				lineBufferData.linePoints[1] = XMFLOAT4{ fa.x, fa.y, fa.z, 1.0f };
				lineBufferData.lineColors[0] = XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f };
				lineBufferData.lineColors[1] = XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f };
				deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
				deviceContext->Draw(2, 0);

				lineBufferData.linePoints[0] = XMFLOAT4{ fm.x, fm.y, fm.z, 1.0f };
				lineBufferData.linePoints[1] = XMFLOAT4{ fb.x, fb.y, fb.z, 1.0f };
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

	if (input.GetKeyDown(KeyCode::R))
	{
		ClearNavMesh();
		AddPolygon(XMVectorSet(-1.0f, 0.0f, -1.0f, 1.0f), XMVectorSet(1.0f, 0.0f, -1.0f, 1.0f), XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f));
		BuildAdjacency();
	}

	const POINT& mouse = input.GetMousePosition();
	const DXGI_SWAP_CHAIN_DESC1& scDesc = Renderer::GetInstance().GetSwapChainDesc();
	const CameraComponent& cam = CameraComponent::GetMainCamera();

	XMVECTOR screenNear = XMVectorSet(static_cast<float>(mouse.x), static_cast<float>(mouse.y), 0.0f, 0.0f);
	XMVECTOR screenFar  = XMVectorSet(static_cast<float>(mouse.x), static_cast<float>(mouse.y), 1.0f, 0.0f);

	XMVECTOR nearPoint = XMVector3Unproject(screenNear, 0.0f, 0.0f, static_cast<float>(scDesc.Width), static_cast<float>(scDesc.Height), 0.0f, 1.0f, cam.GetProjectionMatrix(), cam.GetViewMatrix(), XMMatrixIdentity());
	XMVECTOR farPoint  = XMVector3Unproject(screenFar,  0.0f, 0.0f, static_cast<float>(scDesc.Width), static_cast<float>(scDesc.Height), 0.0f, 1.0f, cam.GetProjectionMatrix(), cam.GetViewMatrix(), XMMatrixIdentity());

	XMVECTOR dir = XMVector3Normalize(XMVectorSubtract(farPoint, nearPoint));

	float dirY = XMVectorGetY(dir);
	if (fabs(dirY) < numeric_limits<float>::epsilon()) return;

	float t = -XMVectorGetY(nearPoint) / dirY;
	if (t < 0.0f) return;

	XMVECTOR hit = XMVectorAdd(nearPoint, XMVectorScale(dir, t));
	hit = XMVectorSetY(hit, 0.0f);

	if (m_navPolys.empty() || m_vertices.empty()) return;

	int polyIndex = FindNearestPoly(hit);
	if (polyIndex < 0) return;

	const NavPoly& poly = m_navPolys[polyIndex];
	float bestDistSq = numeric_limits<float>::max();
	int bestA = -1, bestB = -1;

	for (int index = 0; index < 3; ++index)
	{
		int indexA = poly.indexs[index];
		int indexB = poly.indexs[(index + 1) % 3];

		XMVECTOR v0 = m_vertices[indexA];
		XMVECTOR v1 = m_vertices[indexB];
		XMVECTOR edge = XMVectorSubtract(v1, v0);
		XMVECTOR toHit = XMVectorSubtract(hit, v0);

		float edgeLenSq = XMVectorGetX(XMVector3Dot(edge, edge));
		float u = 0.0f;
		if (edgeLenSq > 0.0f) u = XMVectorGetX(XMVector3Dot(toHit, edge)) / edgeLenSq;
		u = clamp(u, 0.0f, 1.0f);

		XMVECTOR proj = XMVectorAdd(v0, XMVectorScale(edge, u));
		float distSq = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(hit, proj)));

		if (distSq < bestDistSq)
		{
			bestDistSq = distSq;
			bestA = indexA;
			bestB = indexB;
		}
	}

	if (bestA < 0 || bestB < 0) return;

	m_hasPreview = true;
	m_previewPoint = hit;
	m_previewEdgeVertexIndexA = bestA;
	m_previewEdgeVertexIndexB = bestB;

	if (input.GetKeyDown(KeyCode::E))
	{
		int newIndex = static_cast<int>(m_vertices.size());
		m_vertices.emplace_back(hit);

		NavPoly newPoly = {};
		newPoly.indexs = { bestA, bestB, newIndex };

		XMVECTOR ca = m_vertices[newPoly.indexs[0]];
		XMVECTOR cb = m_vertices[newPoly.indexs[1]];
		XMVECTOR cc = m_vertices[newPoly.indexs[2]];
		newPoly.centroid = XMVectorScale(XMVectorAdd(XMVectorAdd(ca, cb), cc), 1.0f / 3.0f);

		m_navPolys.push_back(newPoly);

		BuildAdjacency();

		m_hasPreview = false;
		m_previewEdgeVertexIndexA = -1;
		m_previewEdgeVertexIndexB = -1;
	}

	if (input.GetKeyDown(KeyCode::Q))
	{
		if (!m_pathStartSet)
		{
			m_pathStartSet = true;
			m_pathStartPoint = hit;
			m_currentPath.clear();
		}
		else if (m_currentPath.empty())
		{
			m_currentPath = FindPath(m_pathStartPoint, hit);
		}
		else
		{
			m_pathStartSet = false;
			m_currentPath.clear();
		}
	}
}