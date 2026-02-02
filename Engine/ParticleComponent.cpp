#include "stdafx.h"
#include "ParticleComponent.h"

#include "Renderer.h"
#include "ResourceManager.h"
#include "GameObjectBase.h"
#include "CameraComponent.h"
#include "TimeManager.h"

using namespace std;
using namespace DirectX;

REGISTER_TYPE(ParticleComponent)


void ParticleComponent::Initialize()
{
	m_deviceContext = Renderer::GetInstance().GetDeviceContext();
	m_worldNormalData = &m_owner->GetWorldNormalBuffer();
	ResourceManager& resourceManager = ResourceManager::GetInstance();

	CreateShaders();
	CreateBuffers();

	m_worldNormalBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::WorldNormal);
	m_particleBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::Particle);
	m_particleEmissionBuffer = resourceManager.GetConstantBuffer(PSConstBuffers::ParticleEmission);

	particle_texture_srv_ = resourceManager.GetTexture(texture_file_name_);
}

void ParticleComponent::Update()
{
	m_elapsedTime += TimeManager::GetInstance().GetDeltaTime();
	if (m_killOwnerAfterFinish && m_elapsedTime > m_particleTotalTime)
	{
		#ifdef NDEBUG
		//m_owner->SetAlive(false);
		#endif
	};
	uv_buffer_data_.eclipsedTime = fmodf(m_elapsedTime / m_particleTotalTime, 1.0f); // 0~1 사이 값으로 유지
}

void ParticleComponent::Render()
{
	Renderer& renderer = Renderer::GetInstance();
	const CameraComponent& mainCamera = CameraComponent::GetMainCamera();
	const XMVECTOR& owner_pos = m_owner->GetPosition();

	// 일반 렌더링
	renderer.RENDER_FUNCTION(RenderStage::Scene, m_blendState).emplace_back
	(
		XMVectorGetX(XMVector3LengthSq(renderer.GetRenderSortPoint() - m_owner->GetPosition())),
		[&]()
		{
			ResourceManager& resourceManager = ResourceManager::GetInstance();
			resourceManager.SetRasterState(m_rasterState);

			// 상수 버퍼 업데이트
			m_deviceContext->UpdateSubresource(m_worldNormalBuffer.Get(), 0, nullptr, m_worldNormalData, 0, 0);
			m_deviceContext->UpdateSubresource(m_particleBuffer.Get(),0, nullptr, &uv_buffer_data_, 0, 0);
			m_deviceContext->UpdateSubresource(m_particleEmissionBuffer.Get(), 0, nullptr, &m_particleEmissionColor, 0, 0);

			constexpr UINT stride = sizeof(VertexPosUV);
			constexpr UINT offset = 0;
			m_deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

			m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			m_deviceContext->VSSetShader(m_vertexShaderAndInputLayout.first.Get(), nullptr, 0);
			m_deviceContext->IASetInputLayout(m_vertexShaderAndInputLayout.second.Get());
			m_deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);
			m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::BaseColor), 1, particle_texture_srv_.GetAddressOf());

			m_deviceContext->DrawInstanced(4, m_particleAmount, 0, 0);
		}
	);
}

