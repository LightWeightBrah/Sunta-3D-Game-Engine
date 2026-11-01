#include "Core/SuntaPreCompiled.h"
#include "Audio.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <random>
#include <set>
#include <unordered_map>
#include <vector>

#include <miniaudio/miniaudio.h>

#include "Core/Log.h"
#include "Core/VirtualFileSystem.h"

namespace Sunta
{

namespace
{

constexpr int ListenerIndex = 0;

// The sound files miniaudio can read (stb_vorbis adds .ogg)
const std::vector<std::string> SoundFileExtensions = { ".wav", ".ogg", ".flac", ".mp3" };

ma_engine engine;
bool      isAvailable = false;

std::unordered_map<Audio::VoiceID, std::unique_ptr<ma_sound>> voices;
Audio::VoiceID nextVoiceID = 1;
Audio::VoiceID musicVoice  = Audio::NoVoice;

// The files of each folder used as a sound, found once
std::unordered_map<std::string, std::vector<std::string>> folderFiles;

// Each missing or unreadable file is reported once, not on every swing
std::set<std::string> reportedProblems;

std::mt19937 randomEngine(std::random_device{}());

ma_uint64 ToMilliseconds(float seconds)
{
	return (ma_uint64)(std::max(seconds, 0.0f) * 1000.0f);
}

void ReportOnce(const std::string& path, const std::string& problem)
{
	if (reportedProblems.insert(path).second)
		SUNTA_ENGINE_LOG_ERROR("Audio: {0} '{1}'", problem, path);
}

// A missing file is a wrong path in the scene, an existing one is in a format miniaudio can't read
void ReportUnreadable(const std::string& file)
{
	std::error_code errorCode;
	if (!std::filesystem::exists(file, errorCode))
		ReportOnce(file, "the sound file doesn't exist");
	else
		ReportOnce(file, "couldn't read the sound file (it must be .wav, .ogg, .flac or .mp3)");
}

bool IsSoundFile(const std::filesystem::path& file)
{
	std::string extension = file.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)std::tolower(c); });

	return std::find(SoundFileExtensions.begin(), SoundFileExtensions.end(), extension) != SoundFileExtensions.end();
}

// The sound files of the folder, sorted (the same order on every system)
const std::vector<std::string>& GetFolderFiles(const std::string& folder)
{
	auto found = folderFiles.find(folder);
	if (found != folderFiles.end())
		return found->second;

	std::vector<std::string> files;
	std::error_code errorCode;
	for (const auto& entry : std::filesystem::directory_iterator(folder, errorCode))
	{
		if (entry.is_regular_file() && IsSoundFile(entry.path()))
			files.push_back(entry.path().string());
	}

	std::sort(files.begin(), files.end());
	return folderFiles.emplace(folder, std::move(files)).first->second;
}

// Every real file the sound stands for: the file itself, or each sound file of the folder
std::vector<std::string> GetFiles(const std::string& sound)
{
	const std::string path = VirtualFileSystem::Resolve(sound);

	std::error_code errorCode;
	if (std::filesystem::is_directory(path, errorCode))
		return GetFolderFiles(path);

	return { path };
}

// The file to play this time: a folder gives one of its files at random. Empty when there is nothing to play
std::string ChooseFile(const std::string& sound)
{
	const std::vector<std::string> files = GetFiles(sound);
	if (files.empty())
	{
		ReportOnce(sound, "there are no sound files (.wav .ogg .flac .mp3) in the folder");
		return {};
	}

	if (files.size() == 1)
		return files.front();

	std::uniform_int_distribution<size_t> pick(0, files.size() - 1);
	return files[pick(randomEngine)];
}

float ChoosePitch(const Audio::PlayOptions& options)
{
	if (options.pitchVariation <= 0.0f)
		return options.pitch;

	std::uniform_real_distribution<float> variation(-options.pitchVariation, options.pitchVariation);
	return std::max(options.pitch + variation(randomEngine), 0.01f);
}

ma_uint32 GetSoundFlags(const Audio::PlayOptions& options)
{
	ma_uint32 flags = options.isStreamed ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE;
	if (!options.isSpatial)
		flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

	return flags;
}

void ApplyOptions(ma_sound& sound, const Audio::PlayOptions& options)
{
	ma_sound_set_volume(&sound, options.volume);
	ma_sound_set_pitch(&sound, ChoosePitch(options));
	ma_sound_set_looping(&sound, options.isLooping ? MA_TRUE : MA_FALSE);

	if (options.isSpatial)
	{
		ma_sound_set_position(&sound, options.position.x, options.position.y, options.position.z);
		ma_sound_set_min_distance(&sound, options.minDistance);
		ma_sound_set_max_distance(&sound, std::max(options.maxDistance, options.minDistance));
	}

	if (options.fadeInSeconds > 0.0f)
		ma_sound_set_fade_in_milliseconds(&sound, 0.0f, 1.0f, ToMilliseconds(options.fadeInSeconds));
}

