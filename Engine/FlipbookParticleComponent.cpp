/// FlipbookParticleComponent.cpp의 시작
#include "stdafx.h"
#include "FlipbookParticleComponent.h"
#include "TimeManager.h"

using namespace std;

REGISTER_TYPE(FlipbookParticleComponent)

void FlipbookParticleComponent::Initialize()
{
	ParticleComponent::Initialize();
	ApplyFrameToUV();
}

void FlipbookParticleComponent::Update()
{
	if (!m_playing) return;

	if (m_rows <= 0 || m_columns <= 0) return;

	const int maxFrames = GetMaxFrames();
	if (maxFrames <= 1) return;

	if (m_framesPerSecond <= 0.0f) return;

	const float delta_time = TimeManager::GetInstance().GetDeltaTime();
	m_accumulatedTime += delta_time;

	const float frameDuration = 1.0f / m_framesPerSecond;

	bool updated = false;
	while (m_accumulatedTime >= frameDuration)
	{
		m_accumulatedTime -= frameDuration;
		++m_currentFrame;
		updated = true;

		if (m_currentFrame >= maxFrames)
		{
			if (m_loop)
			{
				m_currentFrame = 0;
			}
			else
			{
				m_currentFrame = maxFrames - 1;
				m_playing = false;
				break;
			}
		}
	}

	if (updated)
	{
		ApplyFrameToUV();
	}
}

void FlipbookParticleComponent::RenderImGui()
{
	ParticleComponent::RenderImGui();

	if (ImGui::CollapsingHeader("Flipbook Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bool changed = false;
		changed |= ImGui::DragInt("Rows", &m_rows, 1, 1, 128);
		changed |= ImGui::DragInt("Columns", &m_columns, 1, 1, 128);
		changed |= ImGui::DragInt("Total Frames", &m_totalFrames, 1, 1, 4096);
		changed |= ImGui::DragFloat("FPS", &m_framesPerSecond, 0.1f, 0.0f, 120.0f);
		ImGui::Checkbox("Loop", &m_loop);
		ImGui::Checkbox("Playing", &m_playing);

		if (ImGui::Button("Restart"))
		{
			m_currentFrame = 0;
			m_accumulatedTime = 0.0f;
			m_playing = true;
			ApplyFrameToUV();
		}

		if (changed)
		{
			ClampFrame();
			ApplyFrameToUV();
		}

		ImGui::Text("Current Frame: %d / %d", m_currentFrame + 1, GetMaxFrames());
	}
}

nlohmann::json FlipbookParticleComponent::Serialize()
{
	nlohmann::json jsonData = ParticleComponent::Serialize();

	jsonData["flipbookRows"] = m_rows;
	jsonData["flipbookColumns"] = m_columns;
	jsonData["flipbookTotalFrames"] = m_totalFrames;
	jsonData["flipbookFPS"] = m_framesPerSecond;
	jsonData["flipbookLoop"] = m_loop;
	jsonData["flipbookPlaying"] = m_playing;
	jsonData["flipbookCurrentFrame"] = m_currentFrame;

	return jsonData;
}

void FlipbookParticleComponent::Deserialize(const nlohmann::json& jsonData)
{
	ParticleComponent::Deserialize(jsonData);

	if (jsonData.contains("flipbookRows")) m_rows = jsonData["flipbookRows"].get<int>();
	if (jsonData.contains("flipbookColumns")) m_columns = jsonData["flipbookColumns"].get<int>();
	if (jsonData.contains("flipbookTotalFrames")) m_totalFrames = jsonData["flipbookTotalFrames"].get<int>();
	if (jsonData.contains("flipbookFPS")) m_framesPerSecond = jsonData["flipbookFPS"].get<float>();
	if (jsonData.contains("flipbookLoop")) m_loop = jsonData["flipbookLoop"].get<bool>();
	if (jsonData.contains("flipbookPlaying")) m_playing = jsonData["flipbookPlaying"].get<bool>();
	if (jsonData.contains("flipbookCurrentFrame")) m_currentFrame = jsonData["flipbookCurrentFrame"].get<int>();

	m_accumulatedTime = 0.0f;
	ClampFrame();
	ApplyFrameToUV();
}

int FlipbookParticleComponent::GetMaxFrames() const
{
	if (m_rows <= 0 || m_columns <= 0) return 0;
	const int gridFrames = m_rows * m_columns;
	if (m_totalFrames <= 0) return gridFrames;
	return min(m_totalFrames, gridFrames);
}

void FlipbookParticleComponent::ClampFrame()
{
	if (m_rows <= 0) m_rows = 1;
	if (m_columns <= 0) m_columns = 1;
	if (m_totalFrames <= 0) m_totalFrames = 1;

	const int maxFrames = GetMaxFrames();
	if (maxFrames <= 0)
	{
		m_currentFrame = 0;
		return;
	}
	if (m_currentFrame < 0) m_currentFrame = 0;
	if (m_currentFrame >= maxFrames) m_currentFrame = maxFrames - 1;
}

void FlipbookParticleComponent::ApplyFrameToUV()
{
	const int maxFrames = GetMaxFrames();
	if (maxFrames <= 0) return;

	const int columns = max(1, m_columns);
	const int frameIndex = max(0, min(m_currentFrame, maxFrames - 1));

	const int columnIndex = frameIndex % columns;
	const int rowIndex = frameIndex / columns;

	uv_scale_.x = 1.0f / static_cast<float>(columns);
	uv_scale_.y = 1.0f / static_cast<float>(max(1, m_rows));
	uv_offset_.x = static_cast<float>(columnIndex) * uv_scale_.x;
	uv_offset_.y = static_cast<float>(rowIndex) * uv_scale_.y;

	if (m_vertexBuffer)
	{
		UpdateUVs();
	}
	else
	{
		RefreshQuadUVs();
	}
}
/// FlipbookParticleComponent.cpp의 끝