#include "stdafx.h"
#include "UIManager.h"

using namespace std;
using namespace DirectX;
using json = nlohmann::json;

void UIManager::Deserialize(const nlohmann::json& jsonData)
{
	if (jsonData.find("ui") == jsonData.end()) return;



}