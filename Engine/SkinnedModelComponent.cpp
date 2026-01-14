/// SkinnedModelComponent.cpp의 시작
#include "stdafx.h"
#include "SkinnedModelComponent.h"

#include "Renderer.h"
#include "ResourceManager.h"
#include "TimeManager.h"
#include "GameObjectBase.h"
#include "CameraComponent.h"

using namespace std;
using namespace DirectX;

REGISTER_TYPE(SkinnedModelComponent)

void SkinnedModelComponent::Initialize()
{
	m_deviceContext = Renderer::GetInstance().GetDeviceContext();
	m_worldNormalData = &m_owner->GetWorldNormalBuffer();

	ResourceManager& resourceManager = ResourceManager::GetInstance();

	CreateShaders();

	m_worldMatrixConstantBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::WorldNormal);
	m_boneConstantBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::Bone);
	m_materialConstantBuffer = resourceManager.GetConstantBuffer(PSConstBuffers::MaterialFactor);
	m_model = resourceManager.LoadSkinnedModel(m_modelFileName);

}

void SkinnedModelComponent::Render()
{
	Renderer::GetInstance().RENDER_FUNCTION(RenderStage::Scene, m_blendState).emplace_back
	(
		// 투명한 오브젝트의 경우 카메라로부터의 거리 저장
		m_blendState == BlendState::AlphaBlend ? XMVectorGetZ(XMVector3Dot(g_mainCamera->GetPosition() - m_worldNormalData->worldMatrix.r[3], g_mainCamera->GetForwardVector())) : 0.0f,
		[&]()
		{
			if (!m_model) return;

			m_deviceContext->UpdateSubresource(m_worldMatrixConstantBuffer.Get(), 0, nullptr, m_worldNormalData, 0, 0);
			//float animTime = TimeManager::GetInstance().GetTotalTime();
			//float swayA = sinf(animTime * 0.8f) * 0.2f;
			//float swayB = sinf(animTime * 1.1f + 1.5f) * 0.3f;
			//float swayC = sinf(animTime * 1.4f + 2.1f) * 0.4f;
			//m_boneBufferData.boneMatrix[0] = XMMatrixRotationZ(swayA);
			//m_boneBufferData.boneMatrix[1] = XMMatrixRotationZ(swayB);
			//m_boneBufferData.boneMatrix[2] = XMMatrixRotationZ(swayC);


			m_deviceContext->UpdateSubresource(m_boneConstantBuffer.Get(), 0, nullptr, &m_boneBufferData, 0, 0);

			ResourceManager& resourceManager = ResourceManager::GetInstance();
			resourceManager.SetRasterState(m_rasterState);

			m_deviceContext->IASetInputLayout(m_vertexShaderAndInputLayout.second.Get());
			m_deviceContext->VSSetShader(m_vertexShaderAndInputLayout.first.Get(), nullptr, 0);
			m_deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

			
			for (const SkinnedMesh& mesh : m_model->skinnedMeshes)
			{
				resourceManager.SetPrimitiveTopology(mesh.topology);

				// 메쉬 버퍼 설정
				constexpr UINT stride = sizeof(SkinnedVertex);
				constexpr UINT offset = 0;
				m_deviceContext->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
				m_deviceContext->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

				// 재질 팩터 설정
				m_deviceContext->UpdateSubresource(m_materialConstantBuffer.Get(), 0, nullptr, &m_materialFactorData, 0, 0);

				// 재질 텍스처 셰이더에 설정
				m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::Albedo), 1, mesh.materialTexture.albedoTextureSRV.GetAddressOf());
				m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::ORM), 1, mesh.materialTexture.ORMTextureSRV.GetAddressOf());
				m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::Normal), 1, mesh.materialTexture.normalTextureSRV.GetAddressOf());



				m_deviceContext->DrawIndexed(mesh.indexCount, 0, 0);
			}
		}
	);
}

