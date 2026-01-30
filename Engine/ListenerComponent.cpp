#include "stdafx.h"
#include "SoundManager.h"
#include "GameObjectBase.h"
#include "ListenerComponent.h"

REGISTER_TYPE(ListenerComponent)

void ListenerComponent::Initialize()
{

}

void ListenerComponent::Update()
{
	SoundManager::GetInstance().UpdateListener(this);
}

void ListenerComponent::Render()
{

}

#ifdef _DEBUG
void ListenerComponent::RenderImGui()
{
}
#endif

nlohmann::json ListenerComponent::Serialize()
{
	nlohmann::json jsonData;
	return jsonData;
}

void ListenerComponent::Deserialize(const nlohmann::json& jsonData)
{

}
