#pragma once

#include <map>
#include <string>

namespace Sunta
{

class Animation;

using AnimationMap = std::map<std::string, Animation>;

// The first animation (in alphabetical order) whose name contains the word, ignoring upper and lower case
// (Idle, idle_01, Armature|Idle, ...). False when there is none
bool TryFindAnimationContaining(const AnimationMap& animations, const std::string& keyword, std::string& outName);

}
