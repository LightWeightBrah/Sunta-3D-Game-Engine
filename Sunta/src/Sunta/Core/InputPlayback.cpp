#include "SuntaPreCompiled.h"
#include "InputPlayback.h"

#include <glad/glad.h> // before GLFW (InputManager.h), glad must be the first OpenGL header

#include "InputManager.h"
#include "KeyCodes.h"
#include "Log.h"
#include "Events/EventBus.h"
#include "Events/EventTypes.h"

#include <stb/stb_image_write.h>

#include <algorithm>

namespace Sunta
{

namespace
{

constexpr int MouseCodeOffset = 10000; // key codes from this up are mouse buttons

struct TimedCommand
{
	float		time = 0.0f;
	std::string command;
	std::string argument;
	std::string argument2;
	std::string argument3;
};

struct PendingRelease
{
	float time = 0.0f;
	int	  code = 0;
};

struct PendingLook
{
	float	  from = 0.0f;
	float	  to   = 0.0f;
	glm::vec2 total{ 0.0f };
	glm::vec2 sent { 0.0f };
};

bool						isActive  = false;
bool						wantsQuit = false;
std::string					outputFolder;
std::vector<TimedCommand>	commands;
size_t						nextCommand = 0;
std::vector<PendingRelease> releases;
std::vector<PendingLook>	looks;
std::vector<std::string>	shotsToSave;
float						lastTime = 0.0f;
InputPlayback::CommandHandler customHandler;
std::vector<int>			heldKeys;
std::unordered_set<std::string> sameFrameShots; // names already shot this frame

// The pointer of the script ("cursor X Y [SECONDS]": window pixels from the top left), for the editor's GUI to read
struct PendingCursor
{
	float	  from = 0.0f;
	float	  to   = 0.0f;
	glm::vec2 start{ 0.0f };
	glm::vec2 end  { 0.0f };
};
bool						  hasCursor = false;
glm::vec2					  cursor{ 0.0f };
std::vector<PendingCursor>	  cursorMoves;

int ParseKey(std::string name)
{
	std::string lower = name;
	std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return (char)std::tolower(c); });

	if (lower == "mouse1" || lower == "lmb") return MouseCodeOffset + SUNTA_MOUSE_BUTTON_LEFT;
	if (lower == "mouse2" || lower == "rmb") return MouseCodeOffset + SUNTA_MOUSE_BUTTON_RIGHT;
	if (lower == "mouse3" || lower == "mmb") return MouseCodeOffset + SUNTA_MOUSE_BUTTON_MIDDLE;
	if (lower == "space")				 return SUNTA_KEY_SPACE;
	if (lower == "shift")				 return SUNTA_KEY_LEFT_SHIFT;
	if (lower == "ctrl")				 return SUNTA_KEY_LEFT_CONTROL;
	if (lower == "enter")				 return SUNTA_KEY_ENTER;
	if (lower == "esc" || lower == "escape") return SUNTA_KEY_ESCAPE;
	if (lower == "tab")					 return SUNTA_KEY_TAB;
	if (lower == "up")					 return SUNTA_KEY_UP;
	if (lower == "down")				 return SUNTA_KEY_DOWN;
	if (lower == "left")				 return SUNTA_KEY_LEFT;
	if (lower == "right")				 return SUNTA_KEY_RIGHT;

	if (lower.size() >= 2 && lower[0] == 'f' && std::isdigit((unsigned char)lower[1]))
		return SUNTA_KEY_F1 + std::stoi(lower.substr(1)) - 1;

	if (name.size() == 1)
	{
		const char c = (char)std::toupper((unsigned char)name[0]);
		if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
			return c; // letters and digits share their ASCII codes with GLFW
	}

	SUNTA_ENGINE_LOG_WARNING("InputPlayback: unknown key '{0}'", name);
	return -1;
}

// How long each frame took since the last "fps" command (seconds), to report how smoothly a moment of the game runs
std::vector<float> frameSeconds;
float lastFrameAt = -1.0f;

// Every change of a mouse button the script made, in order, for the editor's GUI (see TakeMouseButtonChanges)
std::vector<std::pair<int, bool>> mouseButtonChanges;

void Press(int code)
{
	if (code < 0) return;
	if (std::find(heldKeys.begin(), heldKeys.end(), code) == heldKeys.end())
		heldKeys.push_back(code);
	if (code >= MouseCodeOffset)
	{
		EventBus::Publish(MouseButtonPressedEvent{ code - MouseCodeOffset });
		mouseButtonChanges.push_back({ code - MouseCodeOffset, true });
	}
	else
		EventBus::Publish(KeyPressedEvent{ code });
}

