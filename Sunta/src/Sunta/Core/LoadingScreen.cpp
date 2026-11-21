#include "SuntaPreCompiled.h"
#include "LoadingScreen.h"

#include <chrono>

#include "BackgroundRun.h"
#include "InputPlayback.h"
#include "Window.h"
#include "Renderer/Renderer.h"
#include "Renderer/RendererDevice.h"
#include "Renderer/UIRenderer.h"

namespace Sunta
{

namespace
{

constexpr double DrawEverySeconds = 1.0 / 30.0;

const glm::vec4 BackgroundColor = glm::vec4(0.05f, 0.04f, 0.035f, 1.0f);
const glm::vec4 TitleColor      = glm::vec4(0.93f, 0.86f, 0.70f, 1.0f);
const glm::vec4 LabelColor      = glm::vec4(0.65f, 0.61f, 0.54f, 1.0f);
const glm::vec4 BarBackColor    = glm::vec4(0.14f, 0.12f, 0.10f, 1.0f);
const glm::vec4 BarEdgeColor    = glm::vec4(0.48f, 0.38f, 0.22f, 1.0f);
const glm::vec4 BarFillColor    = glm::vec4(0.86f, 0.64f, 0.30f, 1.0f);

double SecondsNow()
{
	using namespace std::chrono;
	return duration<double>(steady_clock::now().time_since_epoch()).count();
}

}

void LoadingScreen::Begin(Window& newWindow, const std::string& newTitle)
{
	window      = &newWindow;
	title       = newTitle;
	label.clear();
	progress    = 0.0f;
	lastDrawnAt = -1.0;

	if (!IsBackgroundRun())
		window->Show();
	Draw();
}

void LoadingScreen::Step(float newProgress, const std::string& newLabel)
{
	if (!window)
		return;

	progress = glm::clamp(glm::max(progress, newProgress), 0.0f, 1.0f);
	label    = newLabel;

	if (SecondsNow() - lastDrawnAt >= DrawEverySeconds)
		Draw();
}

void LoadingScreen::End()
{
	if (!window)
		return;

	window = nullptr;
}

void LoadingScreen::Cover()
{
	if (window)
		Draw(false);
}

void LoadingScreen::Draw(bool present)
{
	lastDrawnAt = SecondsNow();

	const glm::vec2 size = glm::vec2((float)window->GetWidth(), (float)window->GetHeight());
	if (size.x <= 0.0f || size.y <= 0.0f)
		return;

	RendererDevice& device = Renderer::GetDevice();
	device.SetViewport(0, 0, (int)size.x, (int)size.y);
	device.Clear(BackgroundColor.r, BackgroundColor.g, BackgroundColor.b, 1.0f);

	const float unit = size.y;
	const glm::vec2 center = size * 0.5f;

	UIRenderer::Begin(size);
	UIRenderer::DrawTextCentered(title, glm::vec2(center.x, center.y - unit * 0.07f), unit * 0.06f, TitleColor);

	const glm::vec2 barSize = glm::vec2(glm::min(size.x * 0.5f, unit * 0.9f), glm::max(unit * 0.012f, 6.0f));
	const glm::vec2 barTopLeft = glm::vec2(center.x - barSize.x * 0.5f, center.y + unit * 0.02f);
	UIRenderer::DrawRectangle(barTopLeft - glm::vec2(2.0f), barSize + glm::vec2(4.0f), BarEdgeColor);
	UIRenderer::DrawRectangle(barTopLeft, barSize, BarBackColor);
	UIRenderer::DrawRectangle(barTopLeft, glm::vec2(barSize.x * progress, barSize.y), BarFillColor);

	if (!label.empty())
		UIRenderer::DrawTextCentered(label, glm::vec2(center.x, barTopLeft.y + barSize.y + unit * 0.045f), unit * 0.025f, LabelColor);
	UIRenderer::End();

	// A test run with an input script records the loading screen once, half way through
	static bool hasRecorded = false;
	if (present && !hasRecorded && progress >= 0.5f && InputPlayback::IsActive())
	{
		hasRecorded = true;
		InputPlayback::RequestShot("loading_screen");
		InputPlayback::AfterRender((unsigned int)size.x, (unsigned int)size.y);
	}

	// Shows the picture (and keeps the window answering the system while it loads)
	if (present)
		window->Update();
}

}
