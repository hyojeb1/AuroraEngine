#include "stdafx.h"
#include "MainCamera.h"

using namespace DirectX;

XMMATRIX MainCamera::GetViewMatrix() const
{
	constexpr XMVECTOR forwardVector = { 0.0f, 0.0f, 1.0f, 0.0f };
	constexpr XMVECTOR upVector = { 0.0f, 1.0f, 0.0f, 0.0f };

	return XMMatrixLookAtLH(m_position, XMVectorAdd(m_position, forwardVector), upVector);
}

XMMATRIX MainCamera::GetProjectionMatrix() const
{
	return XMMatrixPerspectiveFovLH(m_fovY, m_aspectRatio, m_nearZ, m_farZ);
}
