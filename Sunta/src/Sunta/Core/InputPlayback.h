#pragma once
#include <functional>
#include <string>

namespace Sunta
{
	// Plays keyboard and mouse input from a text file, so automated tests can really play a game
	// (attack, block, dodge, lock on) and save frames at exact moments. Off unless SUNTA_INPUT_SCRIPT is set.
	//
	// One command per line: "<seconds> <command> <arguments>", '#' starts a comment. Times count from the first frame.
	//   down KEY / up KEY            press / release (KEY: W, Space, Shift, Ctrl, Enter, Esc, Tab, F1..F12, 0..9, Mouse1/Mouse2)
	//   tap KEY                      press now, release next frame
	//   hold KEY SECONDS             press now, release later
	//   look DX DY [SECONDS]         move the mouse by DX, DY pixels (spread over SECONDS)
	//   every STEP UNTIL COMMAND...  repeat COMMAND every STEP seconds until the time UNTIL
	//   shot NAME                    save the frame as NAME.png in SUNTA_TEST_OUT (default: the working folder)
	//   log TEXT                     write TEXT to the log (marks a moment in the combat log)
	//   quit                         close the game
	// Other commands go to the handler set with SetCommandHandler (the gameplay test bot: lockon, approach, fight)
	class InputPlayback
	{
	public:
		static void Init();
		static bool IsActive();

		// Before the frame's input is processed: sends what is due
		static void Update(float secondsSinceStart);

		// After the frame is drawn (before swapping): saves requested frames
		static void AfterRender(unsigned int width, unsigned int height);

		static bool WantsQuit();

		// Commands this class doesn't know (command, three arguments, time). Returns false for an unknown one
		using CommandHandler = std::function<bool(const std::string&, const std::string&, const std::string&, const std::string&, float)>;
		static void SetCommandHandler(CommandHandler handler);

		// For the test bot: press / release a key by its script name (W, Shift, Mouse1 ...)
		static void PressKey(const std::string& name);
		static void ReleaseKey(const std::string& name);
		static bool IsKeyHeldByScript(const std::string& name);

		// Folder for test output (frames, combat log)
		static const std::string& GetOutputFolder();
	};
}
