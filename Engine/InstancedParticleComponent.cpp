/// InstancedParticleComponent.cpp의 시작
#include "stdafx.h"
#include "InstancedParticleComponent.h"

#include <algorithm>
#include <d3dcompiler.h>
#include <filesystem>

#include "Renderer.h"
#include "ResourceManager.h"
#include "CameraComponent.h"
#include "GameObjectBase.h" 

using namespace std;
using namespace DirectX;

REGISTER_TYPE(InstancedParticleComponent)

void InstancedParticleComponent::Initialize()
{
	ParticleComponent::Initialize();

#ifdef _DEBUG
	if (instances_.empty())
	{
		uv_scale_ = { 0.5f, 0.5f };

		vector<InstanceData> debug_instances;
		debug_instances.reserve(4);

		InstanceData instance_a = {};
		instance_a.World = XMMatrixTranspose(XMMatrixTranslation(-1.0f, 0.0f, 0.0f));
		instance_a.Color = { 1.0f, 0.6f, 0.6f, 1.0f };
		instance_a.UVOffset = { 0.0f, 0.0f };
		debug_instances.push_back(instance_a);

		InstanceData instance_b = {};
		instance_b.World = XMMatrixTranspose(XMMatrixTranslation(1.0f, 0.0f, 0.0f));
		instance_b.Color = { 0.6f, 1.0f, 0.6f, 1.0f };
		instance_b.UVOffset = { 0.5f, 0.0f };
		debug_instances.push_back(instance_b);

		InstanceData instance_c = {};
		instance_c.World = XMMatrixTranspose(XMMatrixTranslation(-1.0f, -1.0f, 0.0f));
		instance_c.Color = { 0.6f, 0.6f, 1.0f, 1.0f };
		instance_c.UVOffset = { 0.0f, 0.5f };
		debug_instances.push_back(instance_c);

		InstanceData instance_d = {};
		instance_d.World = XMMatrixTranspose(XMMatrixTranslation(1.0f, -1.0f, 0.0f));
		instance_d.Color = { 1.0f, 1.0f, 0.6f, 1.0f };
		instance_d.UVOffset = { 0.5f, 0.5f };
		debug_instances.push_back(instance_d);

		SetInstances(debug_instances);
	}
#endif
}

void InstancedParticleComponent::SetInstances(const vector<InstanceData>& data)
{
	instances_ = data;
	if (instances_.empty())
	{
		return;
	}

	EnsureInstanceBuffer(instances_.size());
}

void InstancedParticleComponent::CreateShaders()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();
	m_vsShaderName = "VSParticleInstanced.hlsl";
	m_pixelShader = resourceManager.GetPixelShader(m_psShaderName);

	com_ptr<ID3D11Device> device;
	m_deviceContext->GetDevice(device.GetAddressOf());

	const filesystem::path shaderPath = "../Asset/Shader/" + m_vsShaderName;
	com_ptr<ID3DBlob> shaderCode = nullptr;
	com_ptr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3DCompileFromFile
	(
		shaderPath.wstring().c_str(),
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main",
		"vs_5_0",
#ifdef _DEBUG
		D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
#else
		D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3,
#endif
		0,
		shaderCode.GetAddressOf(),
		errorBlob.GetAddressOf()
	);
	if (errorBlob)
	{
		cerr << m_vsShaderName << " compile error: " << static_cast<const char*>(errorBlob->GetBufferPointer()) << endl;
	}
	assert(SUCCEEDED(hr));

	hr = device->CreateVertexShader
	(
		shaderCode->GetBufferPointer(),
		shaderCode->GetBufferSize(),
		nullptr,
		m_vertexShaderAndInputLayout.first.GetAddressOf()
	);
	assert(SUCCEEDED(hr));

	const D3D11_INPUT_ELEMENT_DESC input_layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

		{ "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
	};

		hr = device->CreateInputLayout
	(
		input_layout,
		static_cast<UINT>(sizeof(input_layout) / sizeof(input_layout[0])),
		shaderCode->GetBufferPointer(),
		shaderCode->GetBufferSize(),
		m_vertexShaderAndInputLayout.second.GetAddressOf()
	);
	assert(SUCCEEDED(hr));

#ifdef _DEBUG
	m_boundingBoxVertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout("VSLine.hlsl");
	m_boundingBoxPixelShader = resourceManager.GetPixelShader("PSColor.hlsl");
#endif
}

void InstancedParticleComponent::CreateBuffers()
{
	ParticleComponent::CreateBuffers();
	EnsureInstanceBuffer(1);
}

void InstancedParticleComponent::EnsureInstanceBuffer(size_t instance_count)
{
	if (instance_count <= instance_capacity_ && instance_buffer_)
	{
		return;
	}

	com_ptr<ID3D11Device> device;
	m_deviceContext->GetDevice(device.GetAddressOf());

	const size_t new_capacity = max<size_t>(instance_count, 1);
	instance_capacity_ = new_capacity;

	D3D11_BUFFER_DESC buffer_desc = {};
	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(InstanceData) * new_capacity);
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = 0;

	HRESULT hr = device->CreateBuffer(&buffer_desc, nullptr, instance_buffer_.GetAddressOf());
	assert(SUCCEEDED(hr));
}

