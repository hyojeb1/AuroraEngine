#pragma once
#include "Singleton.h"

class ListenerComponent;

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

	void CreateNodeData(const std::string& filename);
	void LoadNodeData();
	void UpdateNodeIndex();
	void UpdateUINodeIndex();
	bool CheckRhythm(float correction);

	std::unordered_map<std::string, std::vector<std::pair<float, float>>>* GetNodeDataPtr() { return &m_NodeData; }

	void Main_BGM_Shot(const std::string filename);
	void Sub_BGM_Shot(const std::string filename, float delay);
	void SFX_Shot(const DirectX::XMVECTOR pos, const std::string filename);
	void UI_Shot(const std::string filename);

	void FadeIn(FMOD::Channel* chan, float sec);
	void FadeOut(FMOD::Channel* chan, float sec, bool stopAfter);

	void UpdateListener(ListenerComponent* listener);
	
	void SetVolume_Main(float volume); //other volume = mainV * otherV;
	void SetVolume_BGM(float volume);
	void SetVolume_SFX(float volume);
	void SetVolume_UI(float volume);

	float GetVolume_Main() const { return m_Volume_Main; }
	float GetVolume_BGM() const { return m_Volume_BGM; }
	float GetVolume_SFX() const { return m_Volume_SFX; }
	float GetVolume_UI() const { return m_Volume_UI; }

	float GetCurrentPlaybackTime();

	size_t GetRhythmTimerIndex() { return m_rhythmTimerIndex; };
	void   ResetRhythmIndex() { m_rhythmTimerIndex = 0; }
	float GetRhythmOffset() { return m_RhythmOffSet; }

	FMOD_VECTOR ToFMOD(DirectX::XMVECTOR vector);

	bool ConsumeNodeChanged();

	FMOD::Channel* GetBGMCh1() { return m_BGMChannel1; }
	FMOD::Channel* GetBGMCh2() { return m_BGMChannel2; }
	//void UpdateSoundResourceUsage();

	void UpdateLowpass();
	void ChangeLowpass() { m_IsLowpass = !m_IsLowpass; }

private:

	FMOD::System* m_CoreSystem = nullptr;

	FMOD::Channel* m_BGMChannel1 = nullptr;
	FMOD::Channel* m_BGMChannel2 = nullptr;

	std::unordered_map<std::string, std::vector<std::pair<float, float>>> m_NodeData;
	std::unordered_map<std::string, std::vector<std::pair<float, float>>> m_SubNodeData;

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

	std::string m_CurrentTrackName{};

	size_t m_rhythmTimerIndex = 0;	//raw time
	size_t m_rhythmUIIndex = 0;
	//FMOD_CPU_USAGE m_Usage;

	FMOD::DSP* m_lowpass = nullptr;
	bool m_IsLowpass = false;
	float m_lowpassCutOff = 22000.0f;

	unsigned int m_MainBGM_StartTime = 0;

	float m_RhythmOffSet = 1.28f;

	bool m_OnNodeChanged = false;
};

