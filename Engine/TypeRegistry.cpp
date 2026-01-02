#include "stdafx.h"
#include "TypeRegistry.h"

using namespace std;

unique_ptr<Base> TypeRegistry::Create(const string& typeName)
{
	auto it = m_registry.find(typeName);
	if (it != m_registry.end()) return it->second();

	#ifdef _DEBUG
	cerr << "Error: 타입 '" << typeName << "' 을(를) 찾을 수 없습니다." << endl;
	#else
	MessageBoxA(nullptr, ("Error: 타입 '" + typeName + "' 을(를) 찾을 수 없습니다.").c_str(), "Fatal Error", MB_OK | MB_ICONERROR);
	#endif
	exit(EXIT_FAILURE);

	return nullptr;
}