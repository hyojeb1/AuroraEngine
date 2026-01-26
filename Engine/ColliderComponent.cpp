#include "stdafx.h"
#include "ColliderComponent.h"

#include "GameObjectBase.h"
#include "ResourceManager.h"
#include "Renderer.h"

using namespace std;
using namespace DirectX;

REGISTER_TYPE(ColliderComponent)

vector<ColliderComponent*> ColliderComponent::s_colliders = {};

GameObjectBase* ColliderComponent::CheckCollision(const XMVECTOR& origin, const XMVECTOR& direction, _Out_ float& distance)
{
	GameObjectBase* collidedObject = nullptr;
	float closestDistance = numeric_limits<float>::max();
	XMVECTOR dirNormalized = XMVector3Normalize(direction);

	for (ColliderComponent* collider : s_colliders)
	{
		for (const auto& [box, transformedBox] : collider->m_boundingBoxes)
		{
			float dist = 0.0f;
			if (transformedBox.Intersects(origin, dirNormalized, dist))
			{
				if (dist < closestDistance)
				{
					closestDistance = dist;
					collidedObject = collider->m_owner;
				}
			}
		}
		for (const auto& [obb, transformedOBB] : collider->m_boundingOrientedBoxes)
		{
			float dist = 0.0f;
			if (transformedOBB.Intersects(origin, dirNormalized, dist))
			{
				if (dist < closestDistance)
				{
					closestDistance = dist;
					collidedObject = collider->m_owner;
				}
			}
		}
		for (const auto& [frustum, transformedFrustum] : collider->m_boundingFrustums)
		{
			float dist = 0.0f;
			if (transformedFrustum.Intersects(origin, dirNormalized, dist))
			{
				if (dist < closestDistance)
				{
					closestDistance = dist;
					collidedObject = collider->m_owner;
				}
			}
		}
	}

	distance = closestDistance;
	return collidedObject;
}

vector<GameObjectBase*> ColliderComponent::CheckCollision(const BoundingFrustum& frustum)
{
	vector<GameObjectBase*> collidedObjects = {};

	for (ColliderComponent* collider : s_colliders)
	{
		bool isCollided = false;
		for (const auto& [box, transformedBox] : collider->m_boundingBoxes)
		{
			if (frustum.Intersects(transformedBox))
			{
				isCollided = true;
				break;
			}
		}
		if (isCollided)
		{
			collidedObjects.push_back(collider->m_owner);
			continue;
		}

		for (const auto& [obb, transformedOBB] : collider->m_boundingOrientedBoxes)
		{
			if (frustum.Intersects(transformedOBB))
			{
				isCollided = true;
				break;
			}
		}
		if (isCollided)
		{
			collidedObjects.push_back(collider->m_owner);
			continue;
		}

		for (const auto& [frustum, transformedFrustum] : collider->m_boundingFrustums)
		{
			if (frustum.Intersects(transformedFrustum))
			{
				isCollided = true;
				break;
			}
		}
		if (isCollided)
		{
			collidedObjects.push_back(collider->m_owner);
			continue;
		}
	}

	return collidedObjects;
}

void ColliderComponent::Initialize()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();

	#ifdef _DEBUG
	m_boundingShapeVertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout("VSLine.hlsl");
	m_boundingShapePixelShader = resourceManager.GetPixelShader("PSColor.hlsl");
	#endif

	s_colliders.push_back(this);
}

void ColliderComponent::FixedUpdate()
{
	const XMMATRIX& worldMatrix = m_owner->GetWorldMatrix();
	for (auto& [box, transformedBox] : m_boundingBoxes) box.Transform(transformedBox, worldMatrix);
	for (auto& [obb, transformedOBB] : m_boundingOrientedBoxes) obb.Transform(transformedOBB, worldMatrix);
	for (auto& [frustum, transformedFrustum] : m_boundingFrustums) frustum.Transform(transformedFrustum, worldMatrix);
}

void ColliderComponent::Update()
{
	const XMMATRIX& worldMatrix = m_owner->GetWorldMatrix();
	for (auto& [box, transformedBox] : m_boundingBoxes) box.Transform(transformedBox, worldMatrix);
	for (auto& [obb, transformedOBB] : m_boundingOrientedBoxes) obb.Transform(transformedOBB, worldMatrix);
	for (auto& [frustum, transformedFrustum] : m_boundingFrustums) frustum.Transform(transformedFrustum, worldMatrix);
}

