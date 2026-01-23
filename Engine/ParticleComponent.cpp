/// ParticleComponent.cpp의 시작
#include "stdafx.h"
#include "ParticleComponent.h"

#include "Renderer.h"
#include "ResourceManager.h"
#include "GameObjectBase.h"
#include "CameraComponent.h"

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
	particle_texture_srv_ = resourceManager.GetTexture("Crosshair.png");

	//m_localBoundingBox.Center = XMFLOAT3(0.f, 0.f, 0.f);
	//m_localBoundingBox.Extents = XMFLOAT3(0.5f, 0.5f, 0.01f);
}

void ParticleComponent::Render()
{
	Renderer& renderer = Renderer::GetInstance();
	const CameraComponent& mainCamera = CameraComponent::GetMainCamera(); // 릴리즈고...

	//m_localBoundingBox.Transform(m_boundingBox, m_owner->GetWorldMatrix());
	//XMVECTOR boxCenter = XMLoadFloat3(&m_boundingBox.Center);
	//XMVECTOR boxExtents = XMLoadFloat3(&m_boundingBox.Extents);
	const XMVECTOR& owner_pos = m_owner->GetPosition();
	const XMVECTOR& sortPoint = renderer.GetRenderSortPoint();

	// 일반 렌더링
	renderer.RENDER_FUNCTION(RenderStage::Scene, m_blendState).emplace_back
	(
		//// 카메라로부터의 거리
		//XMVectorGetX(XMVector3LengthSq(sortPoint - XMVectorClamp(sortPoint, boxCenter - boxExtents, boxCenter + boxExtents))),
		XMVectorGetX(XMVector3LengthSq(sortPoint - owner_pos)),
		[&]()
		{
			// 프러스텀 컬링
			//if (m_boundingBox.Intersects(mainCamera.GetBoundingFrustum()) == false) return;
			if (mainCamera.GetBoundingFrustum().Contains(owner_pos)) return;

			ResourceManager& resourceManager = ResourceManager::GetInstance();
			resourceManager.SetRasterState(m_rasterState);

			// 상수 버퍼 업데이트
			m_deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::WorldNormal).Get(), 0, nullptr, m_worldNormalData, 0, 0);



			UINT stride = sizeof(VertexPosUV);
			UINT offset = 0;
			m_deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

			m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			m_deviceContext->VSSetShader(m_vertexShaderAndInputLayout.first.Get(), nullptr, 0);
			m_deviceContext->IASetInputLayout(m_vertexShaderAndInputLayout.second.Get());
			m_deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);
			m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::BaseColor), 1, particle_texture_srv_.GetAddressOf());

			m_deviceContext->Draw(4, 0);
		}
	);

//// 디버그 - 경계 상자 렌더링
//#ifdef _DEBUG
//	renderer.RENDER_FUNCTION(RenderStage::Scene, BlendState::Opaque).emplace_back
//	(
//		numeric_limits<float>::max(), // 제일 나중에 그림
//		[&]()
//		{
//			if (!m_renderBoundingBox) return; // 렌더링 옵션 체크
//
//			// 프러스텀 컬링 (월드 기준)
//			if (m_boundingBox.Intersects(mainCamera.GetBoundingFrustum()) == false) return;
//
//			ResourceManager& resourceManager = ResourceManager::GetInstance();
//
//			m_deviceContext->IASetInputLayout(m_boundingBoxVertexShaderAndInputLayout.second.Get());
//			m_deviceContext->VSSetShader(m_boundingBoxVertexShaderAndInputLayout.first.Get(), nullptr, 0);
//			m_deviceContext->PSSetShader(m_boundingBoxPixelShader.Get(), nullptr, 0);
//
//			resourceManager.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
//
//			array<XMFLOAT3, 8> boxVertices = {};
//			m_boundingBox.GetCorners(boxVertices.data());
//
//			LineBuffer lineBufferData = {};
//
//			for (const auto& [startIndex, endIndex] : BOX_LINE_INDICES)
//			{
//				lineBufferData.linePoints[0] = XMFLOAT4{ boxVertices[startIndex].x, boxVertices[startIndex].y, boxVertices[startIndex].z, 1.0f };
//				lineBufferData.linePoints[1] = XMFLOAT4{ boxVertices[endIndex].x, boxVertices[endIndex].y, boxVertices[endIndex].z, 1.0f };
//				lineBufferData.lineColors[0] = XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f }; // 파티클은 초록색으로 표시해봄
//				lineBufferData.lineColors[1] = XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f };
//
//				m_deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
//				m_deviceContext->Draw(2, 0);
//			}
//		}
//	);
//#endif
}

