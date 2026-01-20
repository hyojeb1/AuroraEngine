///SceneBase.cpp의 시작
#include "stdafx.h"
#include "SceneBase.h"

#include "CameraComponent.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "TimeManager.h"

#ifdef _DEBUG
#include "InputManager.h"
#endif

using namespace std;
using namespace DirectX;

SceneBase::SceneBase()
{
	m_deviceContext = Renderer::GetInstance().GetDeviceContext();
}

GameObjectBase* SceneBase::CreateRootGameObject(const string& typeName)
{
	unique_ptr<GameObjectBase> gameObject = TypeRegistry::GetInstance().CreateGameObject(typeName);
	GameObjectBase* gameObjectPtr = gameObject.get();

	static_cast<Base*>(gameObjectPtr)->BaseInitialize();
	m_gameObjects.push_back(move(gameObject));

	return gameObjectPtr;
}

GameObjectBase* SceneBase::GetRootGameObject(const string& name)
{
	for (unique_ptr<Base>& gameObject : m_gameObjects)
	{
		GameObjectBase* gameObjectBase = static_cast<GameObjectBase*>(gameObject.get());
		if (gameObjectBase && gameObjectBase->GetName() == name) return gameObjectBase;
	}

	return nullptr;
}

void SceneBase::BaseInitialize()
{
	m_type = GetTypeName(*this);

	#ifdef _DEBUG
	m_debugCamera = make_unique<DebugCamera>();
	static_cast<Base*>(m_debugCamera.get())->BaseInitialize();
	m_debugCamera->Initialize();
	static_cast<GameObjectBase*>(m_debugCamera.get())->CreateComponent<CameraComponent>()->SetAsMainCamera();
	#endif

	// 저장된 씬 파일 불러오기
	const filesystem::path sceneFilePath = "../Asset/Scene/" + m_type + ".json";
	if (filesystem::exists(sceneFilePath))
	{
		ifstream file(sceneFilePath);
		nlohmann::json sceneData;
		file >> sceneData;
		file.close();
		BaseDeserialize(sceneData);
	}

	GetResources();

	#ifdef NDEBUG
	Initialize();
	#endif
}

void SceneBase::BaseFixedUpdate()
{
	for (unique_ptr<Base>& gameObject : m_gameObjects) gameObject->BaseFixedUpdate();
}

void SceneBase::BaseUpdate()
{
	#ifdef _DEBUG
	m_debugCamera->Update();
	static_cast<Base*>(m_debugCamera.get())->BaseUpdate();
	#else
	Update();
	#endif

	RemovePending();
	// 게임 오브젝트 업데이트
	for (unique_ptr<Base>& gameObject : m_gameObjects) gameObject->BaseUpdate();

	#ifdef _DEBUG
	// Ctrl + S 입력 시 씬 저장
	InputManager& inputManager = InputManager::GetInstance();
	if (inputManager.GetKey(KeyCode::Control) && inputManager.GetKeyDown(KeyCode::S))
	{
		cout << "씬: " << m_type << " 저장 중..." << endl;

		const filesystem::path sceneFilePath = "../Asset/Scene/" + m_type + ".json";

		ofstream file(sceneFilePath);
		file << BaseSerialize().dump(4);
		file.close();

		cout << "씬: " << m_type << " 저장 완료!" << endl;
	}
	#endif
}

