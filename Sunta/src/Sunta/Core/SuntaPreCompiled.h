#pragma once

//=====SUNTA PRECOMPILED HEADER=====//

//=====     WINDOWS     ======//

#ifdef SUNTA_PLATFORM_WINDOWS
	
	#define WIN32_LEAN_AND_MEAN  //WINDOWS CONFLICTING NAMES FIXES
	#define NOMINMAX			 //WINDOWS CONFLICTING NAMES FIXES

	#include <Windows.h>

	#undef ERROR	             //WINDOWS CONFLICTING NAMES FIXES
	#undef CreateWindow          //WINDOWS CONFLICTING NAMES FIXES

#endif

//=====     WINDOWS     ======//


#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include <vector>
#include <map>
#include <unordered_map>
#include <queue>
#include <any>

#include <memory>
#include <utility>
#include <functional>
#include <algorithm>
#include <typeindex>

#include <ctime>
#include <cmath>
#include <cstring>

#include <iomanip>
#include <chrono>