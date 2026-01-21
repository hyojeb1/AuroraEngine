/// BOF Animator.h
#pragma once
#include "Resource.h"

struct TransformData
{
	DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 rotation = { 0.0f, 0.0f, 0.0f, 1.0f }; //Quarternion
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
};


class Animator 
{
private:
	AnimationClip* current_clip_	= nullptr;
	float	current_time_									= 0.0f;		
	bool	is_loop_										= true; 

	AnimationClip* previous_clip_	= nullptr;
	float	previous_time_									= 0.0f;
	bool	is_previous_loop_								= true;	
	float	blend_factor_									= 0.0f;
	float	blend_duration_									= 0.0f;
	bool	is_blending_									= false;

	std::vector<DirectX::XMMATRIX>	final_bone_matrices_	= {};	
	const Model*				model_context_			= nullptr;	

public:
	explicit Animator(const Model* model);
	
	void UpdateAnimation(float delta_time);
	void PlayAnimation(const std::string& clip_name, bool is_loop = true, float blend_time = 0.2f);

	const std::vector<DirectX::XMMATRIX>& GetFinalBoneMatrices() const { return final_bone_matrices_; }

	const std::string GetCurrentAnimationName() const;

private:
	AnimationClip* FindClipByName(const std::string& clip_name) const;
	void CalculateBoneTransform(const std::shared_ptr<SkeletonNode>& node, const DirectX::XMMATRIX& parent_transform);

	static TransformData SampleTransform(const AnimationClip* clip, const std::shared_ptr<SkeletonNode> node, float time_position);
	static TransformData BlendTransform(const TransformData& from, const TransformData& to, float blend_factor);
	static TransformData DecomposeTransform(const DirectX::XMMATRIX& matrix);
	static DirectX::XMMATRIX ComposeTransform(const TransformData& transform);

};


///EOF Animator.h