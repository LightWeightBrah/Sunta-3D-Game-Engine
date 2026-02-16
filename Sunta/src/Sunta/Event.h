#pragma once
#include <vector>
#include <functional>

namespace Sunta
{
	class Event
	{
	public:
		void AddListener (std::function<void()> callback);
		void Invoke();
	
	private:
		std::vector<std::function<void()>> listeners;
	};
}