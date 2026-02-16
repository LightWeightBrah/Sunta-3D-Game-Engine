#include <iostream>

#include "AStar.h"
#include "Grid.h"

AStar::AStar()
{
	grid = SetupWorld("", width, height);
	if (grid.empty())
	{
		std::cout << "A Star won't be able to run. Please fix the map file..." << std::endl;
		return;
	}
}

AStar::AStar(std::string gridFilepath)
{
	grid = SetupWorld(gridFilepath, width, height);
	if (grid.empty())
	{
		std::cout << "A Star won't be able to run. Please fix the map file..." << std::endl;
		return;
	}
}

AStar::AStar(std::string gridFilepath, int startX, int startY, int endX, int endY) 
	: startX(startX), startY(startY), endX(endX), endY(endY)
{
	grid = SetupWorld(gridFilepath, width, height);
	if (grid.empty())
	{
		std::cout << "A Star won't be able to run. Please fix the map file..." << std::endl;
		return;
	}
}

void AStar::FindPathBySteps(float deltaTime)
{
	if (!ValidateStartEndCoordinates())
		return;

	if (finishedDrawing)
		return;

	Node* startNode = &grid[startY][startX];
	Node* endNode = &grid[endY][endX];

	astarCounter += deltaTime;
	if (astarCounter >= astarInterval)
	{
		astarCounter = 0;
		if (!pathFound)
		{
			pathFound = RunAStarStep();
			if (pathFound)
			{
				drawingPath = true;
				std::cout << "\nYOU FOUND A PATH, STARTING DRAWING..." << std::endl;
			}
		}
		else if(drawingPath)
		{
			TraverseBackToStartSteps();

		}
	}
}

bool AStar::RunAStarStep()
{
	if (pathFound)
		return true;

	if (closedList.empty())
		closedList.push_back(startNode);

	Node* currentNode = closedList.back();
	if (currentNode == endNode)
		return true;

	switch (neighbourStep)
	{
	case 0:
		if (currentNode->y + 1 < height)
			CheckNeighbour(&grid[currentNode->y + 1][currentNode->x], currentNode);
		break;

	case 1:
		if (currentNode->x - 1 >= 0)
			CheckNeighbour(&grid[currentNode->y][currentNode->x - 1], currentNode);
		break;

	case 2:
		if (currentNode->y - 1 >= 0)
			CheckNeighbour(&grid[currentNode->y - 1][currentNode->x], currentNode);
		break;

	case 3:
		if (currentNode->x + 1 < width)
			CheckNeighbour(&grid[currentNode->y][currentNode->x + 1], currentNode);
		break;
	}

	neighbourStep++;

	if (neighbourStep > 3)
	{
		neighbourStep = 0;

		currentNode = FindLowestCostNode();
		if (!currentNode)
			return false;

		UpdateLists(currentNode);
		if (currentNode == endNode)
			return true;
	}

	return false;
}

void AStar::TraverseBackToStartSteps()
{
	if (pathTracker == nullptr)
	{
		pathTracker = endNode;
		fullPath.push_back(pathTracker);
	}

	if (pathTracker == startNode)
	{
		std::reverse(fullPath.begin(), fullPath.end());

		drawingPath = false;
		finishedDrawing = true;
		return;
	}

	pathTracker->cellType = CELL::ROUTE;
	pathTracker = pathTracker->parent;
	fullPath.push_back(pathTracker);
	
}

std::vector<GridPoint> AStar::GetFullPath()
{
	if (fullPath.empty())
		return {};

	std::vector<GridPoint> returnPath;
	for (auto& node : fullPath)
		returnPath.push_back({ node->x, node->y });

	fullPath.clear();

	return returnPath;
}

void AStar::FindPathFull()
{
	if (grid.empty() || grid[0].empty())
	{
		std::cout << "ERROR: GRID IS EMPTY! CHECK YOUR MAP .TXT FILE!";
		return;
	}

	if(!ValidateStartEndCoordinates())
		return;

	startNode = &grid[startY][startX];
	endNode	  = &grid[endY][endX];

	PrintCoordinates();

	pathFound = RunAStarFull();
	if (pathFound)
	{
		TraverseBackToStartFull();
		RenderMap(grid, startNode, endNode);
		std::cout << "\nYOU FOUND A PATH, LET'S GO!!!" << std::endl;
	}
	else
	{
		std::cout << "\nCouldn't find a path :C" << "Change map template..." << std::endl;
	}
}

