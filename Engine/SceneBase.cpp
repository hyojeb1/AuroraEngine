///bof SceneBase.cpp
#include "stdafx.h"
#include "SceneBase.h"

#include "Renderer.h"
#include "CameraComponent.h"
#include "ResourceManager.h"
#include "TimeManager.h"
#include "NavigationManager.h"
#include "WindowManager.h"
#include "ModelComponent.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "Button.h"
#include "Slider.h"
#include "Panel.h"

using namespace std;
using namespace DirectX;

PostProcessingBuffer SceneBase::m_postProcessingData = {};

GameObjectBase* SceneBase::CreateRootGameObject(const string& typeName)
{
	unique_ptr<GameObjectBase> gameObject = TypeRegistry::GetInstance().CreateGameObject(typeName);
	GameObjectBase* gameObjectPtr = gameObject.get();

	static_cast<Base*>(gameObjectPtr)->BaseInitialize();
	m_gameObjects.push_back(move(gameObject));

	return gameObjectPtr;
}

GameObjectBase* SceneBase::CreatePrefabRootGameObject(const string& prefabFileName)
{
	return CreateFromJson(*SceneManager::GetInstance().GetPrefabData(prefabFileName));
}

GameObjectBase* SceneBase::CreateFromJson(const nlohmann::json& jsonData)
{
	unique_ptr<GameObjectBase> gameObject = TypeRegistry::GetInstance().CreateGameObject(jsonData["type"].get<string>());
	GameObjectBase* gameObjectPtr = gameObject.get();

	static_cast<Base*>(gameObjectPtr)->BaseDeserialize(jsonData);
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

GameObjectBase* SceneBase::GetGameObjectRecursive(const string& name)
{
	for (unique_ptr<Base>& gameObject : m_gameObjects)
	{
		GameObjectBase* gameObjectBase = static_cast<GameObjectBase*>(gameObject.get());
		if (gameObjectBase)
		{
			if (gameObjectBase->GetName() == name) return gameObjectBase;
			GameObjectBase* found = gameObjectBase->GetGameObjectRecursive(name);
			if (found) return found;
		}
	}
	return nullptr;
}

#ifdef _DEBUG
void SceneBase::SaveState()
{
	nlohmann::json sceneData = BaseSerialize();

	if (m_lastSavedSnapshot.is_null() || m_lastSavedSnapshot.empty()) { m_lastSavedSnapshot = sceneData; return; }

	const nlohmann::json forwardDiff = nlohmann::json::diff(m_lastSavedSnapshot, sceneData);
	if (forwardDiff.empty()) return;

	const nlohmann::json inverseDiff = nlohmann::json::diff(sceneData, m_lastSavedSnapshot);
	m_previousStateInversePatches.push_back(inverseDiff);

	if (m_previousStateInversePatches.size() > 1000) m_previousStateInversePatches.pop_front();

	m_lastSavedSnapshot = sceneData;
}

void SceneBase::Undo()
{
	if (m_previousStateInversePatches.empty()) return;

	nlohmann::json inversePatch = move(m_previousStateInversePatches.back());
	m_previousStateInversePatches.pop_back();

	nlohmann::json previousScene = BaseSerialize().patch(inversePatch);

	BaseDeserialize(previousScene);
	for (unique_ptr<Base>& gameObject : m_gameObjects) gameObject->BaseInitialize();

	m_lastSavedSnapshot = previousScene;
}
#endif
void SceneBase::OnResizeEvent()
{
	for (auto& m : m_UIList)
	{
		m->OnResize();
	}
}

void SceneBase::BaseInitialize()
{
	m_deviceContext = Renderer::GetInstance().GetDeviceContext();

	m_type = GetTypeName(*this);

	#ifdef _DEBUG
	m_debugCamera = make_unique<DebugCamera>();
	static_cast<Base*>(m_debugCamera.get())->BaseInitialize();
	m_debugCamera->Initialize();
	#endif

	const filesystem::path sceneFilePath = "../Asset/Scene/" + m_type + ".json";
	if (filesystem::exists(sceneFilePath))
	{
		ifstream file(sceneFilePath);
		nlohmann::json sceneData = {};
		file >> sceneData;
		file.close();
		BaseDeserialize(sceneData);
	}
	for (unique_ptr<Base>& gameObject : m_gameObjects) gameObject->BaseInitialize();

	GetResources();

	#ifdef _DEBUG
	SaveState();
	#else
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
	for (unique_ptr<Base>& gameObject : m_gameObjects) gameObject->BaseUpdate();

	InputManager& inputManager = InputManager::GetInstance();

	const POINT& mousePosition = inputManager.GetMousePosition();
	const bool isMousePressed = inputManager.GetKey(KeyCode::MouseLeft);
	const bool isMouseClicked = inputManager.GetKeyUp(KeyCode::MouseLeft);
	for (auto it = m_UIList.rbegin(); it != m_UIList.rend(); ++it)
	{
		if (auto* button = dynamic_cast<Button*>(it->get()))
		{
			if (button->CheckInput(mousePosition, isMousePressed, isMouseClicked))
				break;
		}

		if (auto* slider = dynamic_cast<Slider*>(it->get()))
		{
			if (slider->CheckInput(mousePosition, isMousePressed))
				break;
		}
	}


	#ifdef _DEBUG
	if (inputManager.GetKeyUp(KeyCode::MouseLeft)) SaveState();

	if (m_isNavMeshCreating) NavigationManager::GetInstance().HandlePlaceLink(m_navMeshCreationHeight);

	if (inputManager.GetKey(KeyCode::Control))
	{
		static nlohmann::json copiedObjectJson = {};
		if (inputManager.GetKeyDown(KeyCode::C))
		{
			if (GameObjectBase* selectedObject = GameObjectBase::GetSelectedObject()) copiedObjectJson = static_cast<Base*>(selectedObject)->BaseSerialize();
		}
		if (inputManager.GetKeyDown(KeyCode::V))
		{
			if (!copiedObjectJson.is_null() && !copiedObjectJson.empty())
			{
				if (GameObjectBase* selectedObject = GameObjectBase::GetSelectedObject()) selectedObject->CreateFromJson(copiedObjectJson);
				else CreateFromJson(copiedObjectJson);
			}
		}

		if (inputManager.GetKeyDown(KeyCode::S))
		{
			cout << "[System] Saving Scene: " << m_type << "..." << endl;

			const filesystem::path sceneFilePath = "../Asset/Scene/" + m_type + ".json";

			ofstream sceneFile(sceneFilePath);
			sceneFile << BaseSerialize().dump(4);
			sceneFile.close();

			cout << "[System] Scene: " << m_type << " saved successfully!" << endl;
		}

		if (inputManager.GetKeyDown(KeyCode::Z)) Undo();
		if (inputManager.GetKeyDown(KeyCode::MouseLeft)) PickObjectDebugCamera();
	}
	#endif
}

void SceneBase::BaseRender()
{
	Renderer& renderer = Renderer::GetInstance();

	renderer.RENDER_FUNCTION(RenderStage::DirectionalLightShadow, BlendState::Opaque).emplace_back
	(
		numeric_limits<float>::lowest(),
		[&]()
		{
			Renderer& renderer = Renderer::GetInstance();
			const CameraComponent& mainCamera = CameraComponent::GetMainCamera();

			renderer.SetViewport(static_cast<FLOAT>(DIRECTIAL_LIGHT_SHADOW_MAP_SIZE), static_cast<FLOAT>(DIRECTIAL_LIGHT_SHADOW_MAP_SIZE));

			const float cameraFarPlane = mainCamera.GetFarZ();

			XMVECTOR lightPosition = (m_globalLightData.lightDirection * -cameraFarPlane * 0.25f) + mainCamera.GetPosition();
			lightPosition = XMVectorSetW(lightPosition, 1.0f);
			renderer.SetRenderSortPoint(lightPosition);

			constexpr XMVECTOR LIGHT_UP = { 0.0f, 1.0f, 0.0f, 0.0f };
			m_viewProjectionData.viewMatrix = XMMatrixLookAtLH(lightPosition, mainCamera.GetPosition(), LIGHT_UP);

			const float lightRange = cameraFarPlane * 0.5f; // TODO: 나중에 조금 줄일수도 있음
			m_viewProjectionData.projectionMatrix = XMMatrixOrthographicLH(lightRange, lightRange, 0.1f, lightRange);

			m_viewProjectionData.VPMatrix = XMMatrixTranspose(m_viewProjectionData.viewMatrix * m_viewProjectionData.projectionMatrix);
			m_globalLightData.lightViewProjectionMatrix = m_viewProjectionData.VPMatrix;

			m_deviceContext->UpdateSubresource(m_viewProjectionConstantBuffer.Get(), 0, nullptr, &m_viewProjectionData, 0, 0);

			m_deviceContext->PSSetShader(m_shadowMapPixelShader.Get(), nullptr, 0);
		}
	);

	renderer.RENDER_FUNCTION(RenderStage::Scene, BlendState::Opaque).emplace_back
	(
		numeric_limits<float>::lowest(), 
		[&]()
		{
			Renderer& renderer = Renderer::GetInstance();
			const CameraComponent& mainCamera = CameraComponent::GetMainCamera();

			renderer.SetViewport();

			renderer.SetRenderSortPoint(mainCamera.GetPosition());

			UpdateConstantBuffers();

			m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::Environment), 1, m_environmentMapSRV.GetAddressOf());
			RenderSkybox();

			#ifdef _DEBUG
			if (m_isRenderDebugCoordinates) RenderDebugCoordinates();
			#endif
			Render();
		}
	);

	if (m_showFPS)
	{
		renderer.UI_RENDER_FUNCTIONS().emplace_back
		(
			[&]()
			{
				static UINT frameCount = 0;
				static float elapsedTime = 0.0f;
				static UINT FPS = 0;

				frameCount++;

				elapsedTime += TimeManager::GetInstance().GetDeltaTime();

				if (elapsedTime >= 1.0)
				{
					FPS = frameCount * static_cast<UINT>(elapsedTime);
					frameCount = 0;
					elapsedTime = 0.0;
				}

				Renderer::GetInstance().RenderTextScreenPosition((L"FPS: " + to_wstring(FPS)).c_str(), XMFLOAT2(10.0f, 10.0f));
			}
		);
	}

	for (unique_ptr<Base>& gameObject : m_gameObjects) gameObject->BaseRender();

	for (const unique_ptr<UIBase>& ui : m_UIList) ui->RenderUI(renderer);

	#ifdef _DEBUG
	if (m_isNavMeshCreating) NavigationManager::GetInstance().RenderNavMesh();
	#endif
}