void InstancedParticleComponent::Render()
{
	if (instances_.empty())
	{
		return;
	}

	Renderer& renderer = Renderer::GetInstance();
	const CameraComponent& mainCamera = CameraComponent::GetMainCamera();

#ifdef _DEBUG
	m_localBoundingBox.Transform(m_boundingBox, m_owner->GetWorldMatrix());
	XMVECTOR boxCenter = XMLoadFloat3(&m_boundingBox.Center);
	XMVECTOR boxExtents = XMLoadFloat3(&m_boundingBox.Extents);
#else
	const XMVECTOR& owner_pos = m_owner->GetPosition();
#endif

	const XMVECTOR& sortPoint = renderer.GetRenderSortPoint();

	renderer.RENDER_FUNCTION(RenderStage::Scene, m_blendState).emplace_back
	(
#ifdef _DEBUG
		XMVectorGetX(XMVector3LengthSq(sortPoint - XMVectorClamp(sortPoint, boxCenter - boxExtents, boxCenter + boxExtents))),
#else
		XMVectorGetX(XMVector3LengthSq(sortPoint - owner_pos)),
#endif
		[&]()
		{
#ifdef _DEBUG
			if (m_boundingBox.Intersects(mainCamera.GetBoundingFrustum()) == false) return;
#else
			if (mainCamera.GetBoundingFrustum().Contains(owner_pos) == DirectX::DISJOINT) return;
#endif

			ResourceManager& resourceManager = ResourceManager::GetInstance();
			resourceManager.SetRasterState(m_rasterState);

			uv_buffer_data_.uvOffset = { 0.0f, 0.0f };
			uv_buffer_data_.uvScale = uv_scale_;
			m_deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Particle).Get(), 0, nullptr, &uv_buffer_data_, 0, 0);

			EnsureInstanceBuffer(instances_.size());

			D3D11_MAPPED_SUBRESOURCE mapped = {};
			HRESULT hr = m_deviceContext->Map(instance_buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
			assert(SUCCEEDED(hr));
			memcpy(mapped.pData, instances_.data(), sizeof(InstanceData) * instances_.size());
			m_deviceContext->Unmap(instance_buffer_.Get(), 0);

			const UINT strides[2] = { sizeof(VertexPosUV), sizeof(InstanceData) };
			const UINT offsets[2] = { 0, 0 };
			ID3D11Buffer* buffers[2] = { m_vertexBuffer.Get(), instance_buffer_.Get() };
			m_deviceContext->IASetVertexBuffers(0, 2, buffers, strides, offsets);

			m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			m_deviceContext->VSSetShader(m_vertexShaderAndInputLayout.first.Get(), nullptr, 0);
			m_deviceContext->IASetInputLayout(m_vertexShaderAndInputLayout.second.Get());
			m_deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);
			m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::BaseColor), 1, particle_texture_srv_.GetAddressOf());

			m_deviceContext->DrawInstanced(4, static_cast<UINT>(instances_.size()), 0, 0);
		}
	);

#ifdef _DEBUG
	renderer.RENDER_FUNCTION(RenderStage::Scene, BlendState::Opaque).emplace_back
	(
		numeric_limits<float>::max(),
		[&]()
		{
			if (!m_renderBoundingBox) return;
			if (m_boundingBox.Intersects(mainCamera.GetBoundingFrustum()) == false) return;

			ResourceManager& resourceManager = ResourceManager::GetInstance();

			m_deviceContext->IASetInputLayout(m_boundingBoxVertexShaderAndInputLayout.second.Get());
			m_deviceContext->VSSetShader(m_boundingBoxVertexShaderAndInputLayout.first.Get(), nullptr, 0);
			m_deviceContext->PSSetShader(m_boundingBoxPixelShader.Get(), nullptr, 0);

			resourceManager.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

			array<XMFLOAT3, 8> boxVertices = {};
			m_boundingBox.GetCorners(boxVertices.data());

			LineBuffer lineBufferData = {};

			for (const auto& [startIndex, endIndex] : BOX_LINE_INDICES)
			{
				lineBufferData.linePoints[0] = XMFLOAT4{ boxVertices[startIndex].x, boxVertices[startIndex].y, boxVertices[startIndex].z, 1.0f };
				lineBufferData.linePoints[1] = XMFLOAT4{ boxVertices[endIndex].x, boxVertices[endIndex].y, boxVertices[endIndex].z, 1.0f };
				lineBufferData.lineColors[0] = XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f };
				lineBufferData.lineColors[1] = XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f };

				m_deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
				m_deviceContext->Draw(2, 0);
			}
		}
	);
#endif
}
/// InstancedParticleComponent.cpp의 끝
