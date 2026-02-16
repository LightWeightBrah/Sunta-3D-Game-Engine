#pragma once
#include <vector>
#include <functional>

class Event
{
public:
	void AddListener (std::function<void()> callback);
	void Invoke();

private:
	std::vector<std::function<void()>> listeners;
};