void SceneBase::BaseRender()
{
	// 방향성 광원 그림자 맵 렌더링
	Renderer::GetInstance().RENDER_FUNCTION(RenderStage::DirectionalLightShadow, BlendState::Opaque).emplace_back
	(
		numeric_limits<float>::lowest(), // 우선순위 가장 높음
		[&]()
		{
			Renderer& renderer = Renderer::GetInstance();
			const CameraComponent& mainCamera = CameraComponent::GetMainCamera();

			renderer.SetViewport(static_cast<FLOAT>(DIRECTIAL_LIGHT_SHADOW_MAP_SIZE), static_cast<FLOAT>(DIRECTIAL_LIGHT_SHADOW_MAP_SIZE));

			// 뷰-투영 상수 버퍼 방향광 기준으로 업데이트
			const float cameraFarPlane = mainCamera.GetFarZ() * 0.1f;

			XMVECTOR lightPosition = (m_globalLightData.lightDirection * -cameraFarPlane) + mainCamera.GetPosition();
			lightPosition = XMVectorSetW(lightPosition, 1.0f);
			renderer.SetRenderSortPoint(lightPosition); // 정렬 기준점도 라이트 위치로 설정

			constexpr XMVECTOR LIGHT_UP = { 0.0f, 1.0f, 0.0f, 0.0f };
			m_viewProjectionData.viewMatrix = XMMatrixLookAtLH(lightPosition, mainCamera.GetPosition(), LIGHT_UP);

			const float lightRange = cameraFarPlane * 2.0f;
			m_viewProjectionData.projectionMatrix = XMMatrixOrthographicLH(lightRange, lightRange, 0.1f, lightRange);

			m_viewProjectionData.VPMatrix = XMMatrixTranspose(m_viewProjectionData.viewMatrix * m_viewProjectionData.projectionMatrix);
			m_globalLightData.lightViewProjectionMatrix = m_viewProjectionData.VPMatrix;

			m_deviceContext->UpdateSubresource(m_viewProjectionConstantBuffer.Get(), 0, nullptr, &m_viewProjectionData, 0, 0);

			// 상수 버퍼 설정
			m_deviceContext->PSSetShader(m_shadowMapPixelShader.Get(), nullptr, 0);
		}
	);

	// 씬 렌더링
	Renderer::GetInstance().RENDER_FUNCTION(RenderStage::Scene, BlendState::Opaque).emplace_back
	(
		numeric_limits<float>::lowest(), // 우선순위 가장 높음
		[&]()
		{
			Renderer& renderer = Renderer::GetInstance();
			const CameraComponent& mainCamera = CameraComponent::GetMainCamera();

			renderer.SetViewport();

			// 정렬 기준점 카메라 위치로 설정
			renderer.SetRenderSortPoint(mainCamera.GetPosition());

			// 상수 버퍼 업데이트 및 셰이더에 설정
			UpdateConstantBuffers();

			// 환경 맵 설정
			m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::Environment), 1, m_environmentMapSRV.GetAddressOf());
			// 스카이박스 렌더링
			RenderSkybox();

			#ifdef _DEBUG
			// 디버그 좌표축 렌더링 (디버그 모드에서만)
			if (m_isRenderDebugCoordinates) RenderDebugCoordinates();
			#else
			Render();
			#endif
		}
	);

	// 게임 오브젝트 렌더링
	for (unique_ptr<Base>& gameObject : m_gameObjects) gameObject->BaseRender();
}

