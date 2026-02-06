#include "stdafx.h"

#include "ResourceManager.h"
#include "Renderer.h"

#include "UIBase.h"

void UIBase::SetTextureAndOffset(const std::string& idle)
{
	m_textureIdle = ResourceManager::GetInstance().GetTextureAndOffset(idle);
	UpdateRect();
}