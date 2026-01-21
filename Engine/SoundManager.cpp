#include "stdafx.h"

#include <numbers>
#include "GameObjectBase.h"
#include "ListenerComponent.h"

#include "SoundManager.h"

constexpr size_t ChannelCount = 64; //profiling 필요

void SoundManager::Initialize()
{
	m_Volume_Main = 1.0f;
	m_Volume_BGM = 1.0f;
	m_Volume_SFX = 1.0f;
	m_Volume_UI = 1.0f;

	FMOD_RESULT result;
	if (!m_CoreSystem)
	{
		result = FMOD::System_Create(&m_CoreSystem);
		if (result != FMOD_OK) { FMOD_LOG(result); FMOD_ASSERT(result); return; }

#ifdef _DEBUG
		result = m_CoreSystem->init(ChannelCount,
			FMOD_INIT_NORMAL |
			FMOD_INIT_VOL0_BECOMES_VIRTUAL |
			FMOD_INIT_PROFILE_ENABLE
			, nullptr);
#else
		result = m_CoreSystem->init(ChannelCount,
			FMOD_INIT_NORMAL |
			FMOD_INIT_VOL0_BECOMES_VIRTUAL
			, nullptr);

#endif
		if (result != FMOD_OK) { FMOD_LOG(result); FMOD_ASSERT(result); return; }

		m_CoreSystem->getMasterChannelGroup(&m_MainGroup);

		if (!m_BGMGroup) m_CoreSystem->createChannelGroup("BGM", &m_BGMGroup);
		if (!m_SFXGroup) m_CoreSystem->createChannelGroup("SFX", &m_SFXGroup);
		if (!m_UIGroup)  m_CoreSystem->createChannelGroup("UI", &m_UIGroup);

		m_MainGroup->addGroup(m_BGMGroup);
		m_MainGroup->addGroup(m_SFXGroup);
		m_MainGroup->addGroup(m_UIGroup);

		ConvertBGMSource();
		ConvertSFXSource();
		ConvertUISource();

		for (auto& n : BGM_List)
		{
			std::cout << n.first << std::endl;
		}

		for (auto& n : SFX_List)
		{
			std::cout << n.first << std::endl;
		}

		for (auto& n : UI_List)
		{
			std::cout << n.first << std::endl;
		}

		CreateNodeData("Sample1_Beat(Kick)");

		m_CoreSystem->update();
	}
}

void SoundManager::Update()
{
	m_CoreSystem->update();
}

void SoundManager::Stop_ChannelGroup()
{
	if (m_BGMGroup) m_BGMGroup->stop();
	if (m_SFXGroup) m_SFXGroup->stop();
	if (m_UIGroup)  m_UIGroup->stop();
}
void SoundManager::Release_ChannelGroup()
{
	if (m_BGMGroup) m_BGMGroup->release();
	if (m_SFXGroup) m_SFXGroup->release();
	if (m_UIGroup)  m_UIGroup->release();
}

void SoundManager::Finalize()
{
	Stop_ChannelGroup();
	Release_ChannelGroup();

	if (m_CoreSystem)
	{
		m_CoreSystem->close();
		m_CoreSystem->release();
	}
}


void SoundManager::SetVolume_Main(float volume)
{
	if (volume < 0) { volume = 0; }
	else if (volume > 1.0f) { volume = 1.0f; }
	m_Volume_Main = volume;

	m_MainGroup->setVolume(m_Volume_Main);
}

void SoundManager::SetVolume_BGM(float volume)
{
	if (volume < 0) { volume = 0; }
	else if (volume > 1.0f) { volume = 1.0f; }
	m_Volume_BGM = m_Volume_Main * volume;

	m_BGMGroup->setVolume(m_Volume_BGM);
}

void SoundManager::SetVolume_SFX(float volume)
{
	if (volume < 0) { volume = 0; }
	else if (volume > 1.0f) { volume = 1.0f; }
	m_Volume_SFX = m_Volume_Main * volume;

	m_SFXGroup->setVolume(m_Volume_SFX);
}

void SoundManager::SetVolume_UI(float volume)
{
	if (volume < 0) { volume = 0; }
	else if (volume > 1.0f) { volume = 1.0f; }
	m_Volume_UI = m_Volume_Main * volume;

	m_UIGroup->setVolume(m_Volume_UI);
}

