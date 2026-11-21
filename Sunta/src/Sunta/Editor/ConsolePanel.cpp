#include "Core/SuntaPreCompiled.h"
#include "ConsolePanel.h"

#include <imgui/imgui.h>

#include "Core/LogHistory.h"

namespace Sunta
{

namespace
{

ImVec4 ColorOf(LogHistory::Level level)
{
	switch (level)
	{
	case LogHistory::Level::Error:   return ImVec4(1.0f, 0.42f, 0.38f, 1.0f);
	case LogHistory::Level::Warning: return ImVec4(1.0f, 0.82f, 0.4f, 1.0f);
	default:                         return ImVec4(0.82f, 0.8f, 0.74f, 1.0f);
	}
}

bool ContainsIgnoringCase(const std::string& text, const char* part)
{
	if (part[0] == '\0')
		return true;

	const std::string lowerPart = [&] { std::string value(part); for (char& c : value) c = (char)std::tolower((unsigned char)c); return value; }();
	std::string lowerText = text;
	for (char& c : lowerText)
		c = (char)std::tolower((unsigned char)c);
	return lowerText.find(lowerPart) != std::string::npos;
}

}

void ConsolePanel::Draw()
{
	if (ImGui::Button("Clear"))
		LogHistory::Clear();
	ImGui::SameLine();
	ImGui::Checkbox("Info", &showsInfo);
	ImGui::SameLine();
	ImGui::Checkbox("Warnings", &showsWarnings);
	ImGui::SameLine();
	ImGui::Checkbox("Errors", &showsErrors);
	ImGui::SameLine();
	ImGui::Checkbox("Follow", &followsNewest);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	ImGui::InputTextWithHint("##ConsoleFilter", "Filter", filter, sizeof(filter));
	ImGui::Separator();

	ImGui::BeginChild("ConsoleLines", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

	for (const LogHistory::Line& line : LogHistory::Copy())
	{
		const bool isShown = (line.level == LogHistory::Level::Info && showsInfo)
		                  || (line.level == LogHistory::Level::Warning && showsWarnings)
		                  || (line.level == LogHistory::Level::Error && showsErrors);
		if (!isShown || !ContainsIgnoringCase(line.text, filter))
			continue;

		ImGui::PushStyleColor(ImGuiCol_Text, ColorOf(line.level));
		ImGui::TextUnformatted(line.text.c_str());
		ImGui::PopStyleColor();
	}

	// New lines came: down to them (unless the panel was scrolled up to read something)
	const size_t added = LogHistory::GetAddedCountNow();
	if (followsNewest && added != linesSeen)
		ImGui::SetScrollHereY(1.0f);
	linesSeen = added;

	ImGui::EndChild();
}

}