#ifdef _DEBUG
void SceneBase::BaseRenderImGui()
{
	ImGui::Begin("Debug Camera");
	static_cast<Base*>(m_debugCamera.get())->BaseRenderImGui();
	ImGui::End();

	ImGui::Begin(m_type.c_str());

	ImGui::Checkbox("Debug Coordinates", &m_isRenderDebugCoordinates);

	ImGui::Checkbox("NavMesh Creating", &m_isNavMeshCreating);
	ImGui::DragFloat("NavMesh Creation Height", &m_navMeshCreationHeight, 0.1f, 0.1f, 100.0f);

	ImGui::Separator();
	array<char, 256> skyboxFileNameBuffer = {};
	strcpy_s(skyboxFileNameBuffer.data(), skyboxFileNameBuffer.size(), m_environmentMapFileName.c_str());

	if (ImGui::InputText("Skybox File Name", skyboxFileNameBuffer.data(), sizeof(skyboxFileNameBuffer))) m_environmentMapFileName = skyboxFileNameBuffer.data();
	if (ImGui::Button("Load Skybox")) m_environmentMapSRV = ResourceManager::GetInstance().GetTexture(m_environmentMapFileName);

	ImGui::Separator();
	ImGui::Text("[Post Processing]");
	
	ImGui::Separator();

	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Bloom");
	ImGui::CheckboxFlags("Bloom", &m_postProcessingData.flags, static_cast<UINT>(PostProcessingBuffer::PostProcessingFlag::Bloom));
	ImGui::DragFloat("Bloom Intensity", &m_postProcessingData.bloomIntensity, 0.1f, 0.0f, 100.0f);

	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Gamma");
	ImGui::CheckboxFlags("Gamma", &m_postProcessingData.flags, static_cast<UINT>(PostProcessingBuffer::PostProcessingFlag::Gamma));
	ImGui::DragFloat("Gamma Intensity", &m_postProcessingData.gammaIntensity, 0.01f, 0.1f, 5.0f);

	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Grayscale");
	ImGui::CheckboxFlags("Grayscale", &m_postProcessingData.flags, static_cast<UINT>(PostProcessingBuffer::PostProcessingFlag::Grayscale));
	ImGui::DragFloat("Grayscale Intensity", &m_postProcessingData.grayScaleIntensity, 0.01f, 0.0f, 1.0f);

	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Vignetting");
	ImGui::CheckboxFlags("Vignetting", &m_postProcessingData.flags, static_cast<UINT>(PostProcessingBuffer::PostProcessingFlag::Vignetting));
	ImGui::ColorEdit4("Vignetting Color And Intensity", &m_postProcessingData.vignettingColor.x);
	
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Radial Blur");
	ImGui::CheckboxFlags("Radial Blur", &m_postProcessingData.flags, static_cast<UINT>(PostProcessingBuffer::PostProcessingFlag::RadialBlur));
	ImGui::DragFloat2("Radial Blur Center", &m_postProcessingData.radialBlurParam.x, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Radial Blur Dist", &m_postProcessingData.radialBlurParam.z, 0.01f, 0.0f, 10.0f);
	ImGui::DragFloat("Radial Blur Strength", &m_postProcessingData.radialBlurParam.w, 0.01f, 0.0f, 100.0f);

	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "LUT");
	const char* lutItems[] = {
	#define X(name) #name,
	LUT_LIST
	#undef X
	};
	int& lut1idx = Renderer::GetInstance().GetSelectedLUTIndex();
	ImGui::Combo("Select LUT", &lut1idx, lutItems, LUTData::COUNT);
	com_ptr<ID3D11ShaderResourceView> lut_texture_1 = nullptr;
	lut_texture_1 = ResourceManager::GetInstance().GetLUT(lut1idx);
	ImGui::Image
	(
		(ImTextureID)lut_texture_1.Get(),
		ImVec2(256, 16)
	);
	int& lut2idx = Renderer::GetInstance().GetSelectedLUT2Index();
	ImGui::CheckboxFlags("LUT CROSSFADE", &m_postProcessingData.flags, static_cast<UINT>(PostProcessingBuffer::PostProcessingFlag::LUT_CROSSFADE));
	ImGui::Combo("Select LUT2", &lut2idx, lutItems, LUTData::COUNT);
	com_ptr<ID3D11ShaderResourceView> lut_texture_2 = nullptr;
	lut_texture_2 = ResourceManager::GetInstance().GetLUT(lut2idx);
	ImGui::Image
	(
		(ImTextureID)lut_texture_2.Get(),
		ImVec2(256, 16)
	);
	ImGui::DragFloat("Lut Lerp Factor", &m_postProcessingData.lutLerpFactor, 0.01f, 0.0f, 1.0f);

	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Light");
	ImGui::DragFloat("IBL Intensity", &m_globalLightData.lightColor.w, 0.001f, 0.0f, 1.0f);
	ImGui::ColorEdit3("Light Color", &m_globalLightData.lightColor.x);

	if (ImGui::DragFloat3("Light Direction", &m_globalLightData.lightDirection.m128_f32[0], 0.001f, -1.0f, 1.0f))
	{
		m_globalLightData.lightDirection = XMVectorSetW(XMVector3Normalize(m_globalLightData.lightDirection), m_globalLightData.lightDirection.m128_f32[3]);
	}
	ImGui::DragFloat("Directional Intensity", &m_globalLightData.lightDirection.m128_f32[3], 0.001f, 0.0f, 100.0f);

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
	if (ImGui::Button("Add Prefab")) ImGui::OpenPopup("Select Prefab");
	if (ImGui::BeginPopup("Select Prefab"))
	{
		const filesystem::path prefabDirectory = "../Asset/Prefab/";
		for (const auto& entry : filesystem::directory_iterator(prefabDirectory))
		{
			if (entry.path().extension() == ".json")
			{
				const string prefabFileName = entry.path().stem().string();
				if (ImGui::Selectable(prefabFileName.c_str()))
				{
					CreatePrefabRootGameObject(prefabFileName + ".json");
					ImGui::CloseCurrentPopup();
				}
			}
		}
		ImGui::EndPopup();
	}

	ImGui::End();

	GameObjectBase* selectedObject = GameObjectBase::GetSelectedObject();
	if (selectedObject)
	{
		static ImGuizmo::OPERATION gizmoOperation = ImGuizmo::TRANSLATE;
		static ImGuizmo::MODE gizmoMode = ImGuizmo::LOCAL;
		static bool useSnap = false;
		array<float, 3> snapTranslation = { 0.5f, 0.5f, 0.5f };
		static float snapRotation = 15.0f;
		static float snapScale = 0.1f;

		ImGui::Begin("Gizmo");
		ImGui::Text("Selected: %s", selectedObject->GetName().c_str());
		if (ImGui::RadioButton("Translate", gizmoOperation == ImGuizmo::TRANSLATE)) gizmoOperation = ImGuizmo::TRANSLATE;
		ImGui::SameLine();
		if (ImGui::RadioButton("Rotate", gizmoOperation == ImGuizmo::ROTATE)) gizmoOperation = ImGuizmo::ROTATE;
		ImGui::SameLine();
		if (ImGui::RadioButton("Scale", gizmoOperation == ImGuizmo::SCALE)) gizmoOperation = ImGuizmo::SCALE;

		if (gizmoOperation != ImGuizmo::SCALE)
		{
			if (ImGui::RadioButton("Local", gizmoMode == ImGuizmo::LOCAL)) gizmoMode = ImGuizmo::LOCAL;
			ImGui::SameLine();
			if (ImGui::RadioButton("World", gizmoMode == ImGuizmo::WORLD)) gizmoMode = ImGuizmo::WORLD;
		}

		ImGui::Checkbox("Snap", &useSnap);
		if (useSnap)
		{
			if (gizmoOperation == ImGuizmo::TRANSLATE) ImGui::InputFloat3("Snap T", snapTranslation.data());
			else if (gizmoOperation == ImGuizmo::ROTATE) ImGui::InputFloat("Snap R", &snapRotation);
			else if (gizmoOperation == ImGuizmo::SCALE) ImGui::InputFloat("Snap S", &snapScale);
		}
		ImGui::End();

		ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

		RECT rect = WindowManager::GetInstance().GetClientPosRect();
		ImGuizmo::SetRect(static_cast<float>(rect.left), static_cast<float>(rect.top), static_cast<float>(rect.right), static_cast<float>(rect.bottom));
		
		ImGuizmo::SetOrthographic(false);

		const float* snap = nullptr;
		array<float, 3> snapValues = { 0.0f, 0.0f, 0.0f };
		if (useSnap)
		{
			if (gizmoOperation == ImGuizmo::TRANSLATE)
			{
				memcpy(snapValues.data(), snapTranslation.data(), sizeof(float) * 3);
				snap = snapValues.data();
			}
			else if (gizmoOperation == ImGuizmo::ROTATE)
			{
				snapValues[0] = snapRotation;
				snap = snapValues.data();
			}
			else if (gizmoOperation == ImGuizmo::SCALE)
			{
				snapValues[0] = snapScale;
				snap = snapValues.data();
			}
		}

		const CameraComponent& mainCamera = CameraComponent::GetMainCamera();
		XMMATRIX worldMatrix = selectedObject->GetWorldMatrix();
		ImGuizmo::Manipulate
		(
			&mainCamera.GetViewMatrix().r[0].m128_f32[0],
			&mainCamera.GetProjectionMatrix().r[0].m128_f32[0],
			gizmoOperation,
			gizmoMode,
			&worldMatrix.r[0].m128_f32[0],
			nullptr,
			snap
		);

		if (ImGuizmo::IsUsing()) selectedObject->ApplyWorldMatrix(worldMatrix);
	}

	RenderImGui_UI();
}

