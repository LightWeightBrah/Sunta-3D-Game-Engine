#include <iostream>
#include <cmath>
#include "Node.h"

Node::Node()
{
	this->x = -1;
	this->y = -1;
	this->cellType = CELL::ERROR;
	this->fCost = 9999;
}

Node::Node(int x, int y, CELL cellType)
{
	this->x = x;
	this->y = y;
	this->cellType = cellType;
	this->gCost = 0; 
	this->fCost = 9999;
}

void Node::PrintPosition()
{
	std::cout << "x is: " << x << " y is: " << y << " cellType = " << cellType << std::endl;
}

void Node::Reset()
{
	parent = nullptr;
	gCost = 0;
	fCost = 9999;
	hCost = 9999;

	if (cellType != CELL::WALL)
		cellType = CELL::NO_WALL;
}

double Node::CalculateHCost(int startX, int startY, int endX, int endY)
{
	return sqrt(pow((startX - endX), 2) + pow((startY - endY), 2));
}

void Node::IncreaseG()
{
	this->gCost++;
}

void Node::SetParent(Node* parent)
{
	this->parent = parent;
}


bool Node::operator == (const Node& other) const
{
	return x == other.x && y == other.y;
}

bool Node::operator != (const Node& other) const
{
	return !(*this == other);
}

bool Node::operator == (const int& other)
{
	return x == -1 || y == -1 || cellType == CELL::ERROR;
}
bool Node::operator != (const int& other)
{
	return !(*this == other);
}