ma_sound* FindVoice(Audio::VoiceID voice)
{
	auto found = voices.find(voice);
	return found != voices.end() ? found->second.get() : nullptr;
}

void FreeVoice(std::unique_ptr<ma_sound>& sound)
{
	ma_sound_uninit(sound.get());
	sound.reset();
}

}

void Audio::Init()
{
	if (isAvailable)
		return;

	const ma_engine_config config = ma_engine_config_init();
	if (ma_engine_init(&config, &engine) != MA_SUCCESS)
	{
		SUNTA_ENGINE_LOG_WARNING("Audio: there is no sound device, the game plays without sound");
		return;
	}

	ma_engine_listener_set_world_up(&engine, ListenerIndex, 0.0f, 1.0f, 0.0f);
	isAvailable = true;

	SUNTA_ENGINE_LOG_INFO("Audio: ready ({0} Hz, {1} channels)", ma_engine_get_sample_rate(&engine), ma_engine_get_channels(&engine));
}

void Audio::Shutdown()
{
	if (!isAvailable)
		return;

	for (auto& [voice, sound] : voices)
		FreeVoice(sound);
	voices.clear();
	musicVoice = NoVoice;

	ma_engine_uninit(&engine);
	isAvailable = false;
}

bool Audio::IsAvailable()
{
	return isAvailable;
}

Audio::VoiceID Audio::Play(const std::string& sound, const PlayOptions& options)
{
	if (!isAvailable || sound.empty())
		return NoVoice;

	const std::string file = ChooseFile(sound);
	if (file.empty())
		return NoVoice;

	auto voice = std::make_unique<ma_sound>();
	if (ma_sound_init_from_file(&engine, file.c_str(), GetSoundFlags(options), nullptr, nullptr, voice.get()) != MA_SUCCESS)
	{
		ReportUnreadable(file);
		return NoVoice;
	}

	ApplyOptions(*voice, options);
	ma_sound_start(voice.get());

	const VoiceID voiceID = nextVoiceID++;
	voices.emplace(voiceID, std::move(voice));
	return voiceID;
}

void Audio::Stop(VoiceID voice, float fadeOutSeconds)
{
	ma_sound* sound = FindVoice(voice);
	if (!sound)
		return;

	// The voice is freed by Update once it is silent
	if (fadeOutSeconds > 0.0f)
		ma_sound_stop_with_fade_in_milliseconds(sound, ToMilliseconds(fadeOutSeconds));
	else
		ma_sound_stop(sound);
}

void Audio::StopAll()
{
	for (auto& [voice, sound] : voices)
		FreeVoice(sound);

	voices.clear();
	musicVoice = NoVoice;
}

bool Audio::IsPlaying(VoiceID voice)
{
	ma_sound* sound = FindVoice(voice);
	return sound && ma_sound_is_playing(sound);
}

void Audio::SetVolume(VoiceID voice, float volume)
{
	if (ma_sound* sound = FindVoice(voice))
		ma_sound_set_volume(sound, volume);
}

void Audio::SetPosition(VoiceID voice, const glm::vec3& position)
{
	if (ma_sound* sound = FindVoice(voice))
		ma_sound_set_position(sound, position.x, position.y, position.z);
}

void Audio::PlayMusic(const std::string& sound, float volume, float fadeSeconds)
{
	StopMusic(fadeSeconds);

	PlayOptions options;
	options.volume        = volume;
	options.isLooping     = true;
	options.isStreamed    = true;
	options.fadeInSeconds = fadeSeconds;

	musicVoice = Play(sound, options);
}

void Audio::StopMusic(float fadeSeconds)
{
	Stop(musicVoice, fadeSeconds);
	musicVoice = NoVoice;
}

void Audio::SetListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
{
	if (!isAvailable)
		return;

	ma_engine_listener_set_position(&engine, ListenerIndex, position.x, position.y, position.z);
	ma_engine_listener_set_direction(&engine, ListenerIndex, forward.x, forward.y, forward.z);
	ma_engine_listener_set_world_up(&engine, ListenerIndex, up.x, up.y, up.z);
}

void Audio::Preload(const std::string& sound)
{
	if (!isAvailable || sound.empty())
		return;

	ma_resource_manager* resourceManager = ma_engine_get_resource_manager(&engine);

	for (const std::string& file : GetFiles(sound))
	{
		if (ma_resource_manager_register_file(resourceManager, file.c_str(), MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_DECODE) != MA_SUCCESS)
			ReportUnreadable(file);
	}
}

void Audio::Update()
{
	for (auto voice = voices.begin(); voice != voices.end(); )
	{
		if (ma_sound_is_playing(voice->second.get()))
		{
			++voice;
			continue;
		}

		FreeVoice(voice->second);
		voice = voices.erase(voice);
	}
}

}