void SceneBase::BaseRenderImGui()
{
	#ifdef _DEBUG
	ImGui::Begin("Debug Camera");
	static_cast<Base*>(m_debugCamera.get())->BaseRenderImGui();
	ImGui::End();
	#endif

	ImGui::Begin(m_type.c_str());

	#ifdef _DEBUG
	ImGui::Checkbox("Debug Coordinates", &m_isRenderDebugCoordinates);
	#endif

	ImGui::ColorEdit3("Light Color", &m_globalLightData.lightColor.x);
	ImGui::DragFloat("IBL Intensity", &m_globalLightData.lightColor.w, 0.001f, 0.0f, 1.0f);

	if (ImGui::DragFloat3("Light Direction", &m_globalLightData.lightDirection.m128_f32[0], 0.001f, -1.0f, 1.0f))
	{
		m_globalLightData.lightDirection = XMVector3Normalize(m_globalLightData.lightDirection);
	}
	ImGui::DragFloat("Directional Intensity", &m_globalLightData.lightDirection.m128_f32[3], 0.001f, 0.0f, 1.0f);

	RenderImGui();

	ImGui::Separator();
	ImGui::Text("Game Objects:");
	for (unique_ptr<Base>& gameObject : m_gameObjects) gameObject->BaseRenderImGui();

	ImGui::Separator();
	if (ImGui::Button("Add GameObject")) ImGui::OpenPopup("Select GameObject Type");
	if (ImGui::BeginPopup("Select GameObject Type"))
	{
		for (const auto& [typeName, factory] : TypeRegistry::GetInstance().m_gameObjectRegistry)
		{
			if (ImGui::Selectable(typeName.c_str()))
			{
				CreateRootGameObject(typeName);
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndPopup();
	}

	ImGui::End();
}

void SceneBase::BaseFinalize()
{
	#ifdef NDEBUG
	Finalize();
	#endif

	// 게임 오브젝트 종료
	for (unique_ptr<Base>& gameObject : m_gameObjects) gameObject->BaseFinalize();
}

nlohmann::json SceneBase::BaseSerialize()
{
	nlohmann::json jsonData;

	// 기본 씬 데이터 저장

	// 씬 조명 정보
	jsonData["lightColor"] =
	{
		m_globalLightData.lightColor.x,
		m_globalLightData.lightColor.y,
		m_globalLightData.lightColor.z,
		m_globalLightData.lightColor.w
	};
	jsonData["lightDirection"] =
	{
		m_globalLightData.lightDirection.m128_f32[0],
		m_globalLightData.lightDirection.m128_f32[1],
		m_globalLightData.lightDirection.m128_f32[2],
		m_globalLightData.lightDirection.m128_f32[3]
	};

	jsonData["environmentMapFileName"] = m_environmentMapFileName;

	// 파생 클래스의 직렬화 호출
	nlohmann::json derivedData = Serialize();
	if (!derivedData.is_null() && derivedData.is_object()) jsonData.merge_patch(derivedData);

	// 게임 오브젝트들 저장
	nlohmann::json gameObjectsData = nlohmann::json::array();
	for (unique_ptr<Base>& gameObject : m_gameObjects) gameObjectsData.push_back(gameObject->BaseSerialize());
	jsonData["rootGameObjects"] = gameObjectsData;

	return jsonData;
}

void SceneBase::BaseDeserialize(const nlohmann::json& jsonData)
{
	// 기본 씬 데이터 로드

	// 씬 조명 정보
	m_globalLightData.lightColor = XMFLOAT4
	(
		jsonData["lightColor"][0].get<float>(),
		jsonData["lightColor"][1].get<float>(),
		jsonData["lightColor"][2].get<float>(),
		jsonData["lightColor"][3].get<float>()
	);
	m_globalLightData.lightDirection = XMVectorSet
	(
		jsonData["lightDirection"][0].get<float>(),
		jsonData["lightDirection"][1].get<float>(),
		jsonData["lightDirection"][2].get<float>(),
		jsonData["lightDirection"][3].get<float>()
	);

	// 환경 맵 파일 이름
	m_environmentMapFileName = jsonData["environmentMapFileName"].get<string>();

	// 파생 클래스의 데이터 로드
	Deserialize(jsonData);

	// 게임 오브젝트들 로드
	for (const auto& gameObjectData : jsonData["rootGameObjects"])
	{
		string typeName = gameObjectData["type"].get<string>();
		unique_ptr<Base> gameObjectPtr = TypeRegistry::GetInstance().CreateGameObject(typeName);

		gameObjectPtr->BaseDeserialize(gameObjectData);
		gameObjectPtr->BaseInitialize();

		m_gameObjects.push_back(move(gameObjectPtr));
	}
}

void SceneBase::RemovePending()
{
	erase_if
	(
		m_gameObjects, [](const unique_ptr<Base>& gameObject)
		{
			if (!gameObject->GetAlive())
			{
				gameObject->BaseFinalize();
				return true;
			}
			return false;
		}
	);
}

void SceneBase::GetResources()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();

	m_skyboxVertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout("VSSkybox.hlsl"); // 스카이박스 정점 셰이더 얻기
	m_skyboxPixelShader = resourceManager.GetPixelShader("PSSkybox.hlsl"); // 스카이박스 픽셀 셰이더 얻기
	m_shadowMapPixelShader = resourceManager.GetPixelShader("PSShadow.hlsl"); // 그림자 맵 생성용 픽셀 셰이더 얻기

	#ifdef _DEBUG
	m_debugCoordinateVertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout("VSCoordinateLine.hlsl"); // 디버그 좌표 정점 셰이더 얻기
	m_debugCoordinatePixelShader = resourceManager.GetPixelShader("PSColor.hlsl"); // 디버그 좌표 픽셀 셰이더 얻기
	#endif

	m_environmentMapSRV = resourceManager.GetTexture(m_environmentMapFileName); // 환경 맵 로드

	m_viewProjectionConstantBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::ViewProjection); // 뷰-투영 상수 버퍼 생성
	m_skyboxViewProjectionConstantBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::SkyboxViewProjection); // 스카이박스 뷰-투영 역행렬 상수 버퍼 생성
	m_timeConstantBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::Time); // 시간 상수 버퍼 생성

	m_cameraPositionConstantBuffer = resourceManager.GetConstantBuffer(PSConstBuffers::CameraPosition); // 카메라 위치 상수 버퍼 생성
	m_globalLightConstantBuffer = resourceManager.GetConstantBuffer(PSConstBuffers::GlobalLight); // 방향광 상수 버퍼 생성
}