#ifdef _DEBUG
void ColliderComponent::Render()
{
	Renderer& renderer = Renderer::GetInstance();

	renderer.RENDER_FUNCTION(RenderStage::Scene, BlendState::Opaque).emplace_back
	(
		0.0f,
		[&]()
		{
			ResourceManager& resourceManager = ResourceManager::GetInstance();
			com_ptr<ID3D11DeviceContext> deviceContext = renderer.GetDeviceContext();

			deviceContext->IASetInputLayout(m_boundingShapeVertexShaderAndInputLayout.second.Get());
			deviceContext->VSSetShader(m_boundingShapeVertexShaderAndInputLayout.first.Get(), nullptr, 0);
			deviceContext->PSSetShader(m_boundingShapePixelShader.Get(), nullptr, 0);

			resourceManager.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

			BoundingBox transformedBox = {};
			BoundingOrientedBox transformedOBB = {};
			BoundingFrustum transformedFrustum = {};

			array<XMFLOAT3, 8> boxVertices = {};

			LineBuffer lineBufferData = {};

			for (const auto& [box, transformedBox] : m_boundingBoxes)
			{
				transformedBox.GetCorners(boxVertices.data());

				for (const auto& [startIndex, endIndex] : BOX_LINE_INDICES)
				{
					lineBufferData.linePoints[0] = XMFLOAT4{ boxVertices[startIndex].x, boxVertices[startIndex].y, boxVertices[startIndex].z, 1.0f };
					lineBufferData.linePoints[1] = XMFLOAT4{ boxVertices[endIndex].x, boxVertices[endIndex].y, boxVertices[endIndex].z, 1.0f };
					lineBufferData.lineColors[0] = XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f };
					lineBufferData.lineColors[1] = XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f };
					deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
					deviceContext->Draw(2, 0);
				}
			}
			for (const auto& [obb, transformedOBB] : m_boundingOrientedBoxes)
			{
				transformedOBB.GetCorners(boxVertices.data());

				for (const auto& [startIndex, endIndex] : BOX_LINE_INDICES)
				{
					lineBufferData.linePoints[0] = XMFLOAT4{ boxVertices[startIndex].x, boxVertices[startIndex].y, boxVertices[startIndex].z, 1.0f };
					lineBufferData.linePoints[1] = XMFLOAT4{ boxVertices[endIndex].x, boxVertices[endIndex].y, boxVertices[endIndex].z, 1.0f };
					lineBufferData.lineColors[0] = XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f };
					lineBufferData.lineColors[1] = XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f };
					deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
					deviceContext->Draw(2, 0);
				}
			}
			for (const auto& [frustum, transformedFrustum] : m_boundingFrustums)
			{
				transformedFrustum.GetCorners(boxVertices.data());

				for (const auto& [startIndex, endIndex] : BOX_LINE_INDICES)
				{
					lineBufferData.linePoints[0] = XMFLOAT4{ boxVertices[startIndex].x, boxVertices[startIndex].y, boxVertices[startIndex].z, 1.0f };
					lineBufferData.linePoints[1] = XMFLOAT4{ boxVertices[endIndex].x, boxVertices[endIndex].y, boxVertices[endIndex].z, 1.0f };
					lineBufferData.lineColors[0] = XMFLOAT4{ 0.0f, 0.0f, 1.0f, 1.0f };
					lineBufferData.lineColors[1] = XMFLOAT4{ 0.0f, 0.0f, 1.0f, 1.0f };
					deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
					deviceContext->Draw(2, 0);
				}
			}
		}
	);
}
#endif

void ColliderComponent::RenderImGui()
{
	if ((ImGui::TreeNode("Bounding Boxes")))
	{
		for (auto& [box, transformedBox] : m_boundingBoxes)
		{
			ImGui::PushID(&box);

			ImGui::DragFloat3("Center", &box.Center.x, 0.1f);
			ImGui::DragFloat3("Extents", &box.Extents.x, 0.1f);

			ImGui::PopID();
		}
		if (ImGui::Button("Add Bounding Box")) m_boundingBoxes.emplace_back();

		ImGui::TreePop();
	}
	if ((ImGui::TreeNode("Bounding Oriented Boxes")))
	{
		for (auto& [obb, transformedOBB] : m_boundingOrientedBoxes)
		{
			ImGui::PushID(&obb);

			ImGui::DragFloat3("Center", &obb.Center.x, 0.1f);
			ImGui::DragFloat3("Extents", &obb.Extents.x, 0.1f);

			XMVECTOR eulerAngles = ToDegrees(static_cast<XMVECTOR>(static_cast<SimpleMath::Quaternion>(obb.Orientation).ToEuler()));
			if (ImGui::DragFloat3("Rotation (Degrees)", eulerAngles.m128_f32, 0.1f))
			{
				XMVECTOR radians = ToRadians(eulerAngles);
				SimpleMath::Quaternion quaternion = SimpleMath::Quaternion::CreateFromYawPitchRoll(XMVectorGetY(radians), XMVectorGetX(radians), XMVectorGetZ(radians));
				obb.Orientation = static_cast<XMFLOAT4>(quaternion);
			}

			ImGui::PopID();
		}
		if (ImGui::Button("Add Bounding Oriented Box")) m_boundingOrientedBoxes.emplace_back();

		ImGui::TreePop();
	}
	if ((ImGui::TreeNode("Bounding Frustums")))
	{
		for (auto& [frustum, transformedFrustum] : m_boundingFrustums)
		{
			ImGui::PushID(&frustum);

			ImGui::DragFloat3("Origin", &frustum.Origin.x, 0.1f);
			ImGui::DragFloat4("Orientation", &frustum.Orientation.x, 0.1f);
			ImGui::DragFloat("RightSlope", &frustum.RightSlope, 0.1f);
			ImGui::DragFloat("LeftSlope", &frustum.LeftSlope, 0.1f);
			ImGui::DragFloat("TopSlope", &frustum.TopSlope, 0.1f);
			ImGui::DragFloat("BottomSlope", &frustum.BottomSlope, 0.1f);
			ImGui::DragFloat("Near", &frustum.Near, 0.1f);
			ImGui::DragFloat("Far", &frustum.Far, 0.1f);

			ImGui::PopID();
		}
		if (ImGui::Button("Add Bounding Frustum")) m_boundingFrustums.emplace_back();

		ImGui::TreePop();
	}
}

