///GunObject.cpp의 시작
#include "stdafx.h"
#include "GunObject.h"

#include "ModelComponent.h"
#include "FSMComponentGun.h"
#include "TimeManager.h"

REGISTER_TYPE(GunObject)

using namespace std;
using namespace DirectX;

void GunObject::Fire()
{
	auto fsm = GetComponent<FSMComponentGun>();
	if (fsm->GetCurrentState() != EState::Attack)
	{
		fsm->ChangeState(EState::Attack);
	}
}

void GunObject::Initialize()
{
	auto fsm = GetComponent<FSMComponentGun>();
	if (!fsm){ fsm = CreateComponent<FSMComponentGun>();}

}

void GunObject::Update()
{

}

///GunObject.cpp의 끝