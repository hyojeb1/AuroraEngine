///BOF FSMComponentEnemy.cpp
#include "stdafx.h"
#include "FSMComponentEnemy.h"
#include "GameObjectBase.h"
#include "SkinnedModelComponent.h"

REGISTER_TYPE(FSMComponentEnemy)

using namespace std;
using namespace DirectX;

void FSMComponentEnemy::Initialize()
{
	skinned_model_ = GetOwner()->GetComponent<SkinnedModelComponent>();
	FSMComponent::Initialize();
}


void FSMComponentEnemy::OnEnterState(EState state)
{
	switch (state)
	{
	case EState::Idle:

#ifdef _DEBUG
		cout << "[Enemy] Entered Idle State." << endl;
#endif

		if (skinned_model_)
		{
			skinned_model_->GetAnimator()->PlayAnimation("Idle", true);
		}
		break;

		break;

	case EState::Attack:
#ifdef _DEBUG
		cout << "[Enemy] Entered Attack State!" << endl;
#endif
		if (skinned_model_)
		{
			skinned_model_->GetAnimator()->PlayAnimation("Attack", true);
		}
		break;

	case EState::Move:
#ifdef _DEBUG
		cout << "[Enemy] Entered Move State." << endl;
#endif
		if (skinned_model_)
		{
			skinned_model_->GetAnimator()->PlayAnimation("Run", true);
		}
		break;

	default:
		break;
	}
}

void FSMComponentEnemy::OnUpdateState(EState state, float dt)
{
	switch (state)
	{
	case EState::Idle:
		break;

	case EState::Attack:

	break;

	default:
		break;
	}
}

void FSMComponentEnemy::OnExitState(EState state)
{
	switch (state)
	{
	case EState::Idle:

		break;

	case EState::Attack:
#ifdef _DEBUG
		cout << "[Enemy] Attack Finished." << endl;
#endif
		break;

	default:
		break;
	}
}
///EOF FSMComponentEnemy.cpp