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
#include "CommonVS.hlsli"

#ifndef __COMMON_SKINNING_HLSLI__
#define __COMMON_SKINNING_HLSLI__

float4 Skinning(float4 pos, float4 weight, uint4 index)
{
    float4 skinVtx = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // Standard Skinning Logic
    // index values are safe as long as they are < MAX_BONES (256) defined in CommonVS
    skinVtx += weight.x * mul(pos, BoneTransforms[index.x]);
    skinVtx += weight.y * mul(pos, BoneTransforms[index.y]);
    skinVtx += weight.z * mul(pos, BoneTransforms[index.z]);
    skinVtx += weight.w * mul(pos, BoneTransforms[index.w]);

    return skinVtx;
}
#endif
/// EOF CommonSkinning.hlsli