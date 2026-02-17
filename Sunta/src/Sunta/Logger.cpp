#define _CRT_SECURE_NO_WARNINGS
#include <cstring>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <sstream>

#include "Logger.h"

namespace Sunta {

Logger::Logger(std::string&& name)
	: name(std::move(name))
	, currentLevel(Level::INFO)
{

}

void Logger::SetLevel(Logger::Level level)
{
	this->currentLevel = level;
}

void Logger::InsertArgument(int& argIndex, const int argsCount, std::string& result, std::string* argsAsStrings)
{
	if (argIndex < argsCount)
	{
		result += argsAsStrings[argIndex];
		argIndex++;
	}
}

void Logger::InsertIndexedArgument(const char* text, int& nextPosition, const int argsCount, std::string& result, std::string* argsAsStrings)
{
	std::string numberAsString = "";
	int tempPosition = nextPosition;

	while (isdigit(text[tempPosition]))
	{
		numberAsString += text[tempPosition];
		tempPosition++;
	}

	if (text[tempPosition] != '}')
		return;

	int number = std::stoi(numberAsString);

	if (number >= 0 && number < argsCount)
	{
		result += argsAsStrings[number];
	}

	nextPosition = tempPosition;
}

std::string Logger::GetTimeAsString()
{
	char buffer[64];
	auto currentTime = std::time(nullptr);
	auto currentLocalTime = *std::localtime(&currentTime);
	std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &currentLocalTime);

	std::stringstream ss;
	ss << buffer;

	return ss.str();
}

}