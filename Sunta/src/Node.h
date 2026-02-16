#pragma once

enum CELL 
{
	WALL	=  5,
	NO_WALL =  0,
	ROUTE	=  1,
	CLOSED	=  2,
	OPEN	=  4,
	ERROR	= -1
};

struct Node
{
	Node* parent;
	CELL cellType;

	int x, y;
	double fCost, gCost, hCost;

	Node();
	Node(int x, int y, CELL cellType);


	void PrintPosition();
	void Reset();
	double CalculateHCost(int startX, int startY, int endX, int endY);
	void IncreaseG();
	void SetParent(Node* parent);

	bool operator == (const Node& other) const;
	bool operator != (const Node& other) const;

	bool operator == (const int& other);
	bool operator != (const int& other);
};