#ifdef _DEBUG
void ParticleComponent::RenderImGui()
{
	// 1. 파티클 설정 섹션
	if (ImGui::CollapsingHeader("Particle Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragInt("Particle Amount", &m_particleAmount, 1, 1, 1000000);

		ImGui::DragFloat("Image Scale", &uv_buffer_data_.imageScale, 0.01f, 0.1f, 10.0f);
		ImGui::DragFloat("Spread Radius", &uv_buffer_data_.spreadRadius, 0.1f, 0.0f, 10.0f);
		ImGui::DragFloat("Spread Distance", &uv_buffer_data_.spreadDistance, 0.1f, 0.0f, 1000.0f);
		ImGui::DragFloat("Particle Total Time", &m_particleTotalTime, 0.01f, 0.1f, 100.0f);
		ImGui::Checkbox("Kill Owner After Finish", &m_killOwnerAfterFinish);
		ImGui::ColorEdit4("Particle Emission Color", &m_particleEmissionColor.x);

		ImGui::Separator();
	}

	// 2. 텍스처 설정 섹션
	if (ImGui::CollapsingHeader("Texture Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static char textureBuffer[256] = "";
		if (textureBuffer[0] == '\0') strcpy_s(textureBuffer, texture_file_name_.c_str());

		if (ImGui::InputText("Texture File", textureBuffer, sizeof(textureBuffer))) texture_file_name_ = textureBuffer;
		if (ImGui::Button("Reload Texture")) particle_texture_srv_ = ResourceManager::GetInstance().GetTexture(texture_file_name_);

		if (particle_texture_srv_)
		{
			ImGui::Text("Preview:");
			ImGui::Image((ImTextureID)particle_texture_srv_.Get(), ImVec2(64, 64));
		}

		ImGui::Separator();

		bool uvChanged = false; 
		uvChanged |= ImGui::DragFloat2("UV Offset", &uv_buffer_data_.uvOffset.x, 0.01f);
		uvChanged |= ImGui::DragFloat2("UV Scale", &uv_buffer_data_.uvScale.x, 0.01f);
	}

	// 3. 렌더링 옵션 섹션
	if (ImGui::CollapsingHeader("Rendering Options", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const char* billboardItems[] = { "None", "Spherical", "Cylindrical" };
		int currentBillboard = static_cast<int>(billboard_type_);
		if (ImGui::Combo("Billboard Type", &currentBillboard, billboardItems, IM_ARRAYSIZE(billboardItems)))
		{
			billboard_type_ = static_cast<BillboardType>(currentBillboard);
			CreateShaders();
		}

		// 블렌드 상태 콤보박스
		const char* blendItems[] = { "Opaque", "AlphaToCoverage", "AlphaBlend" };
		int currentBlend = static_cast<int>(m_blendState);
		if (ImGui::Combo("Blend State", &currentBlend, blendItems, IM_ARRAYSIZE(blendItems)))
		{
			m_blendState = static_cast<BlendState>(currentBlend);
		}

		// 래스터 상태 콤보박스
		const char* rasterItems[] = { "BackBuffer", "Solid", "Wireframe", "SolidCullNone", "WireframeCullNone" };
		int currentRaster = static_cast<int>(m_rasterState);
		if (ImGui::Combo("Raster State", &currentRaster, rasterItems, IM_ARRAYSIZE(rasterItems)))
		{
			m_rasterState = static_cast<RasterState>(currentRaster);
		}

	}
}
#endif

nlohmann::json ParticleComponent::Serialize()
{
	nlohmann::json jsonData;

	jsonData["vsShaderName"] = m_vsShaderName;
	jsonData["psShaderName"] = m_psShaderName;

	jsonData["textureFileName"] = texture_file_name_;

	jsonData["particleAmount"] = m_particleAmount;

	jsonData["uvOffset"] = { uv_buffer_data_.uvOffset.x, uv_buffer_data_.uvOffset.y };
	jsonData["uvScale"] = { uv_buffer_data_.uvScale.x, uv_buffer_data_.uvScale.y };
	jsonData["imageScale"] = uv_buffer_data_.imageScale;
	jsonData["spreadRadius"] = uv_buffer_data_.spreadRadius;
	jsonData["spreadDistance"] = uv_buffer_data_.spreadDistance;
	jsonData["particleTotalTime"] = m_particleTotalTime;
	jsonData["KillOwnerAfterFinish"] = m_killOwnerAfterFinish;
	jsonData["particleEmissionColor"] = { m_particleEmissionColor.x, m_particleEmissionColor.y, m_particleEmissionColor.z, m_particleEmissionColor.w };

	jsonData["billboardType"] = static_cast<int>(billboard_type_);
	jsonData["blendState"] = static_cast<int>(m_blendState);
	jsonData["rasterState"] = static_cast<int>(m_rasterState);

	return jsonData;
}

void ParticleComponent::Deserialize(const nlohmann::json& jsonData)
{
	m_vsShaderName = jsonData["vsShaderName"].get<string>();
	m_psShaderName = jsonData["psShaderName"].get<string>();

	if (jsonData.contains("textureFileName"))
	{
		texture_file_name_ = jsonData["textureFileName"].get<string>();
		particle_texture_srv_ = ResourceManager::GetInstance().GetTexture(texture_file_name_);
	}

	if (jsonData.contains("particleAmount")) m_particleAmount = jsonData["particleAmount"].get<int>();

	if (jsonData.contains("uvOffset"))
	{
		uv_buffer_data_.uvOffset.x = jsonData["uvOffset"][0].get<float>();
		uv_buffer_data_.uvOffset.y = jsonData["uvOffset"][1].get<float>();
	}
	if (jsonData.contains("uvScale"))
	{
		uv_buffer_data_.uvScale.x = jsonData["uvScale"][0].get<float>();
		uv_buffer_data_.uvScale.y = jsonData["uvScale"][1].get<float>();
	}
	if (jsonData.contains("imageScale")) uv_buffer_data_.imageScale = jsonData["imageScale"].get<float>();
	if (jsonData.contains("spreadRadius")) uv_buffer_data_.spreadRadius = jsonData["spreadRadius"].get<float>();
	if (jsonData.contains("spreadDistance")) uv_buffer_data_.spreadDistance = jsonData["spreadDistance"].get<float>();
	if (jsonData.contains("particleTotalTime")) m_particleTotalTime = jsonData["particleTotalTime"].get<float>();
	if (jsonData.contains("KillOwnerAfterFinish")) m_killOwnerAfterFinish = jsonData["KillOwnerAfterFinish"].get<bool>();
	if (jsonData.contains("particleEmissionColor"))
	{
		m_particleEmissionColor.x = jsonData["particleEmissionColor"][0].get<float>();
		m_particleEmissionColor.y = jsonData["particleEmissionColor"][1].get<float>();
		m_particleEmissionColor.z = jsonData["particleEmissionColor"][2].get<float>();
		m_particleEmissionColor.w = jsonData["particleEmissionColor"][3].get<float>();
	}

	if (jsonData.contains("billboardType"))
	{
		billboard_type_ = static_cast<BillboardType>(jsonData["billboardType"].get<int>());
		m_vsShaderName = GetBillboardVSName(billboard_type_);
	}
	if (jsonData.contains("blendState")) m_blendState = static_cast<BlendState>(jsonData["blendState"].get<int>());
	if (jsonData.contains("rasterState")) m_rasterState = static_cast<RasterState>(jsonData["rasterState"].get<int>());
}

void ParticleComponent::CreateShaders()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();
	m_vsShaderName = GetBillboardVSName(billboard_type_);
	m_vertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout(m_vsShaderName, m_inputElements);
	m_pixelShader = resourceManager.GetPixelShader(m_psShaderName);
}
void ParticleComponent::CreateBuffers()
{
	HRESULT hr = S_OK;

	com_ptr<ID3D11Device> device;
	m_deviceContext->GetDevice(device.GetAddressOf());

	D3D11_BUFFER_DESC vbDesc = {};

	vbDesc.Usage = D3D11_USAGE_DYNAMIC;
	vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	vbDesc.ByteWidth = sizeof(VertexPosUV) * static_cast<UINT>(quad_.size()); 
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vbInitData = {};
	vbInitData.pSysMem = quad_.data();

	hr = device->CreateBuffer(&vbDesc, &vbInitData, m_vertexBuffer.GetAddressOf());
	assert(SUCCEEDED(hr));
}


string ParticleComponent::GetBillboardVSName(BillboardType type)
{
	switch (type)
	{
	case BillboardType::None:
		return "VSParticle_None.hlsl";
	case BillboardType::Cylindrical:
		return "VSParticle_Cylindrical.hlsl";
	case BillboardType::Spherical:
	default:
		return "VSParticle.hlsl";
	}
}
/// ParticleComponent.cpp의 끝