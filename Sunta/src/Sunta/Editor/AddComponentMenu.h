#pragma once

namespace Sunta
{

class EntityManager;

// The popup of the Inspector's "Add Component" button
//
// At the top a search field (it has the keyboard at once, Enter adds the first match), then the presets (ready sets of
// components: a whole player, an enemy, a bonfire) and the components in groups (Rendering, Physics, Character, ...)
// Every row shows the component's icon (the same as its header in the Inspector) and a short description when hovered;
// components the entity already has are greyed out
// A new component = one row in the table in AddComponentMenu.cpp
class AddComponentMenu
{
public:
	// Call every frame between BeginPopup / EndPopup of the popup. True when a component was added
	static bool Draw(EntityManager& entityManager, unsigned int entityID);
};

}
