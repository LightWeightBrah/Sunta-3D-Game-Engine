#pragma once

//=====SUNTA PRECOMPILED HEADER=====//

//=====     WINDOWS     ======//

#ifdef SUNTA_PLATFORM_WINDOWS
	
	#define WIN32_LEAN_AND_MEAN  //WINDOWS CONFLICTING NAMES FIXES
	#define NOMINMAX			 //WINDOWS CONFLICTING NAMES FIXES

	#include <Windows.h>
	#include <shellapi.h>

	#undef ERROR	             //WINDOWS CONFLICTING NAMES FIXES
	#undef CreateWindow          //WINDOWS CONFLICTING NAMES FIXES

#endif

//=====     WINDOWS     ======//


#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>

#include <array>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <any>

#include <memory>
#include <limits>
#include <utility>
#include <functional>
#include <algorithm>
#include <typeindex>
#include <filesystem>

#include <ctime>
#include <cmath>
#include <cstring>

#include <iomanip>
#include <chrono>