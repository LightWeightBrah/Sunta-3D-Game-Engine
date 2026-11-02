#pragma once

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

private:
	static inline std::shared_ptr<Framebuffer> sceneTarget;
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
