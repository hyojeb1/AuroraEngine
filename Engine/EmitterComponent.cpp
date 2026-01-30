#include "stdafx.h"
#include "SoundManager.h"
#include "GameObjectBase.h"
#include "EmitterComponent.h"

REGISTER_TYPE(EmitterComponent)

void EmitterComponent::SFX_Shot(std::string filename)
{
	SoundManager::GetInstance().SFX_Shot(GetOwner()->GetPosition(), filename);
}

void EmitterComponent::Initialize()
{

}

void EmitterComponent::Update()
{

}

void EmitterComponent::Render()
{

}

#ifdef _DEBUG
void EmitterComponent::RenderImGui()
{
	std::array<char, 256> sourceFileNameBuffer = {};
	strcpy_s(sourceFileNameBuffer.data(), sourceFileNameBuffer.size(), m_sourceName.c_str());
	if (ImGui::InputText("Source File Name", sourceFileNameBuffer.data(), sizeof(sourceFileNameBuffer))) m_sourceName = sourceFileNameBuffer.data();

	if (ImGui::Button("Play"))
	{
		SFX_Shot(m_sourceName);
	}
}
#endif

nlohmann::json EmitterComponent::Serialize()
{
	nlohmann::json jsonData;

	jsonData["SourceName"] = m_sourceName;

	return jsonData;
}

void EmitterComponent::Deserialize(const nlohmann::json& jsonData)
{
	m_sourceName = jsonData["SourceName"].get<std::string>();
}
