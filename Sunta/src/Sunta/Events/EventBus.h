#pragma once
#include <functional>
#include <map>
#include <typeindex>

namespace Sunta
{

class EventBus
{
public:
	template<typename T>
	static unsigned int Subscribe(std::function<void(const T&)> callback)
	{
		allIDs++;
		unsigned int newID = allIDs;

		auto callbackFunction = [callback](const void* eventData){
				callback(*static_cast<const T*>(eventData));
		};

		allSubscribers[typeid(T)].push_back({ newID, callbackFunction });

		return newID;
	}

	static void Unsubscribe(unsigned int id)
	{
		for (auto& [type, handlers] : allSubscribers)
		{
			for (auto it = handlers.begin(); it != handlers.end(); it++)
			{
				if (it->id == id)
				{
					handlers.erase(it);
					return;
				}
			}
		}
	}

	template<typename T>
	static void Publish(const T& event)
	{
		auto type = std::type_index(typeid(T));

		auto it = allSubscribers.find(type);
		if (it != allSubscribers.end())
		{
			for (auto& handler : it->second)
				handler.callback(&event);
		}
	}

private:
	struct EventHandler
	{
		unsigned int id;
		std::function<void(const void*)> callback;
	};


	//inline static makes sure there's only 1 definition for this variables in all files
	//inline static basically creates variable declaration and definition 
	//so we dont need definition in .cpp
	inline static unsigned int allIDs = 0;
	inline static std::map<std::type_index, std::vector<EventHandler>> allSubscribers;
};

}