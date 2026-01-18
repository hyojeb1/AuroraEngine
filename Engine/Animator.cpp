///
/// BOF Animator.cpp
///
#include "stdafx.h"
#include "Animator.h"

using namespace DirectX;
using namespace std;

namespace HELPER_IN_ANIMATOR_CPP
{
float WrapAnimationTime(float time, float duration)
{
	if (duration <= 0.0f) return 0.0f;
	float wrapped = fmodf(time, duration);
	return wrapped < 0.f ? wrapped + duration : wrapped;
}

VectorKeyframe GetVectorKeyframe(const std::vector<VectorKeyframe>& keys, float time_position, const XMFLOAT3& default_value)
{
	// 1. 예외 처리(데이터 없음)
	if (keys.empty())
	{
		return { time_position, default_value };
	}

	// 2. 프레임이 하나 OR 요청한 시간이 더 앞일 경우
	// 첫 프레임 값을 리턴함
	if (keys.size() == 1 || time_position <= keys.front().time_position)
	{
		return keys.front();
	}

	// 3. 구간 탐색 및 보간
	for (size_t i = 0; i + 1 < keys.size(); ++i)
	{
		if (time_position < keys[i + 1].time_position)
		{
			const float segment_start	= keys[i].time_position;
			const float segment_end		= keys[i + 1].time_position;

			const float scale_factor	= (segment_end - segment_start) > 0.0f ? (time_position - segment_start)/ (segment_end - segment_start) : 0.f;

			XMVECTOR from				= XMLoadFloat3(&keys[i].value);
			XMVECTOR to					= XMLoadFloat3(&keys[i + 1].value);

			XMVECTOR blended			= XMVectorLerp(from, to, scale_factor);

			VectorKeyframe result		= {};
			result.time_position		= time_position;
			XMStoreFloat3(&result.value, blended);
			return result;
		}
	}

	// 4. 요청한 시간이 더 뒤일 경우
	// 마지막 프레임 값을 리턴함
	return keys.back();
}

QuaternionKeyframe GetQuaternionKeyframe(const std::vector<QuaternionKeyframe>& keys, float time_position, const XMFLOAT4& default_value)
	{
		// 1. 예외 처리(데이터 없음)
		if (keys.empty())
		{
			return { time_position, default_value };
		}

		// 2. 프레임이 하나 OR 요청한 시간이 더 앞일 경우
		// 첫 프레임 값을 리턴함
		if (keys.size() == 1 || time_position <= keys.front().time_position)
		{
			return keys.front();
		}

		// 3. 구간 탐색 및 보간
		for (size_t i = 0; i + 1 < keys.size(); ++i)
		{
			if (time_position < keys[i + 1].time_position)
			{
				const float segment_start	= keys[i].time_position;
				const float segment_end		= keys[i + 1].time_position;

				const float scale_factor	= (segment_end - segment_start) > 0.0f ? (time_position - segment_start)/ (segment_end - segment_start) : 0.f;

				XMVECTOR from				= XMLoadFloat4(&keys[i].value);
				XMVECTOR to					= XMLoadFloat4(&keys[i + 1].value);

				XMVECTOR blended			= XMQuaternionSlerp(from, to, scale_factor);
				blended						= XMQuaternionNormalize(blended);

				QuaternionKeyframe result		= {};
				result.time_position		= time_position;
				XMStoreFloat4(&result.value, blended);
				return result;
			}
		}

		// 4. 요청한 시간이 더 뒤일 경우
		// 마지막 프레임 값을 리턴함
		return keys.back();
	}
};

using namespace HELPER_IN_ANIMATOR_CPP;


Animator::Animator(const Model* model) : model_context_(model)
{
	if (model_context_) final_bone_matrices_.resize(model_context_->skeleton.bones.size(), XMMatrixIdentity());
}