void SoundManager::ConvertBGMSource()
{
	const std::filesystem::path BGMDirectory = "../Asset/Sound/BGM/";

	for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(BGMDirectory))
	{
		if (dirEntry.is_regular_file())
		{
			std::string fileName = std::filesystem::path(dirEntry).stem().string();

			FMOD::Sound* temp;
			std::string fullPath = dirEntry.path().string();
			m_CoreSystem->createSound(fullPath.c_str(), FMOD_CREATESAMPLE |
				FMOD_LOOP_OFF |
				FMOD_2D |
				FMOD_ACCURATETIME, nullptr, &temp);

			BGM_List.emplace(fileName, temp);
		}
	}
}

void SoundManager::ConvertSFXSource()
{
	const std::filesystem::path SFXDirectory = "../Asset/Sound/SFX/";

	for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(SFXDirectory))
	{
		if (dirEntry.is_regular_file())
		{
			std::string fileName = std::filesystem::path(dirEntry).stem().string();

			FMOD::Sound* temp;
			std::string fullPath = dirEntry.path().string();
			m_CoreSystem->createSound(fullPath.c_str(), FMOD_DEFAULT | FMOD_3D, nullptr, &temp);

			SFX_List.emplace(fileName, temp);
		}
	}
}

void SoundManager::ConvertUISource()
{
	const std::filesystem::path UIDirectory = "../Asset/Sound/UI/";

	for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(UIDirectory))
	{
		if (dirEntry.is_regular_file())
		{
			std::string fileName = std::filesystem::path(dirEntry).stem().string();

			FMOD::Sound* temp;
			std::string fullPath = dirEntry.path().string();
			m_CoreSystem->createSound(fullPath.c_str(), FMOD_DEFAULT | FMOD_2D, nullptr, &temp);

			UI_List.emplace(fileName, temp);
		}
	}
}

float Hann(int n, int N)
{
	return 0.5f * (1.0f - cosf(2.0f * std::numbers::pi * n / (N - 1)));
}

struct Segment
{
	float start;
	float end;
};

void SoundManager::CreateNodeData(const std::string& filename)
{
	auto it = BGM_List.find(filename);
	if (it == BGM_List.end())
		return;

	FMOD::Sound* sound = it->second;

	FMOD_SOUND_FORMAT format;
	int channels, bits;
	sound->getFormat(nullptr, &format, &channels, &bits);

	float sampleRate = 0.0f;
	sound->getDefaults(&sampleRate, nullptr);

	unsigned int pcmBytes = 0;
	sound->getLength(&pcmBytes, FMOD_TIMEUNIT_PCMBYTES);

	void* ptr1 = nullptr;
	void* ptr2 = nullptr;
	unsigned int len1 = 0, len2 = 0;

	sound->lock(0, pcmBytes, &ptr1, &ptr2, &len1, &len2);

	std::vector<float> pcmFloat;

	if (format == FMOD_SOUND_FORMAT_PCM16)
	{
		int totalSamples = len1 / sizeof(int16_t);
		int16_t* pcm16 = static_cast<int16_t*>(ptr1);

		pcmFloat.resize(totalSamples / channels);

		if (channels == 2)
		{
			for (size_t i = 0; i < pcmFloat.size(); i++)
			{
				pcmFloat[i] =
					(pcm16[i * 2] + pcm16[i * 2 + 1]) / 65536.0f;
			}
		}
		else if (channels == 1)
		{
			for (size_t i = 0; i < pcmFloat.size(); i++)
			{
				pcmFloat[i] = pcm16[i] / 32768.0f;
			}
		}
	}

	sound->unlock(ptr1, ptr2, len1, len2);

	if (pcmFloat.empty())
		return;

	const int fftSize = 1024;
	const int hopSize = 512;

	float* fftIn = (float*)fftwf_malloc(sizeof(float) * fftSize);
	fftwf_complex* fftOut =
		(fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * (fftSize / 2 + 1));

	fftwf_plan fftPlan =
		fftwf_plan_dft_r2c_1d(
			fftSize,
			fftIn,
			fftOut,
			FFTW_MEASURE
		);

	float minHz = 40.0f;
	float maxHz = 110.0f;

	int minBin = (int)(minHz * fftSize / sampleRate);
	int maxBin = (int)(maxHz * fftSize / sampleRate);

	bool active = false;
	float startTime = 0.0f;
	float onsetEnergy = 0.0f;
	float prevEnergy = 0.0f;

	float deltaSum = 0.0f;
	int deltaCount = 0;

	// 튜닝 파라미터
	float deltaMultiplier = 1.1f;   // 킥 민감도
	float decayRatio = 0.3f;        // 킥 길이 (30%까지 감쇠)

	// 킥 피크 검출용
	float prev2Energy = 0.0f;
	float prev1Energy = 0.0f;

	std::vector<Segment> segments;

	const float kickEnergyThreshold = 5000.0f;
	const float kickLength = 0.10f;
	const float minInterval = 0.10f;

	int warmupFrames = 5;
	int frameIndex = 0;

	for (size_t offset = 0;
		offset + fftSize <= pcmFloat.size();
		offset += hopSize)
	{
		frameIndex++;

		for (int i = 0; i < fftSize; i++)
			fftIn[i] = pcmFloat[offset + i] * Hann(i, fftSize);

		fftwf_execute(fftPlan);

		float energy = 0.0f;
		for (int k = minBin; k <= maxBin; k++)
		{
			float r = fftOut[k][0];
			float im = fftOut[k][1];
			energy += r * r + im * im;
		}

		if (frameIndex <= warmupFrames)
		{
			prev2Energy = prev1Energy;
			prev1Energy = energy;
			continue;
		}

		float timeSec = (float)offset / sampleRate;

		bool isKickPeak =
			(prev1Energy > prev2Energy) &&
			(prev1Energy > energy) &&
			(prev1Energy > kickEnergyThreshold);

		if (isKickPeak)
		{
			float kickStart =
				timeSec - (fftSize * 0.5f / sampleRate);
			float kickEnd = kickStart + kickLength;

			if (segments.empty() ||
				kickStart - segments.back().start > minInterval)
			{
				segments.push_back({ kickStart, kickEnd });
			}
		}

		prev2Energy = prev1Energy;
		prev1Energy = energy;
	}

	if (active)
	{
		float endTime = (float)pcmFloat.size() / sampleRate;
		segments.push_back({ startTime, endTime });
	}

	std::vector<Segment> filtered;
	for (auto& s : segments)
	{
		if (s.end - s.start >= 0.03f) // 30ms
			filtered.push_back(s);
	}

	fftwf_destroy_plan(fftPlan);
	fftwf_free(fftIn);
	fftwf_free(fftOut);

	nlohmann::json root;
	root["band"] = "kick";
	root["rangeHz"] = { minHz, maxHz };
	root["segments"] = nlohmann::json::array();

	for (auto& s : filtered)
	{
		root["segments"].push_back({
			{ "start", s.start },
			{ "end",   s.end }
			});
	}

	std::string outPath = "../Asset/BeatMapData/" + filename + "_nodes.json";
	std::ofstream out(outPath);
	out << root.dump(4);
	out.close();
}

