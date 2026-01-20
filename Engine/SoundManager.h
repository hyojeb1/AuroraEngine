#pragma once
#include "Singleton.h"

class ListenerComponent;
class XMVECTOR;

struct SoundResourceUsage
{
	float dsp = 0.f;
	float stream = 0.f;
	float geometry = 0.f;
	float update = 0.f;
	float total = 0.f;
};

class SoundManager : public Singleton<SoundManager>
{
public:
	friend class Singleton<SoundManager>;

	FMOD::System* GetCoreSystem() { return m_CoreSystem; }
	
	void Initialize();
	void Update();
	void Finalize();

	void Stop_ChannelGroup();
	void Release_ChannelGroup();

	void ConvertBGMSource();
	void ConvertSFXSource();
	void ConvertUISource();

	void SFX_Shot(const DirectX::XMVECTOR pos, const std::string filename);

	void UpdateListener(ListenerComponent* listener);
	
	void SetVolume_Main(float volume); //other volume = mainV * otherV;
	void SetVolume_BGM(float volume);
	void SetVolume_SFX(float volume);
	void SetVolume_UI(float volume);

	float GetVolume_Main() { return m_Volume_Main; }
	float GetVolume_BGM() { return m_Volume_BGM; }
	float GetVolume_SFX() { return m_Volume_SFX; }
	float GetVolume_UI() { return m_Volume_UI; }

	FMOD_VECTOR ToFMOD(DirectX::XMVECTOR vector);

	//void UpdateSoundResourceUsage();

private:

	FMOD::System* m_CoreSystem = nullptr;

	FMOD::Channel* m_BGMChannel = nullptr;

	std::vector<FMOD::Channel*> m_loopSFXChannels;

	FMOD::ChannelGroup* m_MainGroup = nullptr;
	FMOD::ChannelGroup* m_BGMGroup = nullptr;
	FMOD::ChannelGroup* m_SFXGroup = nullptr;
	FMOD::ChannelGroup* m_UIGroup = nullptr;

	float m_Volume_Main = 1.0f;
	float m_Volume_BGM = 1.0f;
	float m_Volume_SFX = 1.0f;
	float m_Volume_UI = 1.0f;

	std::unordered_map<std::string, FMOD::Sound*> BGM_List;
	std::unordered_map<std::string, FMOD::Sound*> SFX_List;
	std::unordered_map<std::string, FMOD::Sound*> UI_List;

	//FMOD_CPU_USAGE m_Usage;
};