void Animator::UpdateAnimation(float delta_time)
{
	if (!current_clip_ || !model_context_ || !model_context_->skeleton.root) return;

	// 현재 애니메이션 시간 갱신 : 블랜더 24fps
	const float current_ticks = (current_clip_->ticks_per_second > 0.0f) ? current_clip_->ticks_per_second : AnimationClip::DEFAULT_FPS;
	current_time_ += delta_time * current_ticks;
	
	if (is_loop_)										current_time_ = WrapAnimationTime(current_time_, current_clip_->duration);
	else if (current_time_ > current_clip_->duration)	current_time_ = current_clip_->duration;

	//-------------------------
	// 애니메이션 블렌딩 처리
	//-------------------------
	if (is_blending_ && previous_clip_)
	{
		// 이전 애니메이션 시간 갱신
		const float previous_ticks = (previous_clip_->ticks_per_second > 0.0f) ? previous_clip_->ticks_per_second : 24.0f;
		previous_time_ += delta_time * previous_ticks;

		if (is_previous_loop_)											previous_time_ = WrapAnimationTime(previous_time_, previous_clip_->duration);
		else if (previous_time_ > previous_clip_->duration)				previous_time_ = previous_clip_->duration;

		// 블렌딩 비율 증가
		if (blend_duration_ > 0.f) blend_factor_ += delta_time / blend_duration_;
		else blend_factor_ = 1.f;

		// 블랜딩 종료
		if (blend_factor_ >= 1.0f) {
			blend_factor_ = 1.0f;
			is_blending_ = false;
			previous_clip_ = nullptr;
		}
	}

	CalculateBoneTransform(model_context_->skeleton.root, XMMatrixIdentity());
}

void Animator::PlayAnimation(const std::string& clip_name, bool is_loop, float blend_time)
{
	if (!model_context_)	return;

	AnimationClip* next_clip = FindClipByName(clip_name);
	if (!next_clip)			return;
	if (current_clip_ == next_clip){
		is_loop_ = is_loop;
		return;
	}
	
	previous_clip_		= current_clip_;
	previous_time_		= current_time_;
	is_previous_loop_	= is_loop_;

	current_clip_		= next_clip; 
	current_time_		= 0.f;
	is_loop_			= is_loop;

	// 블랜딩 여부 결정
	if (previous_clip_ && blend_time > 0.f) {
		blend_duration_ = blend_time;
		blend_factor_	= 0.0f;
		is_blending_	= true;
	} else {
		blend_duration_ = 0.f;
		blend_factor_ = 1.0f;
		is_blending_ = false;
		previous_clip_ = nullptr;
	}
	
}

AnimationClip* Animator::FindClipByName(const std::string& clip_name) const
{
	if (!model_context_) return nullptr;

	for (const auto& clip : model_context_->animations) {
		if (clip.name == clip_name) {
			// 원본 데이터(벡터 안에 있는 놈)의 주소를 리턴해야 합니다.
			return const_cast<AnimationClip*>(&clip);
		}
	}

	return nullptr;
}

/// <summary>
///	애니메이션 시스템의 심장
/// 뼈대 계층 구조를 타고 내려가며 최종행렬을 계산하는 역할
/// </summary>
/// <param name="node"></param>
/// <param name="parent_transform"></param>
void Animator::CalculateBoneTransform(const std::shared_ptr<SkeletonNode>& node, const DirectX::XMMATRIX& parent_transform)
{
	if (!node) return;

	// 현재 자세 계산
	TransformData local_transform = SampleTransform(current_clip_, node, current_time_);

	// 애니메이션 블랜딩
	if (is_blending_ && previous_clip_){
		TransformData previous_transform = SampleTransform(previous_clip_, node, previous_time_);
		local_transform = BlendTransform(previous_transform, local_transform, blend_factor_);
	}

	// 행렬 결합 (Local -> Global)
	XMMATRIX local_matrix		= ComposeTransform(local_transform);
	XMMATRIX global_transform	= local_matrix * parent_transform;
	//XMMATRIX global_transform = XMMatrixIdentity();

	// 스키닝 행렬 계산 (Offset Matrix과 global_inverse을 적용)
	if (node->boneIndex >= 0 && static_cast<size_t>(node->boneIndex) < final_bone_matrices_.size()){
		XMMATRIX offset_matrix = XMLoadFloat4x4(&model_context_->skeleton.bones[node->boneIndex].offset_matrix);
		XMMATRIX global_inverse = XMLoadFloat4x4(&model_context_->skeleton.globalInverseTransform);

		//final_bone_matrices_[node->boneIndex] = offset_matrix * global_transform;// *global_inverse; // 왜 있으나 마나지...?
		final_bone_matrices_[node->boneIndex] = offset_matrix * global_transform *global_inverse;
	}

	// 자식 뼈들에게 "나의 globalTransform이 너희들의 parentTransform이다"라고 알림
	for (const auto& child : node->children)
	{
		CalculateBoneTransform(child, global_transform);
	}
}

