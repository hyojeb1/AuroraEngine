/// FlipbookParticleComponent.h의 시작
#pragma once
#include "ParticleComponent.h"

class FlipbookParticleComponent : public ParticleComponent
{
	int m_rows = 1;
	int m_columns = 1;
	int m_totalFrames = 1;
	float m_framesPerSecond = 8.0f;
	bool m_loop = true;
	bool m_playing = true;
	int m_currentFrame = 0;
	float m_accumulatedTime = 0.0f;

public:
	FlipbookParticleComponent() = default;
	~FlipbookParticleComponent() override = default;
	FlipbookParticleComponent(const FlipbookParticleComponent&) = default;
	FlipbookParticleComponent& operator=(const FlipbookParticleComponent&) = default;
	FlipbookParticleComponent(FlipbookParticleComponent&&) = default;
	FlipbookParticleComponent& operator=(FlipbookParticleComponent&&) = default;

	bool NeedsUpdate() const override { return true; }

protected:
	void Initialize() override;
	void Update() override;
	void RenderImGui() override;

	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;

private:
	int GetMaxFrames() const;
	void ClampFrame();
	void ApplyFrameToUV();
};
/// FlipbookParticleComponent.h의 끝