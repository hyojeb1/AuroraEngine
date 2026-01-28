/// ParticleComponent.cpp의 시작
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
	particle_texture_srv_ = resourceManager.GetTexture(texture_file_name_);
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
			m_deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::WorldNormal).Get(), 0, nullptr, m_worldNormalData, 0, 0);

			uv_buffer_data_.uvOffset = uv_offset_;
			uv_buffer_data_.uvScale = uv_scale_;

			m_deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Particle).Get(),0, nullptr, &uv_buffer_data_, 0, 0);

			constexpr UINT stride = sizeof(VertexPosUV);
			constexpr UINT offset = 0;
			m_deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

			m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			m_deviceContext->VSSetShader(m_vertexShaderAndInputLayout.first.Get(), nullptr, 0);
			m_deviceContext->IASetInputLayout(m_vertexShaderAndInputLayout.second.Get());
			m_deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);
			m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::BaseColor), 1, particle_texture_srv_.GetAddressOf());

			m_deviceContext->DrawInstanced(4, 2, 0, 0);
		}
	);
}

void ParticleComponent::RenderImGui()
{
	// 1. 텍스처 설정 섹션
	if (ImGui::CollapsingHeader("Texture Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static char textureBuffer[256] = "";
		if (textureBuffer[0] == '\0') strcpy_s(textureBuffer, texture_file_name_.c_str());

		if (ImGui::InputText("Texture File", textureBuffer, sizeof(textureBuffer)))
		{
			texture_file_name_ = textureBuffer;
		}

		if (ImGui::Button("Reload Texture"))
		{
			particle_texture_srv_ = ResourceManager::GetInstance().GetTexture(texture_file_name_);
		}

		if (particle_texture_srv_)
		{
			ImGui::Text("Preview:");
			ImGui::Image((ImTextureID)particle_texture_srv_.Get(), ImVec2(64, 64));
		}

		ImGui::Separator();

		bool uvChanged = false; 
		uvChanged |= ImGui::DragFloat2("UV Offset", &uv_offset_.x, 0.01f);
		uvChanged |= ImGui::DragFloat2("UV Scale", &uv_scale_.x, 0.01f);		

	}

	// 2. 렌더링 옵션 섹션
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

	// 3. 재질(Material) 및 셰이더 섹션
	if (ImGui::CollapsingHeader("Material & Shader"))
	{
		ImGui::ColorEdit4("Base Color (RGBA)", &m_materialFactorData.baseColorFactor.x);

		ImGui::Separator();

		static char psBuffer[256] = "";
		if (psBuffer[0] == '\0') strcpy_s(psBuffer, m_psShaderName.c_str());

		ImGui::Text("VS Name: %s", m_vsShaderName.c_str());
		ImGui::InputText("PS Name", psBuffer, sizeof(psBuffer));

		if (ImGui::Button("Reload Shaders"))
		{
			m_psShaderName = psBuffer;
			CreateShaders();
		}
	}
}

nlohmann::json ParticleComponent::Serialize()
{
	nlohmann::json jsonData;

	jsonData["vsShaderName"] = m_vsShaderName;
	jsonData["psShaderName"] = m_psShaderName;

	jsonData["textureFileName"] = texture_file_name_;
	jsonData["uvOffset"] = { uv_offset_.x, uv_offset_.y };
	jsonData["uvScale"] = { uv_scale_.x, uv_scale_.y };
	jsonData["billboardType"] = static_cast<int>(billboard_type_);
	jsonData["blendState"] = static_cast<int>(m_blendState);
	jsonData["rasterState"] = static_cast<int>(m_rasterState); 

	jsonData["baseColor"] = {
		m_materialFactorData.baseColorFactor.x,
		m_materialFactorData.baseColorFactor.y,
		m_materialFactorData.baseColorFactor.z,
		m_materialFactorData.baseColorFactor.w
	};

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

	if (jsonData.contains("uvOffset"))
	{
		auto uv = jsonData["uvOffset"];
		uv_offset_ = { uv[0], uv[1] };
	}
	if (jsonData.contains("uvScale"))
	{
		auto uv = jsonData["uvScale"];
		uv_scale_ = { uv[0], uv[1] };
	}

	if (jsonData.contains("billboardType"))
	{
		billboard_type_ = static_cast<BillboardType>(jsonData["billboardType"].get<int>());
	}
	m_vsShaderName = GetBillboardVSName(billboard_type_);
	if (jsonData.contains("blendState")) m_blendState = static_cast<BlendState>(jsonData["blendState"].get<int>());
	if (jsonData.contains("rasterState")) m_rasterState = static_cast<RasterState>(jsonData["rasterState"].get<int>());

	if (jsonData.contains("baseColor"))
	{
		auto color = jsonData["baseColor"];
		m_materialFactorData.baseColorFactor = { color[0], color[1], color[2], color[3] };
	}
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