TransformData Animator::SampleTransform(const AnimationClip* clip, const std::shared_ptr<SkeletonNode> node, float time_position)
{
	if (!node)	 return {};
	if (!clip)	 return DecomposeTransform(XMLoadFloat4x4(&node->localTransform)); // 재생 중인 애니메이션이 없으면? -> 뼈를 원점으로 구겨넣는 게 아니라, 기본 자세(Bind Pose)를 유지

	// ▼▼▼ [추가할 로직: 이름 정제] ▼▼▼
	std::string searchName = node->name; // 원본 이름 복사

	//// "_$AssimpFbx$" 문자열이 포함되어 있는지 확인
	//size_t dummyPos = searchName.find("_$AssimpFbx$");
	//if (dummyPos != std::string::npos)
	//{
	//	// 접미사가 있다면 잘라냄 (예: "Hips_$AssimpFbx$_PreRotation" -> "Hips")
	//	searchName = searchName.substr(0, dummyPos);
	//}
	//// ▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲

	// [수정된 코드] node->name 대신 searchName(정제된 이름)으로 검색
	auto channel_iterator = clip->channels.find(searchName);

	// 채널을 못 찾았다면? -> 기본 자세(Local Transform) 유지
	if (channel_iterator == clip->channels.end())
	{
		return DecomposeTransform(XMLoadFloat4x4(&node->localTransform));
	}
	// 모든 뼈가 움직이지 않을 수 않는다. -> 뼈를 원점으로 구겨넣는 게 아니라, 기본 자세(Bind Pose)를 유지

	const BoneAnimationChannel& channel = channel_iterator->second;
	const XMFLOAT3 default_position = { 0.f, 0.f, 0.f };
	const XMFLOAT4 default_rotation = { 0.f, 0.f, 0.f, 1.f };
	const XMFLOAT3 default_scale	= { 1.f, 1.f, 1.f };

	VectorKeyframe position_key		= GetVectorKeyframe(channel.position_keys, time_position, default_position);
	QuaternionKeyframe rotation_key = GetQuaternionKeyframe(channel.rotation_keys, time_position, default_rotation);
	VectorKeyframe scale_key		= GetVectorKeyframe(channel.scale_keys, time_position, default_scale);
		
	TransformData result			= {};
	result.position					= position_key.value;
	result.rotation					= rotation_key.value;
	result.scale					= scale_key.value;
	return result;
}

TransformData Animator::BlendTransform(const TransformData& from, const TransformData& to, float blend_factor)
{
	XMVECTOR from_position				= XMLoadFloat3(&from.position);
	XMVECTOR to_position				= XMLoadFloat3(&to.position);
	XMVECTOR from_scale					= XMLoadFloat3(&from.scale);
	XMVECTOR to_scale					= XMLoadFloat3(&to.scale);
	XMVECTOR from_rotation				= XMLoadFloat4(&from.rotation);
	XMVECTOR to_rotation				= XMLoadFloat4(&to.rotation);

	XMVECTOR blended_position			= XMVectorLerp(from_position, to_position, blend_factor);
	XMVECTOR blended_scale				= XMVectorLerp(from_scale, to_scale, blend_factor);
	XMVECTOR blended_rotation			= XMQuaternionSlerp(from_rotation, to_rotation, blend_factor);

	TransformData result = {};
	XMStoreFloat3(&result.position, blended_position);
	XMStoreFloat3(&result.scale, blended_scale);
	XMStoreFloat4(&result.rotation, blended_rotation);
	
	return result;
}


TransformData Animator::DecomposeTransform(const XMMATRIX& matrix)
{
	XMVECTOR scale = {};
	XMVECTOR rotation = {};
	XMVECTOR translation = {};
	XMMatrixDecompose(&scale, &rotation, &translation, matrix);

	TransformData result = {};
	XMStoreFloat3(&result.position, translation);
	XMStoreFloat3(&result.scale, scale);
	XMStoreFloat4(&result.rotation, rotation);
	return result;
}

XMMATRIX Animator::ComposeTransform(const TransformData& transform)
{
	XMVECTOR scale = XMLoadFloat3(&transform.scale);
	XMVECTOR rotation = XMLoadFloat4(&transform.rotation);
	XMVECTOR translation = XMLoadFloat3(&transform.position);

	return XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rotation) * XMMatrixTranslationFromVector(translation);
}
/// EOF Animator.cpp
