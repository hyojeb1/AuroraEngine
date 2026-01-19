#include "stdafx.h"
#include "GameObjectBase.h"
#include "ListenerComponent.h"

REGISTER_TYPE(ListenerComponent)

void ListenerComponent::Initialize()
{

}

void ListenerComponent::Update()
{

}

void ListenerComponent::Render()
{

}

void ListenerComponent::RenderImGui()
{
}

nlohmann::json ListenerComponent::Serialize()
{
	nlohmann::json jsonData;
	return jsonData;
}

void ListenerComponent::Deserialize(const nlohmann::json& jsonData)
{

}