void ParticleComponent::RenderImGui()
{
	array<char, 256> vsShaderNameBuffer = {};
	strcpy_s(vsShaderNameBuffer.data(), vsShaderNameBuffer.size(), m_vsShaderName.c_str());
	if (ImGui::InputText("Vertex Shader Name", vsShaderNameBuffer.data(), sizeof(vsShaderNameBuffer))) m_vsShaderName = vsShaderNameBuffer.data();

	array<char, 256> psShaderNameBuffer = {};
	strcpy_s(psShaderNameBuffer.data(), psShaderNameBuffer.size(), m_psShaderName.c_str());
	if (ImGui::InputText("Pixel Shader Name", psShaderNameBuffer.data(), sizeof(psShaderNameBuffer))) m_psShaderName = psShaderNameBuffer.data();


	ImGui::Separator();
	// 재질 팩터
	ImGui::ColorEdit4("BaseColor Factor", &m_materialFactorData.baseColorFactor.x);

	ImGui::DragFloat("Ambient Occlusion Factor", &m_materialFactorData.ambientOcclusionFactor, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Roughness Factor", &m_materialFactorData.roughnessFactor, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Metallic Factor", &m_materialFactorData.metallicFactor, 0.01f, 0.0f, 1.0f);

	ImGui::DragFloat("Normal Scale", &m_materialFactorData.normalScale, 0.01f, 0.0f, 5.0f);

	ImGui::ColorEdit4("Emission Factor", &m_materialFactorData.emissionFactor.x);

	ImGui::Separator();
	int blendStateInt = static_cast<int>(m_blendState);
	if (ImGui::Combo("Blend State", &blendStateInt, "Opaque\0AlphaToCoverage\0AlphaBlend\0")) m_blendState = static_cast<BlendState>(blendStateInt);
	int rasterStateInt = static_cast<int>(m_rasterState);
	if (ImGui::Combo("Raster State", &rasterStateInt, "BackBuffer\0Solid\0Wireframe\0")) m_rasterState = static_cast<RasterState>(rasterStateInt);
}

nlohmann::json ParticleComponent::Serialize()
{
	nlohmann::json jsonData;

	jsonData["vsShaderName"] = m_vsShaderName;
	jsonData["psShaderName"] = m_psShaderName;

	return jsonData;
}

void ParticleComponent::Deserialize(const nlohmann::json& jsonData)
{
	m_vsShaderName = jsonData["vsShaderName"].get<string>();
	m_psShaderName = jsonData["psShaderName"].get<string>();
}

void ParticleComponent::CreateShaders()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();
	m_vertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout(m_vsShaderName, m_inputElements);
	m_pixelShader = resourceManager.GetPixelShader(m_psShaderName);

//#ifdef _DEBUG
//	m_boundingBoxVertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout("VSLine.hlsl");
//	m_boundingBoxPixelShader = resourceManager.GetPixelShader("PSColor.hlsl");
//#endif

}
void ParticleComponent::CreateBuffers()
{
	com_ptr<ID3D11Device> device;
	m_deviceContext->GetDevice(device.GetAddressOf());

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.ByteWidth = sizeof(VertexPosUV) * static_cast<UINT>(quad_.size()); 
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbInitData = {};
	vbInitData.pSysMem = quad_.data();

	// 정점 버퍼 생성
	HRESULT hr = device->CreateBuffer(&vbDesc, &vbInitData, m_vertexBuffer.GetAddressOf());
	assert(SUCCEEDED(hr));
}
/// ParticleComponent.cpp의 끝