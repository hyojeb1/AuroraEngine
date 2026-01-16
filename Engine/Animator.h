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
	DirectX::XMFLOAT4 rotation = { 0.0f, 0.0f, 0.0f, 1.0f }; //Quarternion
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
};

/// <summary>
/// 굳이 포인터로 관리를 해야 하는가?
/// 객체로 충분히 관리할만하지 않은가?
/// </summary>
class Animator 
{
private:
	// 이게 현재의 클립
	std::shared_ptr<AnimationClip>			current_clip_	= nullptr;	// shared : 모던 + 공유가능한 애니메이션?
	float	current_time_									= 0.0f;		
	bool	is_loop_										= true; 

	// 이게 이전의 클립
	// 이는 블랜딩을 위한 준비구나!
	std::shared_ptr<AnimationClip>			previous_clip_	= nullptr;
	float	previous_time_									= 0.0f;
	bool	is_previous_loop_								= true;	
	float	blend_factor_									= 0.0f;
	float	blend_duration_									= 0.0f;
	bool	is_blending_									= false;

	std::vector<DirectX::XMMATRIX>	final_bone_matrices_	= {};		// 최종적으로 본들의 행렬
	std::shared_ptr<SkinnedModel>			model_context_	= nullptr;	// 비유- 모델(배우), 이 변수(대본): 여기에는 매쉬 + 스켈레톤 + 애니메_클립, 3개가 다 있음

public:
	explicit Animator(const std::shared_ptr<SkinnedModel>model);
	
	void UpdateAnimation(float delta_time);
	void PlayAnimation(const std::string& clip_name, bool is_loop = true, float blend_time = 0.2f);

	const std::vector<DirectX::XMMATRIX>& GetFinalBoneMatrices() const { return final_bone_matrices_; }

private:
	AnimationClip* FindClipByName(const std::string& clip_name) const;
	void CalculateBoneTransform(const std::shared_ptr<SkeletonNode>& node, const DirectX::XMMATRIX& parent_transform);

	static TransformData SampleTransform(const std::shared_ptr <AnimationClip> clip, const std::shared_ptr<SkeletonNode> node, float time_position);
	static TransformData BlendTransform(const TransformData& from, const TransformData& to, float blend_factor);
	static TransformData DecomposeTransform(const DirectX::XMMATRIX& matrix);
	static DirectX::XMMATRIX ComposeTransform(const TransformData& transform);

};


///EOF Animator.h