#pragma once

class BaseClassRegisterManager : public Singleton<BaseClassRegisterManager>
{
	friend class Singleton<BaseClassRegisterManager>;

	std::unordered_map<std::string, std::function<std::unique_ptr<class Base>()>> m_registry;

public:
	BaseClassRegisterManager() = default;
	~BaseClassRegisterManager() = default;
	BaseClassRegisterManager(const BaseClassRegisterManager&) = delete;
	BaseClassRegisterManager& operator=(const BaseClassRegisterManager&) = delete;
	BaseClassRegisterManager(BaseClassRegisterManager&&) = delete;
	BaseClassRegisterManager& operator=(BaseClassRegisterManager&&) = delete;

	// 타입 등록
	template<typename T> requires std::derived_from<T, Base>
	void Register(const std::string& typeName) { m_registry[typeName] = []() -> std::unique_ptr<Base> { return std::make_unique<T>(); }; }

	// 타입 이름으로 인스턴스 생성
	std::unique_ptr<class Base> Create(const std::string& typeName);
};