void SceneBase::RenderImGui_UI()
{
	ImGui::Begin("UI Editor");

	
	// 1. UI 생성 버튼들 (가로 배치)
	if (ImGui::Button("Add Panel"))  CreateUI<Panel>()->SetName("New Panel");
	ImGui::SameLine();
	if (ImGui::Button("Add Button")) CreateUI<Button>()->SetName("New Button");
	ImGui::SameLine();
	if (ImGui::Button("Add Slider")) CreateUI<Slider>()->SetName("New Slider");

	ImGui::Separator();

	// 2. UI 리스트 (좌측 패널 역할) - 높이 200px 제한
	ImGui::BeginChild("UI_List", ImVec2(0, 200), true);

	// 반복문을 돌며 리스트 출력
	for (int i = 0; i < m_UIList.size(); ++i) {
		UIBase* ui = m_UIList[i].get();
		if (ui == nullptr) continue;

		// 선택된 항목 하이라이트 처리
		bool isSelected = (m_selectedUI == ui);

		// "타입: 이름" 형태로 출력 (예: "[Button] StartBtn")
		std::string label = "[" + ui->GetTypeName() + "] " + ui->GetName();

		if (ImGui::Selectable(label.c_str(), isSelected)) {
			m_selectedUI = ui; // 클릭 시 선택
		}

		// 우클릭 컨텍스트 메뉴 (삭제 기능)
		if (ImGui::BeginPopupContextItem()) {
			if (ImGui::MenuItem("Delete UI")) {
				// 삭제 처리 (벡터에서 제거)
				m_UIList.erase(m_UIList.begin() + i);
				if (m_selectedUI == ui) m_selectedUI = nullptr;
				ImGui::EndPopup();
				break; // 반복문 탈출 (Iterator 무효화 방지)
			}
			ImGui::EndPopup();
		}
	}
	ImGui::EndChild();

	// 3. 속성 편집기 (선택된 UI가 있을 때만 표시)
	if (m_selectedUI) {
		ImGui::Separator();
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "Inspector: %s", m_selectedUI->GetName().c_str());

		// --- [공통 속성 편집] ---

		// 이름 변경
		static char nameBuf[128];
		strcpy_s(nameBuf, m_selectedUI->GetName().c_str());
		if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
			m_selectedUI->SetName(nameBuf);
		}

		// 활성화 여부
		bool active = m_selectedUI->GetActive();
		if (ImGui::Checkbox("Active", &active)) {
			m_selectedUI->SetActive(active);
		}

		// 위치 (DragFloat2 사용)
		// 주의: Engine의 좌표계에 맞춰 속도(0.01f) 조절 필요
		DirectX::XMFLOAT2 pos = m_selectedUI->GetLocalPosition();
		if (ImGui::DragFloat2("Position", &pos.x, 0.001f)) { // 0.0~1.0 좌표계라면 정밀하게
			m_selectedUI->SetLocalPosition(pos);
		}

		// 스케일
		float scale = m_selectedUI->GetScale();
		if (ImGui::DragFloat("Scale", &scale, 0.01f, 0.0f, 10.0f)) {
			m_selectedUI->SetScale(scale);
		}

		// --- [타입별 특수 속성 편집] ---
		// dynamic_cast를 사용하여 타입별로 다른 UI를 보여줌

		if (auto* btn = dynamic_cast<Button*>(m_selectedUI)) {
			ImGui::Text("Button Properties");
			// 텍스처 경로 편집 (단순 텍스트 입력)
			// 실제로는 파일 다이얼로그가 좋지만, 지금은 InputText로 경로 수정
			// (SetTextureAndOffset 함수가 경로를 인자로 받는다고 가정)
			// 필요하다면 std::string 버퍼 래퍼 필요
		} else if (auto* slider = dynamic_cast<Slider*>(m_selectedUI)) {
			ImGui::Text("Slider Properties");
			// 슬라이더 범위 편집 등
			// float min = slider->GetMin(); ...
			// ImGui::DragFloat("Min", &min); ...
		}
	}
	
	ImGui::End();

}


