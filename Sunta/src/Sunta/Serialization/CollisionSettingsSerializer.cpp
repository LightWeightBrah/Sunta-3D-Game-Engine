#include "Core/SuntaPreCompiled.h"
#include "CollisionSettingsSerializer.h"

#include "Core/Log.h"

namespace Sunta
{

namespace
{

constexpr const char* LayersKey = "collision_layers";
constexpr const char* MatrixKey = "collision_matrix";

const std::string PairSeparatorWithSpaces = std::string(" ") + CollisionPairSeparator + " ";

CollisionLayerSet GetDefaultLayers()
{
	return CollisionSettings().GetLayers();
}

std::string TrimSpaces(const std::string& text)
{
	const size_t firstCharacter = text.find_first_not_of(' ');
	if (firstCharacter == std::string::npos)
		return "";

	const size_t lastCharacter = text.find_last_not_of(' ');
	return text.substr(firstCharacter, lastCharacter - firstCharacter + 1);
}

//=====READING=====//

bool IsValidSlot(const nlohmann::json& slotJson)
{
	return slotJson.is_number_unsigned() && slotJson.get<unsigned int>() < MaxCollisionLayers;
}

bool TryAddLayerFromJson(CollisionLayerSet& layers, const std::string& layerName, const nlohmann::json& slotJson)
{
	return IsValidSlot(slotJson) && layers.TryAddAt(LayerInSlot(slotJson.get<unsigned int>()), layerName);
}

CollisionLayerSet ReadLayerList(const nlohmann::json& layersJson)
{
	CollisionLayerSet layers;

	if (!layersJson.is_object())
		return layers;

	for (const auto& [layerName, slotJson] : layersJson.items())
	{
		if (!TryAddLayerFromJson(layers, layerName, slotJson))
			SUNTA_ENGINE_LOG_WARNING("CollisionSettings: Ignoring layer '{0}' in '{1}', its name or slot is not valid", layerName, LayersKey);
	}

	return layers;
}

// New colliders and old scene files use the default layer, so a project can't live without it
CollisionLayerSet ReadLayers(const nlohmann::json& configJson)
{
	if (!configJson.contains(LayersKey))
		return GetDefaultLayers();

	const CollisionLayerSet layers = ReadLayerList(configJson[LayersKey]);

	if (layers.Contains(DefaultCollisionLayer))
		return layers;

	SUNTA_ENGINE_LOG_ERROR("CollisionSettings: '{0}' has no layer in slot 0, the default layers are used", LayersKey);
	return GetDefaultLayers();
}

bool TryParseLayerPair(const std::string& pairText, const CollisionLayerSet& layers, CollisionLayer& outLayerA, CollisionLayer& outLayerB)
{
	const size_t separatorPosition = pairText.find(CollisionPairSeparator);
	if (separatorPosition == std::string::npos)
		return false;

	const std::string layerNameA = TrimSpaces(pairText.substr(0, separatorPosition));
	const std::string layerNameB = TrimSpaces(pairText.substr(separatorPosition + 1));

	return layers.TryFindByName(layerNameA, outLayerA) && layers.TryFindByName(layerNameB, outLayerB);
}

void ReadRule(const nlohmann::json& pairJson, CollisionSettings& settings)
{
	CollisionLayer layerA = CollisionLayer::None;
	CollisionLayer layerB = CollisionLayer::None;

	const bool isValidPair = pairJson.is_string() && TryParseLayerPair(pairJson.get<std::string>(), settings.GetLayers(), layerA, layerB);

	if (isValidPair)
		settings.SetCanCollide(layerA, layerB, true);
	else
		SUNTA_ENGINE_LOG_WARNING("CollisionSettings: Ignoring '{0}' in '{1}', it is not a pair of existing collision layers", pairJson.dump(), MatrixKey);
}

void ReadRules(const nlohmann::json& configJson, CollisionSettings& settings)
{
	if (!configJson.contains(MatrixKey) || !configJson[MatrixKey].is_array())
	{
		SUNTA_ENGINE_LOG_WARNING("CollisionSettings: '{0}' is missing, so no layers collide", MatrixKey);
		return;
	}

	for (const auto& pairJson : configJson[MatrixKey])
		ReadRule(pairJson, settings);
}

//=====WRITING=====//

nlohmann::ordered_json WriteLayers(const CollisionLayerSet& layers)
{
	nlohmann::ordered_json layersJson = nlohmann::ordered_json::object();

	for (CollisionLayer layer : layers.GetLayers())
	{
		unsigned int slot = 0;
		TryGetLayerSlot(layer, slot);

		layersJson[layers.GetName(layer)] = slot;
	}

	return layersJson;
}

std::string MakePairText(const CollisionLayerSet& layers, CollisionLayer layerA, CollisionLayer layerB)
{
	return layers.GetName(layerA) + PairSeparatorWithSpaces + layers.GetName(layerB);
}

// Every pair is written once (A + B, never B + A)
nlohmann::ordered_json WriteRules(const CollisionSettings& settings)
{
	nlohmann::ordered_json pairsJson = nlohmann::ordered_json::array();

	const std::vector<CollisionLayer> layers = settings.GetLayers().GetLayers();

	for (size_t firstIndex = 0; firstIndex < layers.size(); firstIndex++)
	{
		for (size_t secondIndex = firstIndex; secondIndex < layers.size(); secondIndex++)
		{
			if (settings.CanCollide(layers[firstIndex], layers[secondIndex]))
				pairsJson.push_back(MakePairText(settings.GetLayers(), layers[firstIndex], layers[secondIndex]));
		}
	}

	return pairsJson;
}

}

CollisionSettings CollisionSettingsSerializer::Deserialize(const nlohmann::json& configJson, const CollisionSettings& fallback)
{
	if (!configJson.contains(LayersKey) && !configJson.contains(MatrixKey))
		return fallback;

	// Rules without a list of layers were written for the default layers, ReadLayers gives those
	CollisionSettings settings(ReadLayers(configJson));

	ReadRules(configJson, settings);

	return settings;
}

void CollisionSettingsSerializer::Serialize(nlohmann::ordered_json& configJson, const CollisionSettings& settings)
{
	configJson[LayersKey] = WriteLayers(settings.GetLayers());
	configJson[MatrixKey] = WriteRules(settings);
}

}
