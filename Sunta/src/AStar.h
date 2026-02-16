#pragma once
#include <vector>
#include <string>

#include "Node.h"

struct GridPoint
{
	int x, z;
};

class AStar 
{
private:
	std::vector<Node*> openList;
	std::vector<Node*> closedList;
	
	Node* pathTracker    = nullptr;
	std::vector<Node*> fullPath;
	
	bool drawingPath	 = false;
	bool finishedDrawing = false;
	bool pathFound		 = false;
	
	int neighbourStep	 = 0;
	float astarCounter	 = 0.0f;
	float astarInterval	 = 0.01f;

	float modelCounter  = 0.0f;
	float modelInterval = 1.5f;

	int startX	= -1, startY = -1, 
		endX	= -1, endY	 = -1;

	Node* startNode;
	Node* endNode;


public:
	int width = 0, height = 0;
	std::vector<std::vector<Node>> grid;

public:
	AStar();
	AStar(std::string gridFilepath);
	AStar(std::string gridFilepath, int startX, int startY, int endX, int endY);

	void FindPathBySteps(float deltaTime);
	void FindPathFull();
	void Reset();

	std::vector<GridPoint> GetFullPath();

	inline Node* GetStartNode() const { return startNode; }
	inline Node* GetEndNode()   const { return endNode; }

	const inline bool IsFinished() const { return finishedDrawing; }


private:
	bool RunAStarStep();
	void TraverseBackToStartSteps();

	bool RunAStarFull();
	void TraverseBackToStartFull();
	
	void CheckNeighbour(Node* neighbour, Node* currentNode);
	void ResolveConflicts(Node& neighbour, Node& currentNode);

	Node* FindLowestCostNode();
	void UpdateLists(Node* lowestCostNode);
	
	bool ValidateStartEndCoordinates();
	void PrintCoordinates();

	void RemoveItemFromVector(std::vector<Node*>& vec, Node* node);
	bool IsNodeInVector(const std::vector<Node*>& vec, Node* node);


};
