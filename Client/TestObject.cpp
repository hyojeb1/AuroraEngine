///TestObject.cpp의 시작
#include "stdafx.h"
#include "TestObject.h"

#include "ModelComponent.h"
#include "FSMComponent.h"

using namespace std;
using namespace DirectX;

REGISTER_TYPE(TestObject)

//State를 상속받아서...
namespace
{
	class TestIdleState : public IState
	{
		void Enter(FSMComponent& machine) override
		{
		#ifdef _DEBUG
			cout << "Enter Test Idle" << endl;
		#endif // _DEBUG

		}

		void Exit(FSMComponent& machine) override
		{
		#ifdef _DEBUG
			cout << "Exit Test Idle" << endl;
		#endif // _DEBUG
		}

		void Update(FSMComponent& machine) override
		{

		}
	};

	class TestRunState : public IState
	{
		void Enter(FSMComponent& machine) override
		{
		#ifdef _DEBUG
			cout << "Enter Test Run" << endl;
		#endif // _DEBUG

		}

		void Exit(FSMComponent& machine) override
		{
		#ifdef _DEBUG
			cout << "Exit Test Run" << endl;
		#endif // _DEBUG
		}

		void Update(FSMComponent& machine) override
		{

		}
	};

}

void TestObject::Initialize()
{
	CreateComponent<ModelComponent>();
	SetScale({ 1.0f, 1.0f, 1.0f });
}

void TestObject::Update()
{

}

///TestObject.cpp의 끝