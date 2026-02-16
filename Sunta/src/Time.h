#pragma once

class Time
{
public:
	static float deltaTime;
	static void	 Update();

private:
	static float lastFrame;
};