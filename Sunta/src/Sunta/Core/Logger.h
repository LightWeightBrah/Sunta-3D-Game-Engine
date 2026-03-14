#pragma once
#include <string>
#include <any>
#include <iostream>
#include <sstream>

#include "Terminal.h"

namespace Sunta {

class Logger
{
public:
	Logger(std::string&& name);

	enum class Level
	{
		ERROR = 0,
		WARNING,
		INFO
	};

	void SetLevel(Logger::Level level);

	template<typename... Args>
	void Info(const char* text, Args... args)
	{
		if (currentLevel < Level::INFO)
			return;

		Log(Terminal::Color::Green, text, args...);
	}

	template<typename... Args>
	void Warning(const char* text, Args... args)
	{
		if (currentLevel < Level::WARNING)
			return;

		Log(Terminal::Color::Yellow, text, args...);
	}

	template<typename... Args>
	void Error(const char* text, Args... args)
	{
		if (currentLevel < Level::ERROR)
			return;

		Log(Terminal::Color::Red, text, args...);
	}

private:
	std::string name;
	Level currentLevel;

	void InsertArgument(int& argIndex, const int argsCount, std::string& result, std::string* argsAsStrings);
	void InsertIndexedArgument(const char* text, int& nextPosition, const int argsCount, std::string& result, std::string* argsAsStrings);
	std::string GetTimeAsString();

	template<typename... Args>
	void Log(Terminal::Color color, const char* text, Args... args)
	{
		std::string message = GetLogMessage(text, args...);
	
		Terminal::SetColor(color);
		std::string timeString = GetTimeAsString();
		Terminal::Write("[" + timeString + "] " + "[" + name + "]" + ": " + message);
		Terminal::SetColor(Terminal::Color::White);
	}

	template <typename... Args>
	std::string GetLogMessage(const char* text, Args... args)
	{
		if (text == nullptr || text[0] == '\0')
			return "";

		const int argsCount = sizeof...(args);

		//constexpr deleted whole code below if the condition is true
		if (argsCount == 0)
			return std::string(text);

		std::string argsAsStrings[] = { ConvertTypeToString(args)... , ""}; //empty "" is to work with 0 arguments
		std::string result = "";
		int argIndex = 0;

		for (int i = 0; text[i] != '\0'; i++)
		{
			if (text[i] != '{')
			{
				result += text[i];
				continue;
			}

			if (text[i + 1] == '}')
			{
				InsertArgument(argIndex, argsCount, result, argsAsStrings);
				i++;
				continue;
			}

			if (isdigit(text[i + 1]))
			{
				int nextPosition = i + 1;
				InsertIndexedArgument(text, nextPosition, argsCount, result, argsAsStrings);
				i = nextPosition;
				continue;
			}

			result += text[i];
		}

		return result;
	}

	template <typename T>
	std::string ConvertTypeToString(const T& value)
	{
		std::stringstream ss;
		ss << value;
		return ss.str();
	}
};

}