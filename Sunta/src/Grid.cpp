#include <iostream>
#include <fstream>
#include "Grid.h"

void RenderMap(const std::vector<std::vector<Node>>& grid, Node* startNode, Node* endNode)
{
	for (const auto& row : grid)
	{
		for (const auto& node : row)
		{
			if (&node == startNode)
				std::cout << "OO";
			else if (&node == endNode)
				std::cout << "XX";
			else
			{
				switch (node.cellType)
				{
				case CELL::WALL:
					std::cout << "||";
					break;
				case CELL::ROUTE:
					std::cout << "->";
					break;
				default:
					std::cout << "  ";
					break;
				}
			}
		}
		std::cout << std::endl;
	}
}

void PrintGrid(const std::vector<std::vector<Node>>& grid)
{
	for (const auto& row : grid)
	{
		for (const auto& node : row)
		{
			std::cout << (int)node.cellType << " ";
		}
		std::cout << std::endl;
	}
}

std::vector<std::vector<Node>> BuildGrid(std::string templateString, int& width, int& height)
{
	std::string cleanText = TrimGrid(templateString);
	
	GetGridDimensions(cleanText, width, height);
	return InitGrid(cleanText, width, height);
}

std::vector<std::vector<Node>> SetupWorld(std::string path, int& width, int& height)
{
	if (path.empty())
	{
		std::cout << "No file provided...\n" << std::endl;
		std::cout << "Opening with default 30x20 grid template...\n" << std::endl;
		return BuildGrid(MapTemplates::grid30x20, width, height);
	}

	std::ifstream file(path);
	
	if(!file.is_open())
	{
		std::cout << "ERROR: COULND'T OPEN FILE (\"" << path << "\")" << std::endl;
		std::cout << "Opening with default 30x20 grid template...\n" << std::endl;
		return BuildGrid(MapTemplates::grid30x20, width, height);
	}

	std::string content = "";
	std::string line = "";
	while (std::getline(file, line))
	{
		content += line + "\n";
	}
	file.close();
	
	std::cout << "Read file successfully!" << std::endl;
	return BuildGrid(content, width, height);
}

void PrintMapError(int y, int width, std::string issue)
{
	std::cout << "\nERROR: MAP ERROR!!!" << std::endl;
	std::cout << "Line " << y + 1 << " is " << issue << std::endl;
	std::cout << "Expected width: " << width << std::endl;
}

std::vector < std::vector<Node>> InitGrid(std::string& gridText, int& width, int& height)
{
	std::vector < std::vector<Node>> grid;
	int charIndex = 0;

	for (int y = 0; y < height; y++)
	{
		std::vector<Node> singleRow;
		while (charIndex < gridText.length() && gridText[charIndex] == '\n')
			charIndex++;

		for (int x = 0; x < width; x++)
		{
			if (charIndex >= gridText.length() || gridText[charIndex] == '\n')
			{
				PrintMapError(y, width, "too short");
				return std::vector<std::vector<Node>>();
			}

			char currentCell = gridText[charIndex];
			singleRow.push_back(Node(x, y, (CELL)(currentCell - '0')));
			charIndex++;
		}

		if (charIndex < gridText.length() && gridText[charIndex] != '\n')
		{
			PrintMapError(y, width, "too long");
			return std::vector<std::vector<Node>>();
		}

		if (!singleRow.empty())
			grid.push_back(singleRow);
	}

	return grid;
}


void GetGridDimensions(std::string& gridText, int& width, int& height)
{
	if (gridText.empty())
	{
		std::cout << "ERROR: MAP IS EMPTY!" << std::endl;
		return;
	}

	width = 0;
	height = 1;
	bool widthFound = false;
	int i = 0;

	while (gridText[i] != '\0')
	{
		if (gridText[i] == '\n')
		{
			if (gridText[i + 1] != '\0' && gridText[i + 1] != '\n')
			{
				height++;
			}
			widthFound = true;
		}
		else if (!widthFound)
		{
			width++;
		}

		i++;
	}
}

std::string TrimGrid(std::string girdTemplate)
{
	std::string output = "";
	for (char c : girdTemplate)
	{
		if ((c >= '0' && c <= '9') || c == '\n')
			output += c;
	}

	return output;
}