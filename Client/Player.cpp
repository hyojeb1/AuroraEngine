#include "stdafx.h"
#include "Player.h"

#include "TimeManager.h"
#include "InputManager.h"
#include "ColliderComponent.h"
#include "CameraComponent.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "CamRotObject.h"
#include "ModelComponent.h"
#include "Enemy.h"

#include "GunObject.h"

REGISTER_TYPE(Player)

using namespace std;
using namespace DirectX;

void Player::Initialize()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();
	m_lineVertexBufferAndShader = resourceManager.GetVertexShaderAndInputLayout("VSLine.hlsl");
	m_linePixelShader = resourceManager.GetPixelShader("PSColor.hlsl");

	m_crosshairSRV = resourceManager.GetTexture("Crosshair.png");

	// srv에서 크기 정보 얻기
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	m_crosshairSRV->GetDesc(&srvDesc);
	if (srvDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D)
	{
		D3D11_TEX2D_SRV tex2DSRV = srvDesc.Texture2D;
		com_ptr<ID3D11Resource> resource = nullptr;
		m_crosshairSRV->GetResource(resource.GetAddressOf());
		com_ptr<ID3D11Texture2D> texture2D = nullptr;
		resource.As(&texture2D);
		D3D11_TEXTURE2D_DESC textureDesc = {};
		texture2D->GetDesc(&textureDesc);
		m_crosshairOffset.x = static_cast<float>(textureDesc.Width) * 0.5f;
		m_crosshairOffset.y = static_cast<float>(textureDesc.Height) * 0.5f;
	}

	m_cameraObject = GetChildGameObject("CamRotObject_2");
	m_gunObject = m_cameraObject->GetChildGameObject("Gun");
}

void Player::Update()
{
	float deltaTime = TimeManager::GetInstance().GetDeltaTime();
	InputManager& input = InputManager::GetInstance();

	if (input.GetKey(KeyCode::W)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Forward);
	if (input.GetKey(KeyCode::S)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Backward);
	if (input.GetKey(KeyCode::A)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Left);
	if (input.GetKey(KeyCode::D)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Right);
	if (input.GetKey(KeyCode::Space)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Up);
	if (input.GetKey(KeyCode::Shift)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Down);

	if (input.GetKeyDown(KeyCode::MouseLeft))
	{
		float distance = 0.0f;

		if (auto gun = dynamic_cast<GunObject*>(m_gunObject))
		{
			gun->Fire();
		}

		const CameraComponent& mainCamera = CameraComponent::GetMainCamera();
		const XMVECTOR& origin = mainCamera.GetPosition();
		const XMVECTOR& direction = mainCamera.GetForwardVector();
		GameObjectBase* hit = ColliderComponent::CheckCollision(origin, direction, distance);
		if (hit)
		{
			if (dynamic_cast<Enemy*>(hit)) hit->SetAlive(false);

			LineBuffer lineBuffer = {};
			XMStoreFloat4(&lineBuffer.linePoints[0], m_gunObject->GetWorldPosition());
			lineBuffer.lineColors[0] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
			XMStoreFloat4(&lineBuffer.linePoints[1], XMVectorAdd(origin, XMVectorScale(direction, distance)));
			lineBuffer.lineColors[1] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };

			m_lineBuffers.emplace_back(lineBuffer, 0.5f);
		}
	}
	if (input.GetKeyDown(KeyCode::MouseRight))
	{
		m_enemyIndicators.clear();

		const DXGI_SWAP_CHAIN_DESC1& swapChainDesc = Renderer::GetInstance().GetSwapChainDesc();
		float halfWidth = static_cast<float>(swapChainDesc.Width / 2);
		float halfHeight = static_cast<float>(swapChainDesc.Height / 2);

		const CameraComponent& mainCamera = CameraComponent::GetMainCamera();
		vector<GameObjectBase*> hits = ColliderComponent::CheckCollision(mainCamera.GetBoundingFrustum());
		vector<tuple<float, XMFLOAT2, Enemy*>> enemyHits;

		for (GameObjectBase* hit : hits)
		{
			if (Enemy* enemy = dynamic_cast<Enemy*>(hit))
			{
				XMFLOAT2 distancePair = mainCamera.WorldToScreenPosition(enemy->GetWorldPosition());

				enemyHits.emplace_back(powf(distancePair.x - halfWidth, 2) + powf(distancePair.y - halfHeight, 2), distancePair, enemy);
			}
		}
		sort(enemyHits.begin(), enemyHits.end(), [](const auto& a, const auto& b) { return get<0>(a) < get<0>(b); });

		for (size_t i = 0; i < min(enemyHits.size(), static_cast<size_t>(6)); ++i)
		{
			get<2>(enemyHits[i])->SetAlive(false);

			LineBuffer lineBuffer = {};
			XMStoreFloat4(&lineBuffer.linePoints[0], m_gunObject->GetWorldPosition());
			lineBuffer.lineColors[0] = XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f };
			XMStoreFloat4(&lineBuffer.linePoints[1], get<2>(enemyHits[i])->GetWorldPosition());
			lineBuffer.lineColors[1] = XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f };

			m_lineBuffers.emplace_back(lineBuffer, 0.5f);
			m_enemyIndicators.emplace_back(get<1>(enemyHits[i]), 0.5f);
		}
	}

	for_each(m_lineBuffers.begin(), m_lineBuffers.end(), [&](auto& pair) { pair.second -= deltaTime; });
	if (!m_lineBuffers.empty() && m_lineBuffers.front().second < 0.0f) m_lineBuffers.pop_front();

	for_each(m_enemyIndicators.begin(), m_enemyIndicators.end(), [&](auto& pair) { pair.second -= deltaTime; });
	if (!m_enemyIndicators.empty() && m_enemyIndicators.front().second < 0.0f) m_enemyIndicators.pop_front();
}

void Player::Render()
{
	Renderer& renderer = Renderer::GetInstance();

	// 크로스헤어 UI 렌더링
	renderer.UI_RENDER_FUNCTIONS().emplace_back
	(
		[&]()
		{
			constexpr XMFLOAT2 center = { 0.5f, 0.5f };
			Renderer::GetInstance().RenderImageUIPosition(m_crosshairSRV, center, m_crosshairOffset);
		}
	);

	if (!m_lineBuffers.empty())
	{
		renderer.RENDER_FUNCTION(RenderStage::Scene, BlendState::Opaque).emplace_back
		(
			0.0f,
			[&]()
			{
				ResourceManager& resourceManager = ResourceManager::GetInstance();
				com_ptr<ID3D11DeviceContext> deviceContext = Renderer::GetInstance().GetDeviceContext();

				deviceContext->IASetInputLayout(m_lineVertexBufferAndShader.second.Get());
				deviceContext->VSSetShader(m_lineVertexBufferAndShader.first.Get(), nullptr, 0);
				deviceContext->PSSetShader(m_linePixelShader.Get(), nullptr, 0);

				resourceManager.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

				for (const auto& [lineBuffer, time] : m_lineBuffers)
				{
					deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBuffer, 0, 0);
					deviceContext->Draw(2, 0);
				}
			}
		);
	}

	if (!m_enemyIndicators.empty())
	{
		renderer.UI_RENDER_FUNCTIONS().emplace_back
		(
			[&]()
			{
				for (const auto& [position, time] : m_enemyIndicators) Renderer::GetInstance().RenderImageScreenPosition(m_crosshairSRV, position, m_crosshairOffset, 0.5f);
			}
		);
	}
}