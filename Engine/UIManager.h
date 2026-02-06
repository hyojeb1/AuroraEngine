#pragma once


class UIManager : public Singleton<UIManager>
{
	friend class Singleton<UIManager>;
	
public:
	void Deserialize(const nlohmann::json& jsonData);
	nlohmann::json Serialize() const;

};