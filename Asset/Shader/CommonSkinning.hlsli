//
// CommonSkinning.hlsli
//
// 본 최대 개수 등 상수는 한곳에서 관리하려고 한다.
//
// 본 최대 개수) 
// 예나는 20개인데... 연구필요 
// 단 아트팀에게 어떻게 제한할 것인가가 중요한 문제이다.
//
// 상수 버퍼)
// 상수 버퍼의 최대 크기는 64KB(4096 벡터)이다. 
// 256개(1024 벡터)로 선언했으므로 크기는 이상 없음
// 상수버퍼 슬롯에 대한 고민이 또 있다. 

#ifndef __COMMON_SKINNING_HLSLI__
#define __COMMON_SKINNING_HLSLI__

#define MAX_BONES 256
cbuffer BoneState : register(b3)
{
    matrix BoneTransforms[MAX_BONES];
};


// feature:     4성분 벡터 섞는 함수
// parameter:   pos는 nrm, bi_nrm, tan 역시 와야 함
float4 Skinning(float4 pos, float4 weight, uint4 index)
{
    float4 skinVtx;

    float4 v0 = mul(weight.x, mul(pos, BoneTransforms[index.x]));
    float4 v1 = mul(weight.y, mul(pos, BoneTransforms[index.y]));
    float4 v2 = mul(weight.z, mul(pos, BoneTransforms[index.z]));
    float4 v3 = mul(weight.w, mul(pos, BoneTransforms[index.w]));
    
                             
    skinVtx = v0 + v1 + v2 + v3;
    
    
    return skinVtx;
}


#endif
/// EOF CommonSkinning.hlsli