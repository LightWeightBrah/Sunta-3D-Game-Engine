#include "Core/SuntaPreCompiled.h"
#include "HUDStyle.h"

#include <fstream>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "Core/Log.h"
#include "Core/VirtualFileSystem.h"
#include "Renderer/UIRenderer.h"

namespace Sunta
{

namespace
{

using json = nlohmann::json;

// The keys of the style file
namespace StyleKeys
{
	constexpr const char* Health       = "health";
	constexpr const char* Stamina      = "stamina";
	constexpr const char* Boss         = "boss";
	constexpr const char* Frame        = "frame";
	constexpr const char* CapLeft      = "cap_left";
	constexpr const char* CapRight     = "cap_right";
	constexpr const char* Inner        = "inner";
	constexpr const char* Height       = "height";
	constexpr const char* Offset       = "offset";
	constexpr const char* Ornament     = "ornament";
	constexpr const char* Fill         = "fill";
	constexpr const char* HealthColor  = "health_color";
	constexpr const char* StaminaColor = "stamina_color";
	constexpr const char* TrailColor   = "trail_color";
	constexpr const char* ChannelColor = "channel_color";
	constexpr const char* Panel        = "panel";
	constexpr const char* PanelBorder  = "panel_border";
	constexpr const char* CornerSize   = "corner_size";
	constexpr const char* IconHeight   = "icon_height";
	constexpr const char* FlaskIcon    = "flask_icon";
	constexpr const char* CurrencyIcon = "currency_icon";
}

std::unordered_map<std::string, std::unique_ptr<HUDStyle>> styles;

glm::vec2 ReadVec2(const json& data, const char* key, const glm::vec2& fallback)
{
	if (!data.contains(key) || !data[key].is_array() || data[key].size() < 2)
		return fallback;
	return glm::vec2(data[key][0].get<float>(), data[key][1].get<float>());
}

glm::vec4 ReadVec4(const json& data, const char* key, const glm::vec4& fallback)
{
	if (!data.contains(key) || !data[key].is_array() || data[key].size() < 3)
		return fallback;

	const json& value = data[key];
	return glm::vec4(value[0].get<float>(), value[1].get<float>(), value[2].get<float>(), value.size() > 3 ? value[3].get<float>() : 1.0f);
}

std::shared_ptr<Texture> ReadImage(const json& data, const char* key, const std::string& stylePath)
{
	const std::string path = data.value(key, std::string());
	std::shared_ptr<Texture> image = UIRenderer::GetImage(path);

	if (!path.empty() && !image)
		SUNTA_ENGINE_LOG_WARNING("HUD style '{0}': the picture '{1}' ({2}) can't be read", stylePath, path, key);
	return image;
}

void ReadBar(const json& data, const char* key, HUDBarStyle& bar, const std::string& stylePath)
{
	if (!data.contains(key))
		return;

	const json& barData = data[key];
	bar.frame    = ReadImage(barData, StyleKeys::Frame, stylePath);
	bar.ornament = ReadImage(barData, StyleKeys::Ornament, stylePath);
	bar.capLeft  = barData.value(StyleKeys::CapLeft,  bar.capLeft);
	bar.capRight = barData.value(StyleKeys::CapRight, bar.capRight);
	bar.height   = barData.value(StyleKeys::Height,   bar.height);
	bar.inner    = ReadVec4(barData, StyleKeys::Inner, bar.inner);
	bar.offset   = ReadVec2(barData, StyleKeys::Offset, bar.offset);
}

std::unique_ptr<HUDStyle> Load(const std::string& path)
{
	std::ifstream file(VirtualFileSystem::Resolve(path));
	if (!file)
	{
		SUNTA_ENGINE_LOG_WARNING("HUD style: the file '{0}' can't be opened, the HUD stays simple", path);
		return nullptr;
	}

	const json data = json::parse(file, nullptr, false);
	if (data.is_discarded() || !data.is_object())
	{
		SUNTA_ENGINE_LOG_WARNING("HUD style: '{0}' is not a valid JSON object, the HUD stays simple", path);
		return nullptr;
	}

	auto style = std::make_unique<HUDStyle>();

	ReadBar(data, StyleKeys::Health,  style->health,  path);
	ReadBar(data, StyleKeys::Stamina, style->stamina, path);
	ReadBar(data, StyleKeys::Boss,    style->boss,    path);

	style->fill         = ReadImage(data, StyleKeys::Fill, path);
	style->healthColor  = ReadVec4(data, StyleKeys::HealthColor,  style->healthColor);
	style->staminaColor = ReadVec4(data, StyleKeys::StaminaColor, style->staminaColor);
	style->trailColor   = ReadVec4(data, StyleKeys::TrailColor,   style->trailColor);
	style->channelColor = ReadVec4(data, StyleKeys::ChannelColor, style->channelColor);

	style->panel        = ReadImage(data, StyleKeys::Panel, path);
	style->panelBorder  = data.value(StyleKeys::PanelBorder, style->panelBorder);
	style->cornerSize   = ReadVec2(data, StyleKeys::CornerSize, style->cornerSize);
	style->iconHeight   = data.value(StyleKeys::IconHeight, style->iconHeight);
	style->flaskIcon    = ReadImage(data, StyleKeys::FlaskIcon, path);
	style->currencyIcon = ReadImage(data, StyleKeys::CurrencyIcon, path);

	SUNTA_ENGINE_LOG_INFO("HUD style: '{0}' loaded", path);
	return style;
}

}

const HUDStyle* HUDStyle::Get(const std::string& path)
{
	if (path.empty())
		return nullptr;

	auto found = styles.find(path);
	if (found == styles.end())
		found = styles.emplace(path, Load(path)).first;

	return found->second.get();
}

}