void Release(int code)
{
	if (code < 0) return;
	heldKeys.erase(std::remove(heldKeys.begin(), heldKeys.end(), code), heldKeys.end());
	if (code >= MouseCodeOffset)
	{
		EventBus::Publish(MouseButtonReleasedEvent{ code - MouseCodeOffset });
		mouseButtonChanges.push_back({ code - MouseCodeOffset, false });
	}
	else
		EventBus::Publish(KeyReleasedEvent{ code });
}

float ToFloat(const std::string& text, float fallback)
{
	try { return text.empty() ? fallback : std::stof(text); }
	catch (...) { return fallback; }
}

bool IsRepeated(const std::string& shotName)
{
	int count = 0;
	for (const TimedCommand& c : commands)
		if (c.command == "shot" && c.argument == shotName && ++count > 1)
			return true;
	return false;
}

void Run(const TimedCommand& c, float now)
{
	if (c.command == "down")
		Press(ParseKey(c.argument));
	else if (c.command == "up")
		Release(ParseKey(c.argument));
	else if (c.command == "tap" || c.command == "hold")
	{
		const int code = ParseKey(c.argument);
		Press(code);
		// A tap is released on the next frame (a tiny delay), so the game sees "pressed this frame"
		releases.push_back({ now + (c.command == "hold" ? ToFloat(c.argument2, 0.5f) : 0.001f), code });
	}
	else if (c.command == "look")
	{
		const float seconds = ToFloat(c.argument3, 0.0f);
		looks.push_back({ now, now + seconds, { ToFloat(c.argument, 0.0f), ToFloat(c.argument2, 0.0f) } });
	}
	else if (c.command == "shot")
	{
		// A name used again (a shot repeated with "every") gets a number: s_000, s_001, ...
		static std::unordered_map<std::string, int> uses;
		const std::string name = c.argument.empty() ? "shot" : c.argument;
		// Two shots of one name due in the same frame would be the same picture: only the first is saved
		if (sameFrameShots.count(name))
			return;
		sameFrameShots.insert(name);
		const int use = uses[name]++;
		char suffix[16];
		std::snprintf(suffix, sizeof(suffix), "_%03d", use);
		shotsToSave.push_back(use == 0 && !IsRepeated(name) ? name : name + suffix);
	}
	else if (c.command == "cursor")
	{
		const glm::vec2 target{ ToFloat(c.argument, 0.0f), ToFloat(c.argument2, 0.0f) };
		const float seconds = ToFloat(c.argument3, 0.0f);
		if (!hasCursor || seconds <= 0.0f)
			cursor = target;
		else
			cursorMoves.push_back({ now, now + seconds, cursor, target });
		hasCursor = true;
	}
	else if (c.command == "log")
		SUNTA_ENGINE_LOG_INFO("InputPlayback [{0}s]: {1} {2} {3}", now, c.argument, c.argument2, c.argument3);
	else if (c.command == "fps")
	{
		// "fps [label]": the frames since the last report - their average and the slowest one (a hitch is felt)
		if (!frameSeconds.empty())
		{
			float sum = 0.0f, slowest = 0.0f;
			for (float seconds : frameSeconds)
			{
				sum += seconds;
				slowest = std::max(slowest, seconds);
			}
			const float average = sum / static_cast<float>(frameSeconds.size());
			// (the log has no number formats: the text is made here)
			char report[160];
			std::snprintf(report, sizeof(report), "fps %s average %.1f (%.1f ms), slowest frame %.1f ms, %d frames",
				c.argument.c_str(), average > 0.0f ? 1.0f / average : 0.0f, average * 1000.0f, slowest * 1000.0f,
				static_cast<int>(frameSeconds.size()));
			SUNTA_ENGINE_LOG_INFO("InputPlayback [{0}s]: {1}", static_cast<int>(now), report);
		}
		frameSeconds.clear();
	}
	else if (c.command == "quit")
		wantsQuit = true;
	else if (!customHandler || !customHandler(c.command, c.argument, c.argument2, c.argument3, now))
		SUNTA_ENGINE_LOG_WARNING("InputPlayback: unknown command '{0}'", c.command);
}

void Load(const std::string& path)
{
	std::ifstream file(path);
	if (!file)
	{
		SUNTA_ENGINE_LOG_ERROR("InputPlayback: can't open '{0}'", path);
		return;
	}

	std::string line;
	while (std::getline(file, line))
	{
		const size_t comment = line.find('#');
		if (comment != std::string::npos)
			line.erase(comment);

		std::istringstream words(line);
		TimedCommand c;
		if (!(words >> c.time >> c.command))
			continue;

		if (c.command == "every")
		{
			float step = 0.0f, until = 0.0f;
			TimedCommand repeated;
			words >> step >> until >> repeated.command >> repeated.argument >> repeated.argument2 >> repeated.argument3;
			for (float t = c.time; step > 0.0f && t <= until; t += step)
			{
				repeated.time = t;
				commands.push_back(repeated);
			}
			continue;
		}

		words >> c.argument;
		if (c.command == "log")
		{
			std::string rest;
			std::getline(words, rest);
			c.argument += rest;
		}
		else
			words >> c.argument2 >> c.argument3;

		commands.push_back(c);
	}

	std::stable_sort(commands.begin(), commands.end(), [](const TimedCommand& a, const TimedCommand& b) { return a.time < b.time; });
	SUNTA_ENGINE_LOG_INFO("InputPlayback: {0} commands from '{1}'", commands.size(), path);
}

}

