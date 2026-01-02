#include "stdafx.h"
#include "Base.h"

using namespace std;

string Base::GetTypeName() const
{
	string typeName = typeid(*this).name();
	if (typeName.find("class ") == 0) typeName = typeName.substr(6);

	return typeName;
}