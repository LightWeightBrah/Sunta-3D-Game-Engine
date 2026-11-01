#pragma once

#include <string>
#include <glm/glm.hpp>

namespace Sunta
{

// The sound of the engine: short sounds played once (a sword's swing), looping sounds (a fire crackling) and music
//
// The playing and mixing is done by miniaudio (Dependencies/miniaudio_wrapper, public domain or MIT-0), it reads
// .wav, .flac, .mp3 and .ogg (through stb_vorbis). A sound is a virtual path (@game/Sounds/Combat/swing.ogg) to a file
// or to a FOLDER: a folder plays one of its files at random each time (five footsteps sound like walking, one doesn't)
// A spatial sound is heard from a point in the world: quieter further from the listener (the camera), and from its side
// Every sound gets a number (a voice) to change or stop it later; a voice that finished is freed by Update()
// Without a sound device (or before Init) everything here does nothing, the game plays on in silence
class Audio
{
public:
	using VoiceID = unsigned int;
	static constexpr VoiceID NoVoice = 0;

	struct PlayOptions
	{
		float volume         = 1.0f;
		float pitch          = 1.0f;
		float pitchVariation = 0.0f;  // the pitch of each play is random in pitch +- this (0.05: no two swings are the same)
		bool  isLooping      = false;

		bool      isSpatial   = false;
		glm::vec3 position    = glm::vec3(0.0f);
		float     minDistance = 2.0f;   // full volume closer than this (meters)
		float     maxDistance = 40.0f;  // it gets no quieter further than this

		float fadeInSeconds = 0.0f;
		bool  isStreamed    = false;    // long files (music): read while playing instead of decoded whole in advance
	};

	static void Init();
	static void Shutdown();

	// False without a sound device: nothing is heard
	static bool IsAvailable();

	// NoVoice when the sound can't be played (the reason is logged once per file)
	static VoiceID Play(const std::string& sound, const PlayOptions& options = {});

	static void Stop(VoiceID voice, float fadeOutSeconds = 0.0f);
	static void StopAll();
	static bool IsPlaying(VoiceID voice);

	static void SetVolume(VoiceID voice, float volume);
	static void SetPosition(VoiceID voice, const glm::vec3& position);

	// One music track at a time: a new one fades in while the old one fades out. Empty sound = no new track
	static void PlayMusic(const std::string& sound, float volume = 1.0f, float fadeSeconds = 2.0f);
	static void StopMusic(float fadeSeconds = 2.0f);

	// Where the ears are: the camera of the game
	static void SetListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);

	// Decodes the sound (every file of a folder) now, so its first play doesn't wait for the file in the middle of a fight
	static void Preload(const std::string& sound);

	// Once per frame: frees the voices that finished
	static void Update();
};

}