bool AStar::RunAStarFull()
{
	closedList.push_back(startNode);
	Node* currentNode = closedList.back();

	while (currentNode != endNode)
	{
		if (currentNode->y + 1 < height)
			CheckNeighbour(&grid[currentNode->y + 1][currentNode->x], currentNode);

		if (currentNode->x - 1 >= 0)
			CheckNeighbour(&grid[currentNode->y][currentNode->x - 1], currentNode);

		if (currentNode->y - 1 >= 0)
			CheckNeighbour(&grid[currentNode->y - 1][currentNode->x], currentNode);

		if (currentNode->x + 1 < width)
			CheckNeighbour(&grid[currentNode->y][currentNode->x + 1], currentNode);

		currentNode = FindLowestCostNode();

		if (currentNode == nullptr)
		{
			std::cout << "ERROR: THERE'S NO ROUTE TO END POINT!" << std::endl;
			return false;
		}

		UpdateLists(currentNode);

		if (currentNode == endNode) 
			return true;
	}

	return false;
}

void AStar::TraverseBackToStartFull()
{
	Node* currentNode = endNode;
	fullPath.push_back(currentNode);

	while (currentNode != nullptr && currentNode != startNode)
	{
		if (currentNode != endNode)
			currentNode->cellType = CELL::ROUTE;

		currentNode = currentNode->parent;
		fullPath.push_back(currentNode);
	}

	finishedDrawing = true;

	std::reverse(fullPath.begin(), fullPath.end());
}

void AStar::Reset()
{
	openList.clear();
	closedList.clear();
	fullPath.clear();

	pathFound = false;
	drawingPath = false;
	finishedDrawing = false;
	pathTracker = nullptr;
	neighbourStep = 0;
	astarCounter = 0.0f;
	modelCounter = modelInterval;

	startX = -1, startY = -1;
	endX   = -1, endY   = -1;

	startNode->Reset();
	endNode->Reset();

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			if (grid[y][x].cellType != CELL::WALL)
			{
				grid[y][x].Reset();
			}
		}
	}

	

	ValidateStartEndCoordinates();
}

void AStar::CheckNeighbour(Node* neighbour, Node* currentNode)
{
	if (neighbour->cellType == CELL::WALL)
		return;

	if (IsNodeInVector(closedList, neighbour))
		return;

	ResolveConflicts(*neighbour, *currentNode);

	if (!IsNodeInVector(openList, neighbour))
	{
		neighbour->cellType = CELL::OPEN;
		openList.push_back(neighbour);
	}
}

void AStar::ResolveConflicts(Node& neighbour, Node& currentNode)
{
	double gCost = currentNode.gCost + 1;
	double hCost = neighbour.CalculateHCost(neighbour.x, neighbour.y,
		endNode->x, endNode->y);
	double newFCost = gCost + hCost;

	if (newFCost < neighbour.fCost)
	{
		neighbour.gCost = gCost;
		neighbour.hCost = hCost;
		neighbour.fCost = newFCost;
		neighbour.SetParent(&grid[currentNode.y][currentNode.x]);
	}
}

Node* AStar::FindLowestCostNode()
{
	if (openList.empty())
		return nullptr;

	Node* lowest = openList[0];
	for (int i = 1; i < openList.size(); i++)
	{
		if (openList[i]->fCost <= lowest->fCost)
		{
			lowest = openList[i];
		}
	}

	return lowest;
}

void AStar::UpdateLists(Node* lowestCostNode)
{
	if (lowestCostNode == nullptr)
		return;

	RemoveItemFromVector(openList, lowestCostNode);
	lowestCostNode->cellType = CELL::CLOSED;
	closedList.push_back(lowestCostNode);
}



bool AStar::ValidateStartEndCoordinates()
{
	startX = (startX ==  -1) ?     0		: startX;
	startY = (startY ==  -1) ?  height - 1  : startY;
						 
	endX   = (endX   ==  -1) ?  width  - 1	: endX;
	endY   = (endY   ==  -1) ?	   0		: endY;

	if (startX >= width || startY >= height ||
		endX   >= width || endY   >= height ||
		startX <    0   || startY <    0    ||
		endX   <    0   || endY   <    0)
	{
		std::cout << "ERROR: COORDINATES OUT OF GRID BOUNDS!!!" << std::endl;
		return false;
	}

	startNode = &grid[startY][startX];
	endNode	  = &grid[endY][endX];

	if (startNode->cellType == CELL::WALL || endNode->cellType == CELL::WALL)
	{
		std::cout << "ERROR: Start or End is a WALL (5)!!!" << std::endl;
		return false;
	}

	return true;
}

void AStar::PrintCoordinates()
{
	std::cout << "Width is: (" << width << ") Height is: (" << height << ")" << std::endl;
	std::cout << "Start: [" << startX << ", " << startY << "] "
		<< "End: [" << endX << ", " << endY << "]" << std::endl;
}

void AStar::RemoveItemFromVector(std::vector<Node*>& vec, Node* node)
{
	vec.erase(std::remove(vec.begin(), vec.end(), node), vec.end());
}

bool AStar::IsNodeInVector(const std::vector<Node*>& vec, Node* node)
{
	for (Node* n : vec)
	{
		if (n == node)
			return true;
	}
	return false;
}