#endif

void SceneBase::BaseFinalize()
{
	#ifdef NDEBUG
	Finalize();
	#endif

	for (unique_ptr<Base>& gameObject : m_gameObjects) gameObject->BaseFinalize();
}

nlohmann::json SceneBase::BaseSerialize()
{
	nlohmann::json jsonData;

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

	jsonData["postProcessing"] =
	{
		{ "flags", m_postProcessingData.flags },
		{ "bloomIntensity", m_postProcessingData.bloomIntensity },
		{ "gammaIntensity", m_postProcessingData.gammaIntensity },
		{ "grayScaleIntensity", m_postProcessingData.grayScaleIntensity },
		{ "vignettingColor", { m_postProcessingData.vignettingColor.x, m_postProcessingData.vignettingColor.y, m_postProcessingData.vignettingColor.z, m_postProcessingData.vignettingColor.w } },
		{ "radialBlurParam", { m_postProcessingData.radialBlurParam.x, m_postProcessingData.radialBlurParam.y, m_postProcessingData.radialBlurParam.z, m_postProcessingData.radialBlurParam.w } },
		{ "lutLerpFactor", m_postProcessingData.lutLerpFactor}
	};

	jsonData["environmentMapFileName"] = m_environmentMapFileName;

	nlohmann::json navMeshData = NavigationManager::GetInstance().Serialize();
	if (!navMeshData.is_null() && navMeshData.is_object()) jsonData.merge_patch(navMeshData);

	nlohmann::json uiArray = nlohmann::json::array();
	for (size_t i = 0; i < m_UIList.size(); ++i) {
		if (!m_UIList[i]) continue;
		nlohmann::json uiData = m_UIList[i]->Serialize();
		int parentIdx = -1;
		UIBase* parent = m_UIList[i]->GetParent();
		if (parent) {
			for (size_t j = 0; j < m_UIList.size(); ++j) {
				if (m_UIList[j].get() == parent) {
					parentIdx = static_cast<int>(j);
					break;
				}
			}
		}
		uiData["parentIdx"] = parentIdx;
		uiArray.push_back(uiData);
	}

	jsonData["UI"] = uiArray;

	nlohmann::json derivedData = Serialize();
	if (!derivedData.is_null() && derivedData.is_object()) jsonData.merge_patch(derivedData);

	nlohmann::json gameObjectsData = nlohmann::json::array();
	for (unique_ptr<Base>& gameObject : m_gameObjects) gameObjectsData.push_back(gameObject->BaseSerialize());
	jsonData["rootGameObjects"] = gameObjectsData;

	return jsonData;
}

