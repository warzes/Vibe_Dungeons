#pragma once

struct AudioClip final
{
	std::vector<float> samples; //< decoded float PCM
	int32_t sampleRate = 0;
	int32_t channels = 0;
};

enum class AudioChannel : uint8_t
{
	Music,
	SFX
};

class AudioSystem final
{
public:
	AudioSystem() noexcept;
	~AudioSystem() noexcept;

	AudioSystem(const AudioSystem&) = delete;
	AudioSystem& operator=(const AudioSystem&) = delete;

	[[nodiscard]] bool Valid() const noexcept { return m_valid; }

	AudioClip* LoadWAV(std::string_view path);
	AudioClip* LoadOGG(std::string_view path);
	AudioClip* Load(std::string_view path);

	void Update() noexcept;
	void Play(std::string_view name, AudioChannel channel = AudioChannel::SFX, float volume = 1.0f) noexcept;
	void Stop(std::string_view name) noexcept;
	void StopAll() noexcept;
	void StopChannel(AudioChannel channel) noexcept;
	void SetChannelVolume(AudioChannel channel, float volume) noexcept;

	[[nodiscard]] bool IsPlaying(std::string_view name) const noexcept;
	[[nodiscard]] AudioClip* GetClip(std::string_view name) noexcept;

private:
	struct PlayingClip final
	{
		std::string name;
		AudioChannel channel;
		uint64_t endBytePos; //< stream byte position where this clip ends
	};

	SDL_AudioStream* getOrCreateStream(AudioChannel channel) noexcept;

	bool m_valid = false;
	SDL_AudioStream* m_musicStream = nullptr;
	SDL_AudioStream* m_sfxStream = nullptr;
	float m_musicVolume = 1.0f;
	float m_sfxVolume = 1.0f;
	uint64_t m_musicStreamPos = 0;
	uint64_t m_sfxStreamPos = 0;

	std::unordered_map<std::string, AudioClip> m_clips;
	std::vector<float> m_scratch;
	std::vector<PlayingClip> m_playing;
};
