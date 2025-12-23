#include "stdafx.h"
#include "SceneBase.h"

#include "CameraComponent.h"
#include "Renderer.h"
#include "ResourceManager.h"

using namespace std;
using namespace DirectX;

void SceneBase::Initialize()
{
	m_typeName = typeid(*this).name();
	if (m_typeName.find("class ") == 0) m_typeName = m_typeName.substr(6);

	m_mainCamera = CreateCameraObject()->CreateComponent<CameraComponent>();

	m_viewProjectionConstantBuffer = ResourceManager::GetInstance().GetConstantBuffer(sizeof(ViewProjectionBuffer));
	m_directionalLightConstantBuffer = ResourceManager::GetInstance().GetConstantBuffer(sizeof(DirectionalLightBuffer));

	InitializeScene();
}

void SceneBase::Update(float deltaTime)
{
	RemovePendingGameObjects();
	for (unique_ptr<GameObjectBase>& gameObject : m_gameObjects) gameObject->Update(deltaTime);
}

void SceneBase::Render()
{
	Renderer& renderer = Renderer::GetInstance();
	renderer.BeginFrame(m_clearColor);

	// 뷰-투영 상수 버퍼 업데이트 및 셰이더에 설정
	m_viewProjectionData.viewMatrix = XMMatrixTranspose(m_mainCamera->GetViewMatrix());
	m_viewProjectionData.projectionMatrix = XMMatrixTranspose(m_mainCamera->GetProjectionMatrix());
	m_viewProjectionData.VPMatrix = m_viewProjectionData.projectionMatrix * m_viewProjectionData.viewMatrix;

	com_ptr<ID3D11DeviceContext> deviceContext = renderer.GetDeviceContext();
	deviceContext->UpdateSubresource(m_viewProjectionConstantBuffer.Get(), 0, nullptr, &m_viewProjectionData, 0, 0);
	deviceContext->VSSetConstantBuffers(static_cast<UINT>(VSConstBuffers::ViewProjection), 1, m_viewProjectionConstantBuffer.GetAddressOf());

	// 방향광 상수 버퍼 업데이트 및 셰이더에 설정
	m_directionalLightData.lightDirection = DirectionalLightDirection;
	m_directionalLightData.lightColor = m_clearColor;
	deviceContext->UpdateSubresource(m_directionalLightConstantBuffer.Get(), 0, nullptr, &m_directionalLightData, 0, 0);
	deviceContext->PSSetConstantBuffers(static_cast<UINT>(PSConstBuffers::DirectionalLight), 1, m_directionalLightConstantBuffer.GetAddressOf());

	// 게임 오브젝트 렌더링
	for (unique_ptr<GameObjectBase>& gameObject : m_gameObjects) gameObject->Render();

	// ImGui 렌더링
	RenderImGui();

	renderer.EndFrame();
}

void SceneBase::RenderImGui()
{
	ImGui::Begin(m_typeName.c_str());

	if (ImGui::ColorEdit3("Clear Color", &m_clearColor.x)) {}

	ImGui::Separator();
	ImGui::Text("Game Objects:");
	for (unique_ptr<GameObjectBase>& gameObject : m_gameObjects) gameObject->RenderImGui();

	ImGui::End();
}

GameObjectBase* SceneBase::CreateCameraObject()
{
	GameObjectBase* cameraGameObject = CreateRootGameObject<GameObjectBase>();
	cameraGameObject->SetPosition({ 0.0f, 5.0f, -10.0f });
	cameraGameObject->LookAt({ 0.0f, 0.0f, 0.0f });

	return cameraGameObject;
}

void SceneBase::RemovePendingGameObjects()
{
	for (GameObjectBase* gameObjectToRemove : m_gameObjectsToRemove)
	{
		erase_if
		(
			m_gameObjects, [gameObjectToRemove](const unique_ptr<GameObjectBase>& obj)
			{
				if (obj.get() == gameObjectToRemove)
				{
					obj->Finalize();
					return true;
				}
				return false;
			}
		);
	}
	m_gameObjectsToRemove.clear();
}