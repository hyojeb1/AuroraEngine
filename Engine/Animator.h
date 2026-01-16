///
/// BOF Animator.h
/// 
/// 의도: Resource.h안에 넣는 것이 더 자연스럽다고 생각함
/// 하지만 Resource.h가 너무 커지므로 클래스를 분리하고 vcxproj의 필터로 비슷함을 표현함
/// 
/// 사용처: 스킨드모델컴포넌트
///
#pragma once
#include "Resource.h"

struct TransformData
{
	DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
};


class Animator 
{
private:
	const AnimationClip* m_c ;


};


///EOF Animator.h