void SoundManager::BGM_Shot(const std::string filename)
{
	auto it = BGM_List.find(filename);

	if (it != BGM_List.end())
	{
		m_CoreSystem->playSound(it->second, m_BGMGroup, false, &m_BGMChannel);
	}
}

void SoundManager::SFX_Shot(const DirectX::XMVECTOR pos, const std::string filename)
{
	FMOD::Channel* pChannel = nullptr;

	auto it = SFX_List.find(filename);

	if (it != SFX_List.end())
	{
		m_CoreSystem->playSound(it->second, m_SFXGroup, false, &pChannel);
		FMOD_VECTOR tempPos = ToFMOD(pos);
		FMOD_VECTOR vel{ 0,0,0 };
		pChannel->set3DAttributes(&tempPos, &vel);
	}
}

void SoundManager::UI_Shot(const std::string filename)
{
	FMOD::Channel* pChannel = nullptr;

	auto it = UI_List.find(filename);

	if (it != UI_List.end())
	{
		m_CoreSystem->playSound(it->second, m_UIGroup, false, &pChannel);
	}
}

FMOD_VECTOR SoundManager::ToFMOD(DirectX::XMVECTOR vector)
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMStoreFloat3(&pos, vector);

	FMOD_VECTOR FMOD_pos;
	FMOD_pos.x = pos.x;
	FMOD_pos.y = pos.y;
	FMOD_pos.z = pos.z;

	return FMOD_pos;
}

//void SoundManager::UpdateSoundResourceUsage()
//{
//	m_CoreSystem->getCPUUsage(&m_Usage);
//}

void SoundManager::UpdateListener(ListenerComponent* listener)
{
	FMOD_VECTOR pos = ToFMOD(listener->GetOwner()->GetPosition());
	FMOD_VECTOR vel{ 0,0,0 };
	FMOD_VECTOR fwd{ 0,0,1 };
	FMOD_VECTOR up{ 0,1,0 };

	m_CoreSystem->set3DListenerAttributes(0, &pos, &vel, &fwd, &up);
}
