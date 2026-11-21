#pragma once

#include <deque>
#include <mutex>
#include <string>

namespace Sunta
{

// The last lines of the log kept in memory, for the editor's Console panel (the editor has no console window)
class LogHistory
{
public:
	enum class Level
	{
		Error = 0,
		Warning,
		Info
	};

	struct Line
	{
		Level       level;
		std::string text;
	};

	static constexpr size_t MaxLines = 2000;

	// Called by the loggers for every line
	static void Add(Level level, const std::string& text)
	{
		std::lock_guard<std::mutex> lock(GetMutex());
		std::deque<Line>& lines = GetLines();
		lines.push_back({ level, text });
		if (lines.size() > MaxLines)
			lines.pop_front();
		++GetAddedCount();
	}

	// A copy of the lines (the log may be written by another thread meanwhile)
	static std::deque<Line> Copy()
	{
		std::lock_guard<std::mutex> lock(GetMutex());
		return GetLines();
	}

	static void Clear()
	{
		std::lock_guard<std::mutex> lock(GetMutex());
		GetLines().clear();
	}

	// Grows with every line ever added: the panel scrolls down when it changes
	static size_t GetAddedCountNow()
	{
		std::lock_guard<std::mutex> lock(GetMutex());
		return GetAddedCount();
	}

private:
	static std::mutex& GetMutex()
	{
		static std::mutex mutex;
		return mutex;
	}

	static std::deque<Line>& GetLines()
	{
		static std::deque<Line> lines;
		return lines;
	}

	static size_t& GetAddedCount()
	{
		static size_t count = 0;
		return count;
	}
};

}
