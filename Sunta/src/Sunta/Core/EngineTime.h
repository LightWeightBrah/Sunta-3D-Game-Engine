#pragma once

namespace Sunta
{
	class EngineTime
	{
	public:
		static float deltaTime;
		static void	 Update();
	
	private:
		static float lastFrame;
	};
}