void SceneBase::UpdateConstantBuffers()
{
	const CameraComponent& mainCamera = CameraComponent::GetMainCamera();

	// 뷰-투영 상수 버퍼 업데이트 및 셰이더에 설정
	m_viewProjectionData.viewMatrix = mainCamera.GetViewMatrix();
	m_viewProjectionData.projectionMatrix = mainCamera.GetProjectionMatrix();
	m_viewProjectionData.VPMatrix = XMMatrixTranspose(m_viewProjectionData.viewMatrix * m_viewProjectionData.projectionMatrix);
	m_deviceContext->UpdateSubresource(m_viewProjectionConstantBuffer.Get(), 0, nullptr, &m_viewProjectionData, 0, 0);

	// 스카이박스 뷰-투영 역행렬 상수 버퍼 업데이트 및 셰이더에 설정 // m_viewProjectionData 재활용
	m_viewProjectionData.viewMatrix.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f); // 뷰 행렬의 위치 성분 제거
	m_skyboxViewProjectionData.skyboxVPMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, m_viewProjectionData.viewMatrix * m_viewProjectionData.projectionMatrix));
	m_deviceContext->UpdateSubresource(m_skyboxViewProjectionConstantBuffer.Get(), 0, nullptr, &m_skyboxViewProjectionData, 0, 0);

	// 시간 상수 버퍼 업데이트 및 셰이더에 설정
	m_timeData.totalTime = TimeManager::GetInstance().GetTotalTime();
	m_timeData.deltaTime = TimeManager::GetInstance().GetDeltaTime();
	m_timeData.sinTime = sinf(m_timeData.totalTime);
	m_timeData.cosTime = cosf(m_timeData.totalTime);
	m_deviceContext->UpdateSubresource(m_timeConstantBuffer.Get(), 0, nullptr, &m_timeData, 0, 0);

	// 카메라 위치 상수 버퍼 업데이트 및 셰이더에 설정
	m_cameraPositionData.cameraPosition = mainCamera.GetPosition();
	m_deviceContext->UpdateSubresource(m_cameraPositionConstantBuffer.Get(), 0, nullptr, &m_cameraPositionData, 0, 0);

	// 환경광, 방향광 상수 버퍼 업데이트 및 셰이더에 설정
	m_deviceContext->UpdateSubresource(m_globalLightConstantBuffer.Get(), 0, nullptr, &m_globalLightData, 0, 0);
}

void SceneBase::RenderSkybox()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();

	resourceManager.SetDepthStencilState(DepthStencilState::Skybox);

	resourceManager.SetBlendState(BlendState::Opaque);
	resourceManager.SetRasterState(RasterState::Solid);
	resourceManager.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_deviceContext->IASetInputLayout(m_skyboxVertexShaderAndInputLayout.second.Get());
	m_deviceContext->VSSetShader(m_skyboxVertexShaderAndInputLayout.first.Get(), nullptr, 0);
	m_deviceContext->PSSetShader(m_skyboxPixelShader.Get(), nullptr, 0);

	constexpr UINT STRIDE = 0;
	constexpr UINT OFFSET = 0;
	constexpr ID3D11Buffer* nullBuffer = nullptr;
	m_deviceContext->IASetVertexBuffers(0, 1, &nullBuffer, &STRIDE, &OFFSET);

	m_deviceContext->Draw(3, 0);

	resourceManager.SetDepthStencilState(DepthStencilState::Default);
}

#ifdef _DEBUG
void SceneBase::RenderDebugCoordinates()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();

	resourceManager.SetRasterState(RasterState::Solid);
	resourceManager.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	m_deviceContext->IASetInputLayout(m_debugCoordinateVertexShaderAndInputLayout.second.Get());
	m_deviceContext->VSSetShader(m_debugCoordinateVertexShaderAndInputLayout.first.Get(), nullptr, 0);
	m_deviceContext->PSSetShader(m_debugCoordinatePixelShader.Get(), nullptr, 0);

	constexpr UINT STRIDE = 0;
	constexpr UINT OFFSET = 0;
	constexpr ID3D11Buffer* nullBuffer = nullptr;
	m_deviceContext->IASetVertexBuffers(0, 1, &nullBuffer, &STRIDE, &OFFSET);

	m_deviceContext->DrawInstanced(2, 204, 0, 0);
}
#endif