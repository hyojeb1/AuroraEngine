#pragma once
#include "ComponentBase.h"
#include "NetManager.h"

void ForceLink_NetworkTestComponent();

class NetworkTestComponent : public ComponentBase
{
    // UI용 상태
    uint32_t m_netId = 0;
    bool m_autoConnect = false;
    bool m_isHost = false;

    std::string m_hostIp = "127.0.0.1";
    int m_port = 7777;

    float m_sendAccum = 0.0f;
    float m_sendInterval = 0.05f;

    // 핸들러 등록 여부(중복등록 방지)
    bool m_handlersRegistered = false;

public:
    NetworkTestComponent() = default;
    ~NetworkTestComponent() override = default;

    bool NeedsUpdate() const override { return false; }
    bool NeedsRender() const override { return false; }

private:
    void Initialize() override;
    void RenderImGui() override;
    void Finalize() override;                 // ComponentBase에 없으면 제거해도 됨

    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& jsonData) override;

private:
    static constexpr NetManager::MsgId MSG_HELLO = 1;

    void RegisterHandlers();
    void UnregisterHandlers();

    void SendHello();
};
