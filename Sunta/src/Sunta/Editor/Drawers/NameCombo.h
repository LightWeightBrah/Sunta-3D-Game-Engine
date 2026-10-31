#pragma once

#include <map>
#include <string>
#include <vector>

namespace Sunta
{

class Animation;

// A dropdown of names with "(none)" on top (choosing it empties the name). Returns true when another one was chosen
bool DrawNameCombo(const char* label, std::string& chosenName, const std::vector<std::string>& names);

// The same for the animations of a Model
bool DrawAnimationCombo(const char* label, std::string& chosenName, const std::map<std::string, Animation>& animations);

}