void SceneBase::BaseDeserialize(const nlohmann::json& jsonData)
{
	if (jsonData.contains("lightColor"))
	{
		m_globalLightData.lightColor = XMFLOAT4
		(
			jsonData["lightColor"][0].get<float>(),
			jsonData["lightColor"][1].get<float>(),
			jsonData["lightColor"][2].get<float>(),
			jsonData["lightColor"][3].get<float>()
		);
	}
	if (jsonData.contains("lightDirection"))
	{
		m_globalLightData.lightDirection = XMVectorSet
		(
			jsonData["lightDirection"][0].get<float>(),
			jsonData["lightDirection"][1].get<float>(),
			jsonData["lightDirection"][2].get<float>(),
			jsonData["lightDirection"][3].get<float>()
		);
	}

	if (jsonData.contains("postProcessing"))
	{
		const nlohmann::json& ppData = jsonData["postProcessing"];
		if (ppData.contains("flags")) m_postProcessingData.flags = ppData["flags"].get<UINT>();
		if (ppData.contains("bloomIntensity")) m_postProcessingData.bloomIntensity = ppData["bloomIntensity"].get<float>();
		if (ppData.contains("gammaIntensity")) m_postProcessingData.gammaIntensity = ppData["gammaIntensity"].get<float>();
		if (ppData.contains("grayScaleIntensity")) m_postProcessingData.grayScaleIntensity = ppData["grayScaleIntensity"].get<float>();
		if (ppData.contains("vignettingColor"))
		{
			m_postProcessingData.vignettingColor = XMFLOAT4
			(
				ppData["vignettingColor"][0].get<float>(),
				ppData["vignettingColor"][1].get<float>(),
				ppData["vignettingColor"][2].get<float>(),
				ppData["vignettingColor"][3].get<float>()
			);
		}
		if (ppData.contains("radialBlurParam"))
		{
			m_postProcessingData.radialBlurParam = XMFLOAT4
			(
				ppData["radialBlurParam"][0].get<float>(),
				ppData["radialBlurParam"][1].get<float>(),
				ppData["radialBlurParam"][2].get<float>(),
				ppData["radialBlurParam"][3].get<float>()
			);
		}
		if (ppData.contains("lutLerpFactor")) m_postProcessingData.lutLerpFactor = ppData["lutLerpFactor"].get<float>();
	}

	if (jsonData.contains("environmentMapFileName")) m_environmentMapFileName = jsonData["environmentMapFileName"].get<string>();

	NavigationManager::GetInstance().Deserialize(jsonData);

	m_UIList.clear();
	if (jsonData.contains("UI")) {
		const nlohmann::json& uiArray = jsonData["UI"];
		for (const auto& uiItem : uiArray) {
			string type = uiItem["type"].get<string>();
			UIBase* newUI = UIBase::CreateFactory(type);

			if (newUI) {
				newUI->Deserialize(uiItem);
				m_UIList.push_back(unique_ptr<UIBase>(newUI));
			} else {}
		}
		for (size_t i = 0; i < uiArray.size(); ++i) {			
			if (i >= m_UIList.size()) break;
			if (m_UIList[i] == nullptr) continue;
			int parentIdx = uiArray[i].value("parentIdx", -1);
			if (parentIdx >= 0 && parentIdx < static_cast<int>(m_UIList.size())) {
				m_UIList[i]->SetParent(m_UIList[parentIdx].get());
			}
		}
	}

	Deserialize(jsonData);

	GameObjectBase::SetSelectedObject(nullptr);
	BaseFinalize();
	m_gameObjects.clear();

	for (const auto& gameObjectData : jsonData["rootGameObjects"])
	{
		string typeName = gameObjectData["type"].get<string>();
		unique_ptr<Base> gameObjectPtr = TypeRegistry::GetInstance().CreateGameObject(typeName);

		gameObjectPtr->BaseDeserialize(gameObjectData);
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

	m_skyboxVertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout("VSSkybox.hlsl"); // ?��카이박스 ?��?�� ?��?��?�� ?���?
	m_skyboxPixelShader = resourceManager.GetPixelShader("PSSkybox.hlsl"); // ?��카이박스 ?��??? ?��?��?�� ?���?
	m_shadowMapPixelShader = resourceManager.GetPixelShader("PSShadow.hlsl"); // 그림?�� �? ?��?��?�� ?��??? ?��?��?�� ?���?

	#ifdef _DEBUG
	m_debugCoordinateVertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout("VSCoordinateLine.hlsl"); // ?��버그 좌표 ?��?�� ?��?��?�� ?���?
	m_debugCoordinatePixelShader = resourceManager.GetPixelShader("PSColor.hlsl"); // ?��버그 좌표 ?��??? ?��?��?�� ?���?
	#endif

	m_environmentMapSRV = resourceManager.GetTexture(m_environmentMapFileName); // ?���? �? 로드

	m_viewProjectionConstantBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::ViewProjection); // �?-?��?�� ?��?�� 버퍼 ?��?��
	m_skyboxViewProjectionConstantBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::SkyboxViewProjection); // ?��카이박스 �?-?��?�� ?��?��?�� ?��?�� 버퍼 ?��?��
	m_timeConstantBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::Time); // ?���? ?��?�� 버퍼 ?��?��

	m_cameraPositionConstantBuffer = resourceManager.GetConstantBuffer(PSConstBuffers::CameraPosition); // 카메?�� ?���? ?��?�� 버퍼 ?��?��
	m_globalLightConstantBuffer = resourceManager.GetConstantBuffer(PSConstBuffers::GlobalLight); // 방향�? ?��?�� 버퍼 ?��?��

	m_postProcessingConstantBuffer = resourceManager.GetConstantBuffer(PSConstBuffers::PostProcessing); // ?��처리?�� ?��?�� 버퍼 ?��?��

	m_spriteFont = resourceManager.GetSpriteFont(L"Gugi");
}

