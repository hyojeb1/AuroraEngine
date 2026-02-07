#include "stdafx.h"

#include "ResourceManager.h"
#include "Renderer.h"

#include "UIBase.h"
#include "Button.h"
#include "Panel.h"
#include "Slider.h"

using namespace nlohmann;

void UIBase::SetTextureAndOffset(const std::string& idle)
{
	m_textureIdle = ResourceManager::GetInstance().GetTextureAndOffset(idle);
	UpdateRect();
}

json UIBase::Serialize() const
{
    json data;
    data["type"] = GetTypeName();
    data["name"] = m_name;
    data["active"] = m_isActive;

    data["pos"] = { m_localPosition.x, m_localPosition.y };
    data["scale"] = m_scale;

    data["textureIdle"] = m_pathIdle;

    return data;
}

void UIBase::Deserialize(const json& data)
{
    if (data.contains("name")) m_name = data["name"];
    if (data.contains("active")) m_isActive = data["active"];

    if (data.contains("pos")) {
        m_localPosition.x = data["pos"][0];
        m_localPosition.y = data["pos"][1];
    }
    if (data.contains("scale")) m_scale = data["scale"];

    if (data.contains("textureIdle")) m_pathIdle = data.value("textureIdle", "");

    UpdateRect();
}

UIBase* UIBase::CreateFactory(const std::string& typeName)
{
    if (typeName == "Button")      return new Button();
    if (typeName == "Panel")       return new Panel();
    if (typeName == "Slider")      return new Slider();
    // if (typeName == "MovingPanel") return new MovingPanel();

    return nullptr;
}

