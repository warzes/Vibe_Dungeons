#pragma once

#include <string>
#include "core/json_alias.h"

class AudioSystem;
class JsonDataManager;

class AudioManager final
{
public:
	AudioManager(AudioSystem& audio, JsonDataManager& jsonData);
	~AudioManager() noexcept = default;

	AudioManager(const AudioManager&) = delete;
	AudioManager& operator=(const AudioManager&) = delete;

	void PlaySound(const std::string& soundId);
	void PlayMusic(const std::string& musicId);
	void StopMusic();
	void SetMusicVolume(float vol);
	void SetSfxVolume(float vol);
	void LoadSoundDefinitions();

private:
	[[nodiscard]] const json* findSound(const std::string& soundId) const;

	AudioSystem& m_audio;
	JsonDataManager& m_jsonData;
	std::string m_currentMusic;
};