void SkinnedModelComponent::RenderImGui()
{
	array<char, 256> modelFileNameBuffer = {};
	strcpy_s(modelFileNameBuffer.data(), modelFileNameBuffer.size(), m_modelFileName.c_str());
	if (ImGui::InputText("Model File Name", modelFileNameBuffer.data(), sizeof(modelFileNameBuffer))) m_modelFileName = modelFileNameBuffer.data();

	array<char, 256> vsShaderNameBuffer = {};
	strcpy_s(vsShaderNameBuffer.data(), vsShaderNameBuffer.size(), m_vsShaderName.c_str());
	if (ImGui::InputText("Vertex Shader Name", vsShaderNameBuffer.data(), sizeof(vsShaderNameBuffer))) m_vsShaderName = vsShaderNameBuffer.data();

	array<char, 256> psShaderNameBuffer = {};
	strcpy_s(psShaderNameBuffer.data(), psShaderNameBuffer.size(), m_psShaderName.c_str());
	if (ImGui::InputText("Pixel Shader Name", psShaderNameBuffer.data(), sizeof(psShaderNameBuffer))) m_psShaderName = psShaderNameBuffer.data();

	if (ImGui::Button("Load"))
	{
		m_model = ResourceManager::GetInstance().LoadSkinnedModel(m_modelFileName);
		CreateShaders();
	}

	ImGui::Separator();
	// 재질 팩터
	ImGui::ColorEdit4("Albedo Factor", &m_materialFactorData.albedoFactor.x);

	ImGui::DragFloat("Ambient Occlusion Factor", &m_materialFactorData.ambientOcclusionFactor, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Roughness Factor", &m_materialFactorData.roughnessFactor, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Metallic Factor", &m_materialFactorData.metallicFactor, 0.01f, 0.0f, 1.0f);

	ImGui::DragFloat("IOR", &m_materialFactorData.ior, 0.01f, 1.0f, 3.0f);

	ImGui::DragFloat("Normal Scale", &m_materialFactorData.normalScale, 0.01f, 0.0f, 5.0f);
	ImGui::DragFloat("Height Scale", &m_materialFactorData.heightScale, 0.001f, 0.0f, 0.2f);

	ImGui::DragFloat("Light Factor", &m_materialFactorData.lightFactor, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Glow Factor", &m_materialFactorData.glowFactor, 0.01f, 0.0f, 1.0f);

	ImGui::ColorEdit4("Emission Factor", &m_materialFactorData.emissionFactor.x);

	ImGui::Separator();
	int blendStateInt = static_cast<int>(m_blendState);
	if (ImGui::Combo("Blend State", &blendStateInt, "Opaque\0AlphaBlend\0AlphaToCoverage\0"))
	{
		m_blendState = static_cast<BlendState>(blendStateInt);
		ResourceManager::GetInstance().SetBlendState(m_blendState);
	}
	int rasterStateInt = static_cast<int>(m_rasterState);
	if (ImGui::Combo("Raster State", &rasterStateInt, "BackBuffer\0Solid\0Wireframe\0"))
	{
		m_rasterState = static_cast<RasterState>(rasterStateInt);
		ResourceManager::GetInstance().SetRasterState(m_rasterState);
	}

}

nlohmann::json SkinnedModelComponent::Serialize()
{
	nlohmann::json jsonData;

	jsonData["vsShaderName"] = m_vsShaderName;
	jsonData["psShaderName"] = m_psShaderName;
	jsonData["modelFileName"] = m_modelFileName;

	// 재질 팩터
	jsonData["materialFactorData"]["albedoFactor"] = { m_materialFactorData.albedoFactor.x, m_materialFactorData.albedoFactor.y, m_materialFactorData.albedoFactor.z, m_materialFactorData.albedoFactor.w };
	
	jsonData["materialFactorData"]["ambientOcclusionFactor"] = m_materialFactorData.ambientOcclusionFactor;
	jsonData["materialFactorData"]["roughnessFactor"] = m_materialFactorData.roughnessFactor;
	jsonData["materialFactorData"]["metallicFactor"] = m_materialFactorData.metallicFactor;

	jsonData["materialFactorData"]["ior"] = m_materialFactorData.ior;

	jsonData["materialFactorData"]["normalScale"] = m_materialFactorData.normalScale;
	jsonData["materialFactorData"]["heightScale"] = m_materialFactorData.heightScale;

	jsonData["materialFactorData"]["lightFactor"] = m_materialFactorData.lightFactor;
	jsonData["materialFactorData"]["glowFactor"] = m_materialFactorData.glowFactor;

	jsonData["materialFactorData"]["emissionFactor"] = { m_materialFactorData.emissionFactor.x, m_materialFactorData.emissionFactor.y, m_materialFactorData.emissionFactor.z, m_materialFactorData.emissionFactor.w };

	jsonData["blendState"] = static_cast<int>(m_blendState);
	jsonData["rasterState"] = static_cast<int>(m_rasterState);

	return jsonData;
}

void SkinnedModelComponent::Deserialize(const nlohmann::json& jsonData)
{
	m_vsShaderName = jsonData["vsShaderName"].get<string>();
	m_psShaderName = jsonData["psShaderName"].get<string>();
	m_modelFileName = jsonData["modelFileName"].get<string>();

	// 재질 팩터
	m_materialFactorData.albedoFactor = XMFLOAT4
	(
		jsonData["materialFactorData"]["albedoFactor"][0].get<float>(),
		jsonData["materialFactorData"]["albedoFactor"][1].get<float>(),
		jsonData["materialFactorData"]["albedoFactor"][2].get<float>(),
		jsonData["materialFactorData"]["albedoFactor"][3].get<float>()
	);

	m_materialFactorData.ambientOcclusionFactor = jsonData["materialFactorData"]["ambientOcclusionFactor"].get<float>();
	m_materialFactorData.roughnessFactor = jsonData["materialFactorData"]["roughnessFactor"].get<float>();
	m_materialFactorData.metallicFactor = jsonData["materialFactorData"]["metallicFactor"].get<float>();

	m_materialFactorData.ior = jsonData["materialFactorData"]["ior"].get<float>();

	m_materialFactorData.normalScale = jsonData["materialFactorData"]["normalScale"].get<float>();
	m_materialFactorData.heightScale = jsonData["materialFactorData"]["heightScale"].get<float>();

	m_materialFactorData.lightFactor = jsonData["materialFactorData"]["lightFactor"].get<float>();
	m_materialFactorData.glowFactor = jsonData["materialFactorData"]["glowFactor"].get<float>();

	m_materialFactorData.emissionFactor = XMFLOAT4
	(
		jsonData["materialFactorData"]["emissionFactor"][0].get<float>(),
		jsonData["materialFactorData"]["emissionFactor"][1].get<float>(),
		jsonData["materialFactorData"]["emissionFactor"][2].get<float>(),
		jsonData["materialFactorData"]["emissionFactor"][3].get<float>()
	);

	m_blendState = static_cast<BlendState>(jsonData["blendState"].get<int>());
	m_rasterState = static_cast<RasterState>(jsonData["rasterState"].get<int>());
}

void SkinnedModelComponent::CreateShaders()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();
	m_vertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout(m_vsShaderName, m_inputElements);
	m_pixelShader = resourceManager.GetPixelShader(m_psShaderName);
}
/// SkinnedModelComponent.cpp의 끝