void ColliderComponent::Finalize()
{
	auto it = find(s_colliders.begin(), s_colliders.end(), this);
	if (it != s_colliders.end()) s_colliders.erase(it);
}

nlohmann::json ColliderComponent::Serialize()
{
	nlohmann::json jsonData;

	jsonData["boundingBoxes"] = nlohmann::json::array();
	for (const auto& [box, transformedBox] : m_boundingBoxes)
	{
		nlohmann::json boxData;
		boxData["center"] = { box.Center.x, box.Center.y, box.Center.z };
		boxData["extents"] = { box.Extents.x, box.Extents.y, box.Extents.z };
		jsonData["boundingBoxes"].push_back(boxData);
	}
	jsonData["boundingOrientedBoxes"] = nlohmann::json::array();
	for (const auto& [obb, transformedOBB] : m_boundingOrientedBoxes)
	{
		nlohmann::json obbData;
		obbData["center"] = { obb.Center.x, obb.Center.y, obb.Center.z };
		obbData["extents"] = { obb.Extents.x, obb.Extents.y, obb.Extents.z };
		obbData["orientation"] = { obb.Orientation.x, obb.Orientation.y, obb.Orientation.z, obb.Orientation.w };
		jsonData["boundingOrientedBoxes"].push_back(obbData);
	}
	jsonData["boundingFrustums"] = nlohmann::json::array();
	for (const auto& [frustum, transformedFrustum] : m_boundingFrustums)
	{
		nlohmann::json frustumData;
		frustumData["origin"] = { frustum.Origin.x, frustum.Origin.y, frustum.Origin.z };
		frustumData["orientation"] = { frustum.Orientation.x, frustum.Orientation.y, frustum.Orientation.z, frustum.Orientation.w };
		frustumData["rightSlope"] = frustum.RightSlope;
		frustumData["leftSlope"] = frustum.LeftSlope;
		frustumData["topSlope"] = frustum.TopSlope;
		frustumData["bottomSlope"] = frustum.BottomSlope;
		frustumData["near"] = frustum.Near;
		frustumData["far"] = frustum.Far;
		jsonData["boundingFrustums"].push_back(frustumData);
	}

	return jsonData;
}

void ColliderComponent::Deserialize(const nlohmann::json& jsonData)
{
	m_boundingBoxes.clear();
	for (const auto& boxData : jsonData["boundingBoxes"])
	{
		BoundingBox box;
		box.Center = XMFLOAT3{ boxData["center"][0], boxData["center"][1], boxData["center"][2] };
		box.Extents = XMFLOAT3{ boxData["extents"][0], boxData["extents"][1], boxData["extents"][2] };
		AddBoundingBox(box);
	}
	m_boundingOrientedBoxes.clear();
	for (const auto& obbData : jsonData["boundingOrientedBoxes"])
	{
		BoundingOrientedBox obb;
		obb.Center = XMFLOAT3{ obbData["center"][0], obbData["center"][1], obbData["center"][2] };
		obb.Extents = XMFLOAT3{ obbData["extents"][0], obbData["extents"][1], obbData["extents"][2] };
		obb.Orientation = XMFLOAT4{ obbData["orientation"][0], obbData["orientation"][1], obbData["orientation"][2], obbData["orientation"][3] };
		AddBoundingOrientedBox(obb);
	}
	m_boundingFrustums.clear();
	for (const auto& frustumData : jsonData["boundingFrustums"])
	{
		BoundingFrustum frustum;
		frustum.Origin = XMFLOAT3{ frustumData["origin"][0], frustumData["origin"][1], frustumData["origin"][2] };
		frustum.Orientation = XMFLOAT4{ frustumData["orientation"][0], frustumData["orientation"][1], frustumData["orientation"][2], frustumData["orientation"][3] };
		frustum.RightSlope = frustumData["rightSlope"];
		frustum.LeftSlope = frustumData["leftSlope"];
		frustum.TopSlope = frustumData["topSlope"];
		frustum.BottomSlope = frustumData["bottomSlope"];
		frustum.Near = frustumData["near"];
		frustum.Far = frustumData["far"];
		AddBoundingFrustum(frustum);
	}
}