#include "stdafx.h"
#include "ComponentBase.h"

void ComponentBase::BaseInitialize()
{
	m_name = GetTypeName();

	Initialize();
}

void ComponentBase::BaseRenderImGui()
{
	if (ImGui::TreeNode(m_name.c_str()))
	{
		RenderImGui();

		ImGui::TreePop();
	}
}