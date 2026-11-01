#include "Core/SuntaPreCompiled.h"
#include "VideoPlayer.h"

#include <pl_mpeg/pl_mpeg.h>

#include "Renderer.h"
#include "RendererDevice.h"
#include "Texture.h"
#include "Core/Log.h"

namespace Sunta
{

namespace
{

constexpr int BytesPerPixel = 4;

void OnVideoFrame(plm_t*, plm_frame_t* frame, void* user)
{
	static_cast<VideoPlayer*>(user)->TakeFrame(frame);
}

// Frames longer than this (a long loading frame) are not caught up at once: the video slows down instead of skipping
constexpr double MaxStepSeconds = 0.1;

}

VideoPlayer::~VideoPlayer()
{
	Close();
}

bool VideoPlayer::Open(const std::string& path)
{
	Close();

	player = plm_create_with_filename(path.c_str());
	if (!player)
	{
		SUNTA_ENGINE_LOG_ERROR("VideoPlayer: couldn't open '{0}' (it must be an MPEG-1 video, see VideoPlayer.h)", path);
		return false;
	}

	width  = plm_get_width(player);
	height = plm_get_height(player);
	if (width <= 0 || height <= 0)
	{
		SUNTA_ENGINE_LOG_ERROR("VideoPlayer: '{0}' has no video picture", path);
		Close();
		return false;
	}

	plm_set_audio_enabled(player, 0);
	plm_set_loop(player, 0);
	plm_set_video_decode_callback(player, &OnVideoFrame, this);

	pixels.assign((size_t)width * height * BytesPerPixel, 0);
	texture  = Renderer::GetDevice().CreateTexture(width, height, pixels.data());
	hasFrame = false;
	return true;
}

void VideoPlayer::Close()
{
	if (player)
		plm_destroy(player);

	player   = nullptr;
	hasFrame = false;
}

void VideoPlayer::Update(float deltaTime)
{
	if (player && !plm_has_ended(player))
		plm_decode(player, glm::min((double)deltaTime, MaxStepSeconds));
}

bool VideoPlayer::IsFinished() const
{
	return !player || plm_has_ended(player);
}

void VideoPlayer::TakeFrame(void* decodedFrame)
{
	plm_frame_to_rgba(static_cast<plm_frame_t*>(decodedFrame), pixels.data(), width * BytesPerPixel);

	// The decoder leaves the alpha channel alone: the picture is opaque
	for (size_t alpha = 3; alpha < pixels.size(); alpha += BytesPerPixel)
		pixels[alpha] = 255;

	texture->SetPixels(pixels.data());
	hasFrame = true;
}

}
