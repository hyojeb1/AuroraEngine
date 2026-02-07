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
    data["type"] = GetTypeName(); // 중요: 내 타입이 뭔지 저장
    data["name"] = m_name;
    data["active"] = m_isActive;

    // 위치 (XMFLOAT2 -> Array)
    data["pos"] = { m_localPosition.x, m_localPosition.y };
    data["scale"] = m_scale;

    // 텍스처 경로나 Color 등 공통 속성이 있다면 여기서 저장
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

    // 위치 등이 바뀌었으니 렉트 업데이트 필요
    UpdateRect();
}

UIBase* UIBase::CreateFactory(const std::string& typeName)
{
    if (typeName == "Button")      return new Button();
    if (typeName == "Panel")       return new Panel();
    if (typeName == "Slider")      return new Slider();
    // if (typeName == "MovingPanel") return new MovingPanel();

    return nullptr; // 모르는 타입
}

