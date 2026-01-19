#include "stdafx.h"

#include <filesystem>
#include <DirectXMath.h>

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

		for (auto& n : SFX_List)
		{
			std::cout << n.first << std::endl;
		}

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
