#pragma once

#include <glm/glm.hpp>

#include <memory>

namespace Sunta
{

class Framebuffer;
class Shader;
class VertexArray;

// How the finished picture of the scene is turned into what the screen shows (see EnvironmentComponent)
struct PostProcessSettings
{
	// How bright the picture is before tone mapping: 1 = as lit, more = brighter
	float exposure = 0.8f;

	// The glow around bright things (fire, embers, glowing armor): what is brighter than 'bloomThreshold' spreads
	// around itself, added 'bloomIntensity' strong. More 'bloomBlurPasses' = a wider glow
	bool  isBloomEnabled  = true;
	float bloomThreshold  = 0.7f;
	float bloomIntensity  = 1.2f;
	int   bloomBlurPasses = 6;
};

// HDR and bloom (see learnopengl.com: HDR, Bloom)
//
// Between Begin and End the scene is drawn into a framebuffer of floating point colors, so a light brighter than white
// (a fire up close, the boss's glowing cracks) keeps how bright it really is instead of being cut at 1. End then:
//  1. bright pass: keeps only what is brighter than the threshold, in a picture of a quarter of the size
//  2. blur: blurs that picture with a Gaussian blur, horizontally and vertically in turns (two framebuffers in turns)
//  3. composite: the scene + the blurred glow, multiplied by the exposure and tone mapped (ACES) back into 0..1,
//     drawn over the whole screen, where the HUD then goes on top
class PostProcessing
{
public:
	// Starts drawing the scene into the HDR framebuffer of this size. False when it can't be made (draw as before)
	static bool Begin(int width, int height, float clearR, float clearG, float clearB);

	// Puts the finished picture on the screen (the framebuffer used before Begin)
	static void End(const PostProcessSettings& settings);

	// See-through things (crystal, ice): between Begin and End, copies what is drawn so far, so they can show it
	// through themselves, bent (see Lit.shader, CalculateCrystal). False when there is nothing to copy from
	static bool CopySceneForSeeThrough();
	// Binds that copy to a texture slot; its size in pixels
	static void BindSceneCopy(unsigned int slot);
	// True between a successful copy and the end of the scene
	static bool HasSceneCopy() { return hasSceneCopy; }
	static glm::vec2 GetSceneCopySize();

	// An impact frame: for a blink the picture is blurred out from its middle, its colors split at the edges and its
	// contrast punched up (the way fighting games and action films sell a heavy blow). 'strength' 0..1, fading
	// over 'seconds' of real time; a stronger kick replaces a weaker one still fading
	static void Kick(float strength, float seconds = 0.3f);

	// A blow in the game kicks the picture by its camera shake times this (the scene's Environment sets it; 0: never)
	static void KickFromHit(float cameraShake);
	static inline float hitKickPerShake = 0.0f;

private:
	static inline std::shared_ptr<Framebuffer> sceneTarget;
	static inline std::shared_ptr<Framebuffer> sceneCopy;
	static inline bool isDrawingScene = false;
	static inline bool hasSceneCopy   = false;
	static inline std::shared_ptr<Framebuffer> bloomTargets[2];

	static inline std::shared_ptr<Shader> brightPassShader;
	static inline std::shared_ptr<Shader> blurShader;
	static inline std::shared_ptr<Shader> compositeShader;

	// Two triangles over the whole screen
	static inline std::shared_ptr<VertexArray> screenQuad;

	static void EnsureInitialized();
	static void EnsureTargets(int width, int height);
	static void DrawScreenQuad();

	// Returns which of the two bloom targets holds the blurred glow
	static int MakeBloom(const PostProcessSettings& settings);
};

}
