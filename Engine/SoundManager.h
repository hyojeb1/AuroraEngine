#pragma once

#include "Singleton.h"

class SoundManager : public Singleton<SoundManager>
{
public:
	friend class Singleton<SoundManager>;

	void Initialize();
	void Update();
	void Finalize();

	void Stop_ChannelGroup();
	void Release_ChannelGroup();

	void SetVolume_Main(float volume); //other volume = mainV * otherV;
	void SetVolume_BGM(float volume);
	void SetVolume_SFX(float volume);
	void SetVolume_UI(float volume);

	float GetVolume_Main() { return m_Volume_Main; }
	float GetVolume_BGM() { return m_Volume_BGM; }
	float GetVolume_SFX() { return m_Volume_SFX; }
	float GetVolume_UI() { return m_Volume_UI; }


	void ConvertBGMSource(std::string filename, bool isStream);
	void ConvertSFXSource(std::string filename);
	void ConvertUISource (std::string filename);

	FMOD::System* GetCoreSystem() { return m_CoreSystem; }
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

	std::unordered_map<std::string, FMOD::Sound*> L_BGM;
	std::unordered_map<std::string, FMOD::Sound*> L_SFX;
	std::unordered_map<std::string, FMOD::Sound*> L_UI;
};

