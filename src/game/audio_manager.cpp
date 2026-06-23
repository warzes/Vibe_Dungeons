#include "stdafx.h"
#include "game/audio_manager.h"
#include "engine/audio_system.h"
#include "core/json_data_manager.h"
#include "core/logger.h"

//=============================================================================

AudioManager::AudioManager(AudioSystem& audio, JsonDataManager& jsonData)
	: m_audio(audio)
	, m_jsonData(jsonData)
{}

//=============================================================================

void AudioManager::LoadSoundDefinitions()
{
	const json& sounds = m_jsonData.AllSounds();
	if (!sounds.is_array())
	{
		Logger::Warn("AudioManager: No sound definitions to load");
		return;
	}

	for (const auto& entry : sounds)
	{
		if (!entry.contains("file"))
		{
			continue;
		}

		const std::string path = entry["file"].get<std::string>();
		m_audio.Load(path);
	}

	Logger::Info(std::string("AudioManager: Loaded ") + std::to_string(sounds.size()) + " sound definitions");
}

//=============================================================================

const json* AudioManager::findSound(const std::string& soundId) const
{
	const json& sounds = m_jsonData.AllSounds();
	if (!sounds.is_array())
	{
		return nullptr;
	}

	for (const auto& entry : sounds)
	{
		if (entry.contains("id") && entry["id"].get<std::string>() == soundId)
		{
			return &entry;
		}
	}

	return nullptr;
}

//=============================================================================

void AudioManager::PlaySound(const std::string& soundId)
{
	const json* sound = findSound(soundId);
	if (!sound)
	{
		Logger::Warn(std::string("AudioManager: Sound not found: ") + soundId);
		return;
	}

	if (!sound->contains("file"))
	{
		Logger::Warn(std::string("AudioManager: Sound '") + soundId + "' has no 'file' field");
		return;
	}

	const std::string path = (*sound)["file"].get<std::string>();

	if (!m_audio.GetClip(path))
	{
		if (!m_audio.Load(path))
		{
			Logger::Warn(std::string("AudioManager: Failed to load: ") + path);
			return;
		}
	}

	float volume = 1.0f;
	if (sound->contains("volume"))
	{
		volume = (*sound)["volume"].get<float>();
	}

	m_audio.Play(path, AudioChannel::SFX, volume);
}

//=============================================================================

void AudioManager::PlayMusic(const std::string& musicId)
{
	const json* sound = findSound(musicId);
	if (!sound)
	{
		Logger::Warn(std::string("AudioManager: Music not found: ") + musicId);
		return;
	}

	if (!sound->contains("file"))
	{
		Logger::Warn(std::string("AudioManager: Music '") + musicId + "' has no 'file' field");
		return;
	}

	const std::string path = (*sound)["file"].get<std::string>();

	if (!m_currentMusic.empty() && m_currentMusic != path)
	{
		m_audio.Stop(m_currentMusic);
	}

	if (!m_audio.GetClip(path))
	{
		if (!m_audio.Load(path))
		{
			Logger::Warn(std::string("AudioManager: Failed to load music: ") + path);
			return;
		}
	}

	float volume = 1.0f;
	if (sound->contains("volume"))
	{
		volume = (*sound)["volume"].get<float>();
	}

	m_audio.Play(path, AudioChannel::Music, volume);
	m_currentMusic = path;
}

//=============================================================================

void AudioManager::StopMusic()
{
	if (!m_currentMusic.empty())
	{
		m_audio.Stop(m_currentMusic);
		m_currentMusic.clear();
	}
}

//=============================================================================

void AudioManager::SetMusicVolume(float vol)
{
	m_audio.SetChannelVolume(AudioChannel::Music, vol);
}

//=============================================================================

void AudioManager::SetSfxVolume(float vol)
{
	m_audio.SetChannelVolume(AudioChannel::SFX, vol);
}
