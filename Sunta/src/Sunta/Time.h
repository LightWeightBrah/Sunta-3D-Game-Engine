#pragma once

namespace Sunta
{
	class Time
	{
	public:
		static float deltaTime;
		static void	 Update();
	
	private:
		static float lastFrame;
	};
}