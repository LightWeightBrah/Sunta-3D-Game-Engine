#include <algorithm>

#include "Event.h"

void Event::AddListener(std::function<void()> callback)
{
	listeners.push_back(callback);
}

void Event::Invoke()
{
	for (auto& callback : listeners)
	{
		if(callback)
			callback();
	}
}