#include "stdafx.h"
#include "engine/audio_system.h"
#include "core/logger.h"
#include "core/exception.h"

#define STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.h>

AudioSystem::AudioSystem() noexcept
{
	if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
	{
		Logger::Warn("AudioSystem: SDL_InitSubSystem(AUDIO) failed");
		return;
	}

	m_valid = true;
	Logger::Info("AudioSystem initialized");
}

AudioSystem::~AudioSystem() noexcept
{
	if (m_musicStream)
	{
		SDL_ClearAudioStream(m_musicStream);
		SDL_DestroyAudioStream(m_musicStream);
	}
	if (m_sfxStream)
	{
		SDL_ClearAudioStream(m_sfxStream);
		SDL_DestroyAudioStream(m_sfxStream);
	}
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

SDL_AudioStream* AudioSystem::getOrCreateStream(AudioChannel channel) noexcept
{
	const auto create = [&](SDL_AudioStream*& stream) -> SDL_AudioStream*
	{
		if (stream)
		{
			return stream;
		}

		SDL_AudioSpec spec = {};
		spec.format = SDL_AUDIO_F32;
		spec.channels = 2;
		spec.freq = 48000;

		stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
		if (!stream)
		{
			Logger::Warn(std::string("AudioSystem: SDL_OpenAudioDeviceStream failed: ") + SDL_GetError());
			return nullptr;
		}
		SDL_ResumeAudioStreamDevice(stream);
		return stream;
	};

	switch (channel)
	{
		case AudioChannel::Music: return create(m_musicStream);
		case AudioChannel::SFX: return create(m_sfxStream);
	}
	return nullptr;
}

AudioClip* AudioSystem::LoadWAV(std::string_view path)
{
	SDL_AudioSpec spec = {};
	uint8_t* buf = nullptr;
	uint32_t len = 0;

	if (!SDL_LoadWAV(path.data(), &spec, &buf, &len))
	{
		Logger::Warn(std::string("AudioSystem: failed to load WAV: ") + path.data());
		return nullptr;
	}

	AudioClip clip;
	clip.sampleRate = spec.freq;
	clip.channels = spec.channels;

	if (spec.format == SDL_AUDIO_F32)
	{
		const float* fbuf = reinterpret_cast<const float*>(buf);
		const size_t sampleCount = len / sizeof(float);
		clip.samples.assign(fbuf, fbuf + sampleCount);
	}
	else if (spec.format == SDL_AUDIO_S16)
	{
		SDL_AudioSpec srcSpec = spec;
		SDL_AudioSpec dstSpec = {};
		dstSpec.format = SDL_AUDIO_F32;
		dstSpec.channels = spec.channels;
		dstSpec.freq = spec.freq;

		uint8_t* converted = nullptr;
		int convertedLen = 0;
		if (SDL_ConvertAudioSamples(&srcSpec, buf, static_cast<int>(len),
			&dstSpec, &converted, &convertedLen))
		{
			const float* fbuf = reinterpret_cast<const float*>(converted);
			const size_t sampleCount = static_cast<size_t>(convertedLen) / sizeof(float);
			clip.samples.assign(fbuf, fbuf + sampleCount);
			SDL_free(converted);
		}
	}
	else
	{
		SDL_free(buf);
		Logger::Warn("AudioSystem: unsupported WAV format");
		return nullptr;
	}

	SDL_free(buf);

	std::string key(path);
	m_clips[key] = std::move(clip);
	return &m_clips[key];
}

AudioClip* AudioSystem::LoadOGG(std::string_view path)
{
	int32_t channels = 0;
	int32_t sampleRate = 0;
	short* output = nullptr;

	const std::string pathStr(path);
	const int32_t frames = stb_vorbis_decode_filename(pathStr.c_str(), &channels, &sampleRate, &output);
	if (frames <= 0 || !output)
	{
		if (output) free(output);
		Logger::Warn(std::string("AudioSystem: failed to load OGG: ") + path.data());
		return nullptr;
	}

	AudioClip clip;
	clip.sampleRate = sampleRate;
	clip.channels = channels;
	clip.samples.resize(static_cast<size_t>(frames) * channels);

	const float inv32768 = 1.0f / 32768.0f;
	std::transform(output, output + clip.samples.size(), clip.samples.begin(),
		[inv32768](short s) { return static_cast<float>(s) * inv32768; });

	free(output);

	std::string key(path);
	m_clips[key] = std::move(clip);
	return &m_clips[key];
}

AudioClip* AudioSystem::Load(std::string_view path)
{
	std::string_view ext;
	const size_t dot = path.rfind('.');
	if (dot != std::string_view::npos)
	{
		ext = path.substr(dot);
	}

	if (ext == ".wav" || ext == ".WAV")
	{
		return LoadWAV(path);
	}
	if (ext == ".ogg" || ext == ".OGG")
	{
		return LoadOGG(path);
	}

	// Try WAV first, then OGG
	AudioClip* clip = LoadWAV(path);
	if (clip) return clip;
	return LoadOGG(path);
}

void AudioSystem::Play(std::string_view name, AudioChannel channel, float volume) noexcept
{
	if (!m_valid)
	{
		return;
	}

	auto it = m_clips.find(std::string(name));
	if (it == m_clips.end())
	{
		return;
	}

	SDL_AudioStream* stream = getOrCreateStream(channel);
	if (!stream)
	{
		return;
	}

	const AudioClip& clip = it->second;
	const float channelVolume = (channel == AudioChannel::Music) ? m_musicVolume : m_sfxVolume;
	const float finalVolume = volume * channelVolume;
	const uint64_t bytes = static_cast<uint64_t>(clip.samples.size()) * sizeof(float);
	uint64_t& streamPos = (channel == AudioChannel::Music) ? m_musicStreamPos : m_sfxStreamPos;

	if (finalVolume >= 1.0f - 1e-6f && finalVolume <= 1.0f + 1e-6f)
	{
		SDL_PutAudioStreamData(stream,
			clip.samples.data(),
			static_cast<int32_t>(bytes));
	}
	else
	{
		m_scratch.resize(clip.samples.size());
		for (size_t i = 0; i < m_scratch.size(); ++i)
		{
			m_scratch[i] = clip.samples[i] * finalVolume;
		}
		SDL_PutAudioStreamData(stream,
			m_scratch.data(),
			static_cast<int32_t>(bytes));
	}

	streamPos += bytes;
	m_playing.push_back({std::string(name), channel, streamPos});
}

void AudioSystem::Update() noexcept
{
	auto cleanupStream = [&](SDL_AudioStream* stream, uint64_t streamPos, AudioChannel channel)
	{
		if (!stream)
		{
			return;
		}

		const uint64_t remaining = static_cast<uint64_t>(SDL_GetAudioStreamQueued(stream));
		const uint64_t consumed = streamPos - remaining;

		for (auto it = m_playing.begin(); it != m_playing.end(); )
		{
			if (it->channel == channel && consumed >= it->endBytePos)
			{
				it = m_playing.erase(it);
			}
			else
			{
				++it;
			}
		}
	};

	cleanupStream(m_musicStream, m_musicStreamPos, AudioChannel::Music);
	cleanupStream(m_sfxStream, m_sfxStreamPos, AudioChannel::SFX);
}

bool AudioSystem::IsPlaying(std::string_view name) const noexcept
{
	return std::ranges::find_if(m_playing,
		[&](const PlayingClip& p) { return p.name == name; }) != m_playing.end();
}

void AudioSystem::Stop(std::string_view name) noexcept
{
	const std::string target(name);

	// Remove target clips from playlist only — stream data remains intact
	// and will be naturally consumed by the audio device. The Update()
	// cleanup loop will ignore the removed clips since they're no longer
	// in m_playing. This avoids destructive stream rebuild which would
	// restart all remaining clips from the beginning.
	std::erase_if(m_playing, [&](const PlayingClip& p) { return p.name == target; });
}

void AudioSystem::StopAll() noexcept
{
	if (m_musicStream)
	{
		SDL_ClearAudioStream(m_musicStream);
	}
	if (m_sfxStream)
	{
		SDL_ClearAudioStream(m_sfxStream);
	}
	m_playing.clear();
}

void AudioSystem::StopChannel(AudioChannel channel) noexcept
{
	SDL_AudioStream* stream = (channel == AudioChannel::Music) ? m_musicStream : m_sfxStream;
	if (stream)
	{
		SDL_ClearAudioStream(stream);
	}

	for (auto it = m_playing.begin(); it != m_playing.end(); )
	{
		if (it->channel == channel)
		{
			it = m_playing.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void AudioSystem::SetChannelVolume(AudioChannel channel, float volume) noexcept
{
	switch (channel)
	{
		case AudioChannel::Music: m_musicVolume = volume; break;
		case AudioChannel::SFX: m_sfxVolume = volume; break;
	}
}

AudioClip* AudioSystem::GetClip(std::string_view name) noexcept
{
	auto it = m_clips.find(std::string(name));
	return it != m_clips.end() ? &it->second : nullptr;
}
