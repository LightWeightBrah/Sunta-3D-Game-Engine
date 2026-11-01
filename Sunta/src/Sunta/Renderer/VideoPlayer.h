#pragma once

#include <memory>
#include <string>
#include <vector>

typedef struct plm_t plm_t;

namespace Sunta
{

class Texture;

// Plays a video file frame by frame into a texture (the picture only, no sound yet)
//
// The decoding is done by pl_mpeg (Dependencies/pl_mpeg_wrapper, MIT license): it reads MPEG-1 video (.mpg).
// Any video can be turned into one with ffmpeg, e.g.
//     ffmpeg -i intro.mp4 -c:v mpeg1video -q:v 4 -an intro.mpg
// Each Update decodes the frames that fall into the elapsed time, the newest one is copied into the texture
class VideoPlayer
{
public:
	VideoPlayer() = default;
	~VideoPlayer();

	VideoPlayer(const VideoPlayer&) = delete;
	VideoPlayer& operator=(const VideoPlayer&) = delete;

	// False when the file can't be read as an MPEG-1 video
	bool Open(const std::string& path);
	void Close();

	void Update(float deltaTime);

	bool IsOpen()     const { return player != nullptr; }
	bool IsFinished() const;

	// The current frame (nullptr before the first one)
	const Texture* GetTexture() const { return hasFrame ? texture.get() : nullptr; }
	int GetWidth()  const { return width; }
	int GetHeight() const { return height; }

	// Called by the decoder with each new frame (a plm_frame_t of pl_mpeg)
	void TakeFrame(void* decodedFrame);

private:
	plm_t*                     player = nullptr;
	std::shared_ptr<Texture>   texture;
	std::vector<unsigned char> pixels;
	int  width    = 0;
	int  height   = 0;
	bool hasFrame = false;
};

}
