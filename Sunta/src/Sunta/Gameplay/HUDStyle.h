#pragma once

#include <memory>
#include <string>
#include <glm/glm.hpp>

namespace Sunta
{

class Texture;

// How one bar of the HUD looks with a picture: a frame that grows with the bar (its ends keep their shape)
// and the channel inside it where the fill goes. All numbers in pixels of the frame's texture
struct HUDBarStyle
{
	std::shared_ptr<Texture> frame;

	// How wide the ornate left and right ends are: they are never stretched, only the middle is
	float capLeft  = 0.0f;
	float capRight = 0.0f;

	// The channel inside the frame: its distance from the left, top, right and bottom edge of the texture
	glm::vec4 inner = glm::vec4(0.0f);

	// How tall the whole frame is on the screen (pixels), and where its top left corner is (from the HUD area's corner)
	float     height = 40.0f;
	glm::vec2 offset = glm::vec2(0.0f);

	// An optional picture over the middle of the bar (a crest on the boss bar), as tall as the frame
	std::shared_ptr<Texture> ornament;

	bool IsUsable() const { return frame != nullptr; }
};

// How the HUD looks: the pictures and colors of the player's bars, the boss bar and the corners
// (the flask and the currency), read from a file of the game, e.g. @game/UI/HUD/hud_style.json
// Without a style (or with a picture missing) the HUD draws the simple flat bars instead
struct HUDStyle
{
	HUDBarStyle health;
	HUDBarStyle stamina;
	HUDBarStyle boss;

	// Laid over the filled part of each bar, tinted with the bar's color (a grey picture with light and shade)
	std::shared_ptr<Texture> fill;
	glm::vec4 healthColor  = glm::vec4(0.62f, 0.07f, 0.05f, 1.0f);
	glm::vec4 staminaColor = glm::vec4(0.24f, 0.48f, 0.17f, 1.0f);
	glm::vec4 trailColor   = glm::vec4(0.85f, 0.70f, 0.40f, 1.0f);
	glm::vec4 channelColor = glm::vec4(0.025f, 0.02f, 0.02f, 0.94f);

	// The corners: a panel stretched by nine-slice (its 'panelBorder' pixels keep their shape), an icon on its left
	std::shared_ptr<Texture> panel;
	float     panelBorder = 24.0f;
	glm::vec2 cornerSize  = glm::vec2(210.0f, 78.0f);
	float     iconHeight  = 64.0f;
	std::shared_ptr<Texture> flaskIcon;
	std::shared_ptr<Texture> currencyIcon;

	// ========== Optional parts (a style without them keeps the plain look) ==========

	// The UI's font (a TTF of the game) and the letter spacing of titles (the boss's name, the corners' labels), a part
	// of the letter height
	std::string fontPath;
	float       titleSpacing = 0.0f;

	// A third bar under the stamina, for the Wrath (see WrathComponent); its fill color
	HUDBarStyle wrath;
	glm::vec4   wrathColor = glm::vec4(0.62f, 0.3f, 0.95f, 1.0f);

	// An emblem beside the bars (a crown, a sigil): its picture, and a picture of it lit that is revealed from the
	// bottom as the Wrath fills (it pulses when full, burns while set loose). 'emblemSize' on the screen
	std::shared_ptr<Texture> emblem;
	std::shared_ptr<Texture> emblemLit;
	glm::vec2 emblemSize   = glm::vec2(0.0f);
	glm::vec2 emblemOffset = glm::vec2(0.0f);

	// Quick slots in the bottom left (as Elden Ring's): a framed square per item the player can use - the flask (its
	// charges), the summon (its calls) - with its key under it. 'slotSize' on the screen; without 'slotFrame' the
	// corner panel shows the flask instead
	std::shared_ptr<Texture> slotFrame;
	glm::vec2 slotSize   = glm::vec2(84.0f);
	float     slotGap    = 14.0f;
	std::shared_ptr<Texture> summonIcon;

	// The currency drawn plain in the bottom right (its icon and its number, no panel)
	bool isCurrencyPlain = false;

	// The boss's name centered over its bar (else at its left end), and the posture bars with a mark in their middle
	// (Sekiro's diamond)
	bool isBossNameCentered = false;
	// How long the boss bar's channel is, a part of the screen's width (0 = the engine's)
	float bossWidth = 0.0f;
	// The mark on the locked-on enemy (a picture centered on its aim point, 'lockOnSize' pixels); none = a small square
	std::shared_ptr<Texture> lockOnMark;
	// The panel behind menus and read texts (the level up, a book, the pause menu): a nine-slice picture, its border
	std::shared_ptr<Texture> menuPanel;
	float menuPanelBorder = 40.0f;
	float lockOnSize = 28.0f;
	bool hasPostureMark     = false;

	// The style read from the file, kept after the first time. nullptr for an empty path or a file that can't be read
	// (the reason is logged once)
	static const HUDStyle* Get(const std::string& path);
};

}