void SceneBase::UpdateConstantBuffers()
{
	const CameraComponent& mainCamera = CameraComponent::GetMainCamera();

	// �?-?��?�� ?��?�� 버퍼 ?��?��?��?�� �? ?��?��?��?�� ?��?��
	m_viewProjectionData.viewMatrix = mainCamera.GetViewMatrix();
	m_viewProjectionData.projectionMatrix = mainCamera.GetProjectionMatrix();
	m_viewProjectionData.VPMatrix = XMMatrixTranspose(m_viewProjectionData.viewMatrix * m_viewProjectionData.projectionMatrix);
	m_deviceContext->UpdateSubresource(m_viewProjectionConstantBuffer.Get(), 0, nullptr, &m_viewProjectionData, 0, 0);

	// ?��카이박스 �?-?��?�� ?��?��?�� ?��?�� 버퍼 ?��?��?��?�� �? ?��?��?��?�� ?��?�� // m_viewProjectionData ?��?��?��
	m_viewProjectionData.viewMatrix.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f); // �? ?��?��?�� ?���? ?���? ?���?
	m_skyboxViewProjectionData.skyboxVPMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, m_viewProjectionData.viewMatrix * m_viewProjectionData.projectionMatrix));
	m_deviceContext->UpdateSubresource(m_skyboxViewProjectionConstantBuffer.Get(), 0, nullptr, &m_skyboxViewProjectionData, 0, 0);

	// ?���? ?��?�� 버퍼 ?��?��?��?�� �? ?��?��?��?�� ?��?��
	m_timeData.totalTime = TimeManager::GetInstance().GetTotalTime();
	m_timeData.deltaTime = TimeManager::GetInstance().GetDeltaTime();
	m_timeData.sinTime = sinf(m_timeData.totalTime);
	m_timeData.cosTime = cosf(m_timeData.totalTime);
	m_deviceContext->UpdateSubresource(m_timeConstantBuffer.Get(), 0, nullptr, &m_timeData, 0, 0);

	// 카메?�� ?���? ?��?�� 버퍼 ?��?��?��?�� �? ?��?��?��?�� ?��?��
	m_cameraPositionData.cameraPosition = mainCamera.GetPosition();
	m_deviceContext->UpdateSubresource(m_cameraPositionConstantBuffer.Get(), 0, nullptr, &m_cameraPositionData, 0, 0);

	// ?��경광, 방향�? ?��?�� 버퍼 ?��?��?��?�� �? ?��?��?��?�� ?��?��
	m_deviceContext->UpdateSubresource(m_globalLightConstantBuffer.Get(), 0, nullptr, &m_globalLightData, 0, 0);

	// ?��처리?�� ?��?�� 버퍼 ?��?��?��?�� �? ?��?��?��?�� ?��?��
	m_deviceContext->UpdateSubresource(m_postProcessingConstantBuffer.Get(), 0, nullptr, &m_postProcessingData, 0, 0);
}

void SceneBase::RenderSkybox()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();

	resourceManager.SetDepthStencilState(DepthStencilState::Skybox);

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
void SceneBase::PickObjectDebugCamera()
{
	const POINT& mouse = InputManager::GetInstance().GetMousePosition();
	pair<XMVECTOR, XMVECTOR> ray = CameraComponent::GetMainCamera().RayCast(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
	GameObjectBase::SetSelectedObject(ModelComponent::CheckCollision(ray.first, ray.second));
}

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

	m_deviceContext->DrawInstanced(2, 2004, 0, 0);
}
#endif