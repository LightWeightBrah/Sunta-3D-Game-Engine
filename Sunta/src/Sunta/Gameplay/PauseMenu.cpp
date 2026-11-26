#include "Core/SuntaPreCompiled.h"
#include "HUDSystem.h"
#include "PauseMenu.h"

#include "Audio/Audio.h"
#include "Core/InputManager.h"
#include "Core/KeyCodes.h"
#include "Renderer/UIRenderer.h"

namespace Sunta
{

namespace
{

constexpr const char* Labels[2] = { "Resume", "Quit" };
constexpr int ButtonCount = 2;

const glm::vec4 ShadeColor        = glm::vec4(0.0f, 0.0f, 0.0f, 0.62f);
const glm::vec4 TitleColor        = glm::vec4(0.93f, 0.86f, 0.70f, 1.0f);
const glm::vec4 LineColor         = glm::vec4(0.75f, 0.62f, 0.38f, 0.8f);
const glm::vec4 ButtonColor       = glm::vec4(0.06f, 0.05f, 0.04f, 0.85f);
const glm::vec4 SelectedColor     = glm::vec4(0.32f, 0.24f, 0.12f, 0.92f);
const glm::vec4 TextColor         = glm::vec4(0.80f, 0.76f, 0.68f, 1.0f);
const glm::vec4 SelectedTextColor = glm::vec4(1.0f, 0.93f, 0.78f, 1.0f);
const glm::vec4 HintColor         = glm::vec4(0.65f, 0.62f, 0.56f, 0.9f);

// Parts of the height of the game's area (the menu scales with the window)
constexpr float TitleHeight  = 0.075f;
constexpr float ButtonHeight = 0.065f;
constexpr float ButtonWidth  = 0.26f;
constexpr float ButtonGap    = 0.022f;
constexpr float TextHeight   = 0.036f;
constexpr float HintHeight   = 0.022f;

bool IsInside(const glm::vec2& point, const glm::vec2& topLeft, const glm::vec2& size)
{
	return point.x >= topLeft.x && point.y >= topLeft.y && point.x <= topLeft.x + size.x && point.y <= topLeft.y + size.y;
}

}

void PauseMenu::SetPaused(bool paused)
{
	if (paused == isPaused)
		return;

	isPaused = paused;
	selected = 0;
	Audio::SetPaused(paused);
}

void PauseMenu::Update(bool isEscapeTaken)
{
	if (InputManager::IsKeyDown(SUNTA_KEY_ESCAPE) && (isPaused || !isEscapeTaken))
	{
		SetPaused(!isPaused);
		return;
	}

	if (!isPaused)
		return;

	if (InputManager::IsKeyDown(SUNTA_KEY_W) || InputManager::IsKeyDown(SUNTA_KEY_UP))
		selected = (selected + ButtonCount - 1) % ButtonCount;
	if (InputManager::IsKeyDown(SUNTA_KEY_S) || InputManager::IsKeyDown(SUNTA_KEY_DOWN))
		selected = (selected + 1) % ButtonCount;

	// The mouse picks the button it is over (where the buttons were drawn last frame)
	const glm::vec2 mouse = InputManager::GetMousePosition();
	int underMouse = -1;
	for (int i = 0; i < ButtonCount; ++i)
	{
		if (buttonSize.x > 0.0f && IsInside(mouse, buttonTopLeft[i], buttonSize))
			underMouse = i;
	}
	if (underMouse >= 0 && glm::length(InputManager::GetMouseDelta()) > 0.0f)
		selected = underMouse;

	const bool clicked   = underMouse >= 0 && InputManager::IsMouseButtonDown(SUNTA_MOUSE_BUTTON_LEFT);
	const bool confirmed = InputManager::IsKeyDown(SUNTA_KEY_ENTER) || InputManager::IsKeyDown(SUNTA_KEY_SPACE);
	if (!clicked && !confirmed)
		return;

	const int chosen = clicked ? underMouse : selected;
	if (chosen == 0)
		SetPaused(false);
	else
		isQuitRequested = true;
}

void PauseMenu::Draw(const glm::vec2& areaTopLeft, const glm::vec2& areaSize)
{
	if (!isPaused)
		return;

	const float unit = areaSize.y;
	const glm::vec2 center = areaTopLeft + areaSize * 0.5f;

	UIRenderer::DrawRectangle(areaTopLeft, areaSize, ShadeColor);

	const float titleY = center.y - unit * 0.17f;
	// The game's framed panel behind the title and the buttons, when its HUD style has one
	const bool isFramed = HUDSystem::DrawMenuPanel(glm::vec2(center.x - unit * ButtonWidth * 0.9f, titleY - unit * TitleHeight * 1.1f),
		glm::vec2(unit * ButtonWidth * 1.8f, unit * (TitleHeight * 2.2f + 0.06f + ButtonCount * (ButtonHeight + ButtonGap)) + unit * 0.12f));
	UIRenderer::DrawTextCentered("PAUSED", glm::vec2(center.x, titleY), unit * TitleHeight, TitleColor);
	const float lineWidth = unit * ButtonWidth * 1.3f;
	UIRenderer::DrawRectangle(glm::vec2(center.x - lineWidth * 0.5f, titleY + unit * TitleHeight * 0.75f), glm::vec2(lineWidth, 2.0f), LineColor);

	buttonSize = glm::vec2(unit * ButtonWidth, unit * ButtonHeight);
	float top = center.y - unit * 0.06f;
	for (int i = 0; i < ButtonCount; ++i)
	{
		const bool isSelected = i == selected;
		buttonTopLeft[i] = glm::vec2(center.x - buttonSize.x * 0.5f, top);

		if (isFramed)
		{
			// In a framed panel: the words alone, the chosen one on a soft band between two fading lines
			if (isSelected)
			{
				const glm::vec2 middle = buttonTopLeft[i] + buttonSize * 0.5f;
				UIRenderer::DrawFadeBand(middle, glm::vec2(buttonSize.x * 1.3f, buttonSize.y), glm::vec4(0.55f, 0.3f, 0.85f, 0.28f));
				UIRenderer::DrawFadeLine(middle - glm::vec2(0.0f, buttonSize.y * 0.5f), buttonSize.x, 1.0f, LineColor);
				UIRenderer::DrawFadeLine(middle + glm::vec2(0.0f, buttonSize.y * 0.5f), buttonSize.x, 1.0f, LineColor);
			}
		}
		else
		{
			UIRenderer::DrawRectangle(buttonTopLeft[i], buttonSize, isSelected ? SelectedColor : ButtonColor);
			UIRenderer::DrawRectangleOutline(buttonTopLeft[i], buttonSize, isSelected ? 2.0f : 1.0f, LineColor);
		}
		UIRenderer::DrawTextCentered(Labels[i], buttonTopLeft[i] + buttonSize * 0.5f, unit * TextHeight,
			isSelected ? SelectedTextColor : TextColor);

		top += buttonSize.y + unit * ButtonGap;
	}

	UIRenderer::DrawTextCentered("Esc  resume      W/S  choose      Enter  confirm", glm::vec2(center.x, top + unit * 0.04f),
		unit * HintHeight, HintColor);
}

bool PauseMenu::TakeQuitRequest()
{
	const bool wasRequested = isQuitRequested;
	isQuitRequested = false;
	return wasRequested;
}

void PauseMenu::Reset()
{
	SetPaused(false);
	isQuitRequested = false;
}

}