void InputPlayback::Init()
{
	const char* script = std::getenv("SUNTA_INPUT_SCRIPT");
	const char* output = std::getenv("SUNTA_TEST_OUT");
	outputFolder = (output && *output) ? output : ".";

	if (!script || !*script)
		return;

	std::error_code ignored;
	std::filesystem::create_directories(outputFolder, ignored);

	Load(script);
	isActive = true;
}

bool InputPlayback::IsActive()
{
	return isActive;
}

void InputPlayback::Update(float now)
{
	if (!isActive)
		return;

	if (lastFrameAt >= 0.0f && now > lastFrameAt)
		frameSeconds.push_back(now - lastFrameAt);
	lastFrameAt = now;

	for (auto it = releases.begin(); it != releases.end();)
	{
		if (it->time <= now && lastTime < now)
		{
			Release(it->code);
			it = releases.erase(it);
		}
		else
			++it;
	}

	sameFrameShots.clear();
	while (nextCommand < commands.size() && commands[nextCommand].time <= now)
		Run(commands[nextCommand++], now);

	for (auto it = looks.begin(); it != looks.end();)
	{
		const float	   span		= it->to - it->from;
		const float	   progress = span <= 0.0f ? 1.0f : std::clamp((now - it->from) / span, 0.0f, 1.0f);
		const glm::vec2 target	= it->total * progress;
		InputManager::AddMouseDelta(target - it->sent);
		it->sent = target;

		if (progress >= 1.0f)
			it = looks.erase(it);
		else
			++it;
	}

	for (auto it = cursorMoves.begin(); it != cursorMoves.end();)
	{
		const float span = it->to - it->from;
		float k = span <= 0.0f ? 1.0f : std::clamp((now - it->from) / span, 0.0f, 1.0f);
		k = k * k * (3.0f - 2.0f * k);
		cursor = it->start + (it->end - it->start) * k;
		if (k >= 1.0f)
			it = cursorMoves.erase(it);
		else
			++it;
	}

	lastTime = now;
}

bool InputPlayback::TryGetCursor(glm::vec2& outPosition)
{
	if (!isActive || !hasCursor)
		return false;
	outPosition = cursor;
	return true;
}

std::vector<std::pair<int, bool>> InputPlayback::TakeMouseButtonChanges()
{
	std::vector<std::pair<int, bool>> changes;
	changes.swap(mouseButtonChanges);
	return changes;
}

void InputPlayback::RequestShot(const std::string& name)
{
	shotsToSave.push_back(name);
}

void InputPlayback::AfterRender(unsigned int width, unsigned int height)
{
	if (shotsToSave.empty())
		return;

	// The real size of the drawn picture (the window size can differ on high-DPI screens)
	int framebufferWidth = 0, framebufferHeight = 0;
	if (GLFWwindow* context = glfwGetCurrentContext())
		glfwGetFramebufferSize(context, &framebufferWidth, &framebufferHeight);
	if (framebufferWidth > 0 && framebufferHeight > 0)
	{
		width  = (unsigned int)framebufferWidth;
		height = (unsigned int)framebufferHeight;
	}
	if (width == 0 || height == 0)
		return;

	std::vector<unsigned char> pixels((size_t)width * height * 3);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, (GLsizei)width, (GLsizei)height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

	stbi_flip_vertically_on_write(1);
	for (const std::string& name : shotsToSave)
	{
		const std::string path = (std::filesystem::path(outputFolder) / (name + ".png")).string();
		if (!stbi_write_png(path.c_str(), (int)width, (int)height, 3, pixels.data(), (int)width * 3))
			SUNTA_ENGINE_LOG_ERROR("InputPlayback: can't save '{0}'", path);
	}
	shotsToSave.clear();
}

bool InputPlayback::WantsQuit()
{
	return wantsQuit;
}

void InputPlayback::SetCommandHandler(CommandHandler handler)
{
	customHandler = std::move(handler);
}

void InputPlayback::PressKey(const std::string& name)
{
	Press(ParseKey(name));
}

void InputPlayback::ReleaseKey(const std::string& name)
{
	Release(ParseKey(name));
}

bool InputPlayback::IsKeyHeldByScript(const std::string& name)
{
	const int code = ParseKey(name);
	return std::find(heldKeys.begin(), heldKeys.end(), code) != heldKeys.end();
}

const std::string& InputPlayback::GetOutputFolder()
{
	return outputFolder;
}

}
