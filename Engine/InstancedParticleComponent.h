/// InstancedParticleComponent.h의 시작
#pragma once
#include "ParticleComponent.h"

class InstancedParticleComponent : public ParticleComponent
{
public:
	struct InstanceData
	{
		DirectX::XMMATRIX World = DirectX::XMMatrixIdentity();
		DirectX::XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT2 UVOffset = { 0.0f, 0.0f };
		DirectX::XMFLOAT2 Padding = { 0.0f, 0.0f };
	};

	InstancedParticleComponent() = default;
	~InstancedParticleComponent() override = default;
	InstancedParticleComponent(const InstancedParticleComponent&) = default;
	InstancedParticleComponent& operator=(const InstancedParticleComponent&) = default;
	InstancedParticleComponent(InstancedParticleComponent&&) = default;
	InstancedParticleComponent& operator=(InstancedParticleComponent&&) = default;

	void SetInstances(const std::vector<InstanceData>& data);

protected:
	void Initialize() override;
	void CreateShaders() override;
	void CreateBuffers() override;
	void Render() override;

private:
	void EnsureInstanceBuffer(size_t instance_count);

	std::vector<InstanceData> instances_;
	com_ptr<ID3D11Buffer> instance_buffer_ = nullptr;
	size_t instance_capacity_ = 0;
};
/// InstancedParticleComponent.h의 끝
