<div align="center">
  <h1>Sunta Engine 3D</h1>
  <img src="https://github.com/user-attachments/assets/4e58ae55-28b3-4ddb-b76b-5c802ab82e38" width="600" alt="Sunta Engine Logo" />
  <p align="center">
    <a href="https://github.com/LightWeightBrah/Sunta-3D-Game-Engine/actions/workflows/build.yml"><img src="https://github.com/LightWeightBrah/Sunta-3D-Game-Engine/actions/workflows/build.yml/badge.svg" alt="CI Status" valign="middle"></a>
    <a href="https://github.com/LightWeightBrah/Sunta-3D-Game-Engine/releases"><img src="https://img.shields.io/github/v/tag/LightWeightBrah/Sunta-3D-Game-Engine?label=version&color=orange" alt="Version" valign="middle"></a>
    <a href="LICENSE"><img src="https://img.shields.io/github/license/LightWeightBrah/Sunta-3D-Game-Engine?color=blue" alt="License" valign="middle"></a>
  </p>
  
  <p><i>A custom 3D Graphics Engine focused on high-performance rendering and built-in editor tools.</i></p>
</div>

---

## Media Showcase
Overview of the technical features implemented in Sunta Engine, ranging from the material system to the integrated editor.

### Visuals and Materials
The engine uses a custom lighting system based on the Phong reflection model to handle surface highlights and light interaction.



<div align="center">
  <h3>Specular Mapping (Texture-based detail)</h3>
  <img src="https://github.com/user-attachments/assets/67078846-06eb-4321-a3b7-c42b775cbbd4" width="800" alt="Specular mapping on cube container box" />
  <p><i>Using Specular Maps to define surface shininess at a pixel level, allowing for realistic mixing of metallic and matte materials.</i></p>
</div>

<br />

<div align="center">
  <h3>Core Material Physics (No texture)</h3>
  <img src="https://github.com/user-attachments/assets/2a75c028-28f7-4d51-a0e4-3ad0ce6f80ae" width="800" alt="Phong lighting model on 3D Cube" />
  <p><i>Demonstrating the underlying light-reflection mathematics applied to geometry without textures.</i></p>
</div>

---

### Legacy Systems (Refactoring in Progress)
**Important Note:** The features below were fully functional in previous versions. The core architecture is currently being refactored, and these systems are being ported to the new structure.

<div align="center">
  <h3>Skeletal Animation</h3>
  <img src="https://github.com/user-attachments/assets/5192e57a-432e-4ba7-b6d8-d9f4288b2550" width="800" alt="Bone-based skeletal animation on 3D model" />
  <p><i>Showcasing the skeletal animation logic, supporting bone-based movements and smooth transitions.</i></p>
</div>

<br />

<div align="center">
  <h3>Grid-Based A* Pathfinding</h3>
  <img src="https://github.com/user-attachments/assets/ca79aaba-e06b-45f5-8fe7-74ab3636509a" width="800" alt="A star pathfinding visualization on 3D grid" />
  <p><i>A navigation system designed for grid-based environments, allowing AI agents to calculate efficient paths while avoiding obstacles.</i></p>
</div>

---

### Integrated Editor
The engine includes a built-in editor designed to simplify scene creation and real-time debugging.

<div align="center">
  <h3>Real-time Property Inspector</h3>
  <img src="https://github.com/user-attachments/assets/e82e96e1-2a2d-4d50-8d68-ff460cab9a51" width="800" alt="Property editor with real time changes" />
  <p><i>Modify object data like positions, colors, and logic toggles instantly. The interface is automatically generated from the underlying C++ components.</i></p>
</div>

<br />

<div align="center">
  <h3>Window Docking System</h3>
  <img src="https://github.com/user-attachments/assets/4e281319-6077-4421-9b91-dc0680cc18bc" width="800" alt ="Editor window docking system"/>
  <p><i>A professional workspace interface that allows windows to be dragged, snapped, and organized into custom layouts.</i></p>
</div>

<br />

#### Project Launcher, Settings and One-Click Game Build
* **Project Launcher:** `SuntaEditor` starts with a window that lists recent projects and lets you create a new project or open an existing one, before the editor opens. A checkbox can skip it and open the last project at startup. `Project > Switch Project` returns to it.
* **Project Settings:** game name, start scene, window size, fullscreen, vsync and game icon are edited in the editor (`Project > Project Settings`) and stored in the project's `game.config`.
* **Build Game:** packages the project into a folder for players with one click, no compiler needed. The **Development build** checkbox makes a game that shows a console window with its log, and the icon of the game is made from one PNG for the system you build on (`.exe` icon on Windows, `.icns` in an `.app` on macOS, the PNG itself on Linux).

---

## Engine Architecture

### System Abstraction and Logic
The engine is built on a modular architecture that separates low-level API calls from high-level logic:
* **Graphics Abstraction:** Clean wrappers for OpenGL objects including VAO, VBO, EBO, Textures, and Shaders.
* **Platform-Independent Window:** Designed to support multiple windowing APIs (currently using GLFW, with architecture ready for native Win32 or Linux).
* **Resource Manager:** Assets are loaded once and managed globally to ensure efficient memory usage.
* **Input Manager:** A centralized system for handling keyboard and mouse interaction across different engine modules.
* **Camera System:** A flexible 3D camera supporting smooth movement and rotation.
* **Event System:** A decoupled "Event Bus" that allows communication between systems without direct dependencies.

### Performance and Optimization
* **Data-Oriented ECS:** Separating data from logic to improve processing speed and memory layout efficiency.
* **Dirty Flag System:** Optimization that ensures heavy 3D math and matrix recalculations only occur when an object is modified.
* **Primitive Factory:** A specialized class for instant generation of standard 3D shapes like Cubes and Spheres.
* **Custom Logger:** A built-in diagnostic tool for real-time error tracking and performance monitoring.

### Gameplay Systems
* **Components:** Tag, Transform, Script, Mesh, Model, Animator, Directional / Point / Spot Light, Box Collider and Physics Body.
* **Lua Scripting:** gameplay is written in Lua (sol2) with hot-reload in the editor.
* **Physics:** collisions, triggers (enter, stay and exit events) and raycasts.
* **Scenes:** serialized to JSON. Resource paths are virtual (`@engine/...`, `@game/...`), so projects can be moved between computers.
* **Models:** OBJ, FBX, glTF and Collada through Assimp.

---

## Build System and CI
* **Build System:** CMake with presets (`CMakePresets.json`) for Windows, Linux and macOS.
* **Continuous Integration (CI):** automated build pipelines verify code stability on every commit.
* **Releases:** pushing a tag like `v0.1.0` publishes a GitHub Release with the engine packages for Windows (`.zip`), Linux and macOS (`.tar.gz`).
* **Engine package:** every workflow run uploads the ready-to-use engine for Windows, Linux and macOS (`Sunta-Engine-<system>-Release`): the editor, the game template and the sample project, no repository needed. See "Running the Editor".

---

## Tech Stack and Dependencies
Sunta Engine integrates several industry-standard libraries:

* [**GLFW**](https://www.glfw.org/) - Window and context management.
* [**GLAD**](https://glad.dav1d.de/) - OpenGL function loading.
* [**GLM**](https://github.com/g-truc/glm) - Linear algebra and mathematics.
* [**ImGui**](https://github.com/ocornut/imgui) - Editor interface and tooling (docking and viewports).
* [**ImGuizmo**](https://github.com/CedricGuillemet/ImGuizmo) - Transform gizmos in the editor.
* [**Assimp**](https://www.assimp.org/) - 3D model and animation loading.
* [**stb_image**](https://github.com/nothings/stb) - Texture and image parsing.
* [**Lua**](https://www.lua.org/) and [**sol2**](https://github.com/ThePhD/sol2) - Gameplay scripting.
* [**nlohmann/json**](https://github.com/nlohmann/json) - Scene and config serialization.
* [**nativefiledialog-extended**](https://github.com/btzy/nativefiledialog-extended) - Native file and folder dialogs.

---

## Learning Resources & Credits
The following resources were used to study and implement the core systems of Sunta Engine:

* [**TheCherno**](https://www.youtube.com/@TheCherno) – Learning modern C++ practices and conceptual engine architecture.
* [**LearnOpenGL**](https://learnopengl.com/) – Implementation of the graphics pipeline, lighting models, and Assimp model loading.
* [**OGLDev**](https://www.youtube.com/@OGLDEV) – Study of skeletal animation mathematics logic and advanced OpenGL techniques.
* [**Udemy (CMake Mastery)**](https://www.udemy.com/course/cmake-tests-and-tooling-for-cc-projects/) – Mastering CMake architecture, C++ tooling, and CI workflows.
* [**Official CMake Guide**](https://cmake.org/cmake/help/latest/guide/tutorial/index.html) – Used alongside the Udemy course to further study CMake documentation and best practices.

**Physics and collision detection**
* [**Two-Bit Coding**](https://www.youtube.com/playlist?list=PLSlpr6o9vURwq3oxVZSimY8iC-cdd3kIs) – Playlist I learned a lot about physics and collisions from.
* [**GamesWithGabe**](https://www.youtube.com/watch?v=Nm1Cgmbg5SQ) – Separating Axis Theorem EXPLAINED.
* [**tutorials123**](https://www.youtube.com/watch?v=Euuw72Ymu0M) – Sutherland-Hodgman Polygon Clipping Algorithm.
* [**DLBmaths**](https://www.youtube.com/watch?v=HC5YikQxwZA) – Shortest distance between two skew lines in 3D space (closest points of two segments, same naming as in the video).

---

## Project Structure
```text
Sunta/                    the engine (static library) and engine resources (Sunta/res)
  src/Sunta/
    Core/                 application, layers, virtual file system, resource loading
    Scene/                scenes
    ECS/                  entities and components
    Renderer/             rendering and OpenGL wrappers
    Physics/              collisions, triggers, raycasts
    Serialization/        saving and loading scenes (JSON)
    Scripting/            Lua scripting
    Events/               event bus
    Utilities/            helpers
    Editor/               editor code (separate library, SuntaEditorModule)
    Runtime/              layer that runs a game
    Platform/             Windows, Linux, Mac, GLFW, OpenGL
Apps/Editor/             SuntaEditor (the editor program)
Apps/Runtime/            SuntaRuntime (the game executable template)
Projects/MyGame/         sample game project
cmake/                   CMake helpers (program icons)
Dependencies/            third-party libraries
bin/  out/               build output (git-ignored)
```
CMake targets: `Sunta` (engine core), `SuntaEditorModule` (editor code), `SuntaEditor` and `SuntaRuntime`.

---

## Building the Engine

You choose only the system and the build type: **Release** (fast, use it for playing and testing) or **Debug** (slow, for debugging). Every block below is meant to be copied as it is.

**Requirements:** CMake 3.21+, Ninja and a C++17 compiler. Third-party libraries are in `Dependencies/`.

```bash
git clone https://github.com/LightWeightBrah/Sunta-3D-Game-Engine.git
cd Sunta-3D-Game-Engine
```

### Windows
Install Visual Studio 2022 with the "Desktop development with C++" workload. Then open **Developer PowerShell for VS 2022** (or "x64 Native Tools Command Prompt for VS 2022"), go to the repository folder and run:

Release:
```bat
cmake --preset x64-Release
cmake --build --preset x64-Release
bin\Release-Windows-AMD64\SuntaEditor.exe
```
Debug:
```bat
cmake --preset x64-Debug
cmake --build --preset x64-Debug
bin\Debug-Windows-AMD64\SuntaEditor.exe
```
In Visual Studio itself: `File > Open > Folder`, choose the repository folder, select the `x64-Debug` or `x64-Release` preset in the toolbar, select `SuntaEditor.exe` as the startup item and press F5.

### Linux
Packages for Ubuntu / Debian (use the equivalents on other distributions):
```bash
sudo apt install build-essential cmake ninja-build pkg-config \
    libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    libgtk-3-dev
```
`libgtk-3-dev` is needed for the native file and folder dialogs. If CMake reports missing Wayland or xkbcommon, also install `libwayland-dev libxkbcommon-dev wayland-protocols extra-cmake-modules`.

Release:
```bash
cmake --preset linux-release
cmake --build --preset linux-release
./bin/Release-Linux-*/SuntaEditor
```
Debug:
```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
./bin/Debug-Linux-*/SuntaEditor
```
> The Linux port is not well tested yet. If something fails to build, the cause is most likely a missing system package or a file in `Sunta/src/Sunta/Platform/Linux`.

### macOS
Install the compiler tools and the build tools once:
```bash
xcode-select --install
brew install cmake ninja
```
Release:
```bash
cmake --preset macos-release
cmake --build --preset macos-release
./bin/Release-Darwin-*/SuntaEditor
```
Debug:
```bash
cmake --preset macos-debug
cmake --build --preset macos-debug
./bin/Debug-Darwin-*/SuntaEditor
```
> The macOS port exists in the code but has never been compiled.

The results go to `bin/<Configuration>-<System>-<Processor>/` and contain `SuntaEditor` and `SuntaRuntime`.

---

## Running the Editor
Start `SuntaEditor`: it shows the project launcher, where you create a new project or open an existing one (a project can also be given on the command line: `SuntaEditor <project folder>`).
The launcher remembers projects in per-user settings (`%APPDATA%/SuntaEngine/editor_settings.json` on Windows, `~/.config/SuntaEngine/` on Linux, `~/Library/Application Support/SuntaEngine/` on macOS). Enabling "Open the last project at startup" skips the launcher, which is handy when starting from an IDE. `Project > Switch Project` returns to it.

**Engine package.** The workflow uploads the engine as an artifact (`Sunta-Engine-<system>-Release`, a `.tar.gz` on Linux and macOS, because a zip loses the executable flag). Unpack the whole folder and start `SuntaEditor`:
```
SuntaEngine/
  SuntaEditor, SuntaRuntime   the editor and the game template
  res/Sunta/                  engine resources
  Projects/MyGame/            sample project (the launcher's "Open Sample Project")
```
The editor uses `res/Sunta` and `Projects/MyGame` next to itself when they exist, an editor compiled from the repository reads them from the repository instead. Building a game works the same in both cases.

---

## Game Projects
A project is a folder with a `res` folder inside. A new project made in the launcher has the same folders as the engine:
```text
MyGame/
  res/
    game.config      game settings (JSON)
    Fonts/
    Materials/
    Models/
    Scenes/          scenes (.scene)
    Scripts/         Lua scripts
    Shaders/
    Textures/
    App/             game icon (window_icon.png), created when you choose an icon
  Build/             where the editor builds the game
```
`game.config`:
```json
{
	"name": "My Game",
	"start_scene": "@game/Scenes/Main.scene",
	"window_width": 1600,
	"window_height": 900,
	"window_icon": "@game/App/window_icon.png",
	"fullscreen": false,
	"vsync": true
}
```
All of these fields can be edited in the editor (`Project > Project Settings`). A sample project lives in `Projects/MyGame`.

---

## Building a Game
In the editor open `Project > Project Settings` and click **Save and Build Game**. `Project > Build Game` builds with the last choice of the **Development build** checkbox. The build runs in the background with a progress bar, so the editor stays usable, and when it fails the window shows the reason (for example a missing start scene or a game executable that was not compiled).
The editor needs no CMake and no compiler, it assembles a ready folder:
```text
<project>/Build/<GameName>_Release/
  <GameName>.exe        copy of SuntaRuntime with the name and icon of the game
  res/Sunta/            engine resources (without icons and images used only by the editor)
  res/Game/             project resources
```
Zip this folder and send it to players. A development build goes to `<GameName>_Development`. On macOS the game is packed as `<GameName>.app` (the executable and `res` are inside it, in `Contents/MacOS`), on Linux the executable has no extension.

**Release and development builds.** The game is always a copy of the Release `SuntaRuntime` program compiled by CMake, so the Release configuration must be compiled. A Release editor has it already, a Debug editor needs the Release preset built too (for example `cmake --preset x64-Release` and `cmake --build --preset x64-Release`, see "Building the Engine"), the editor finds it in the neighbouring `bin/` folder. The **Development build** checkbox does not change the program, it only puts `"developer_mode": true` into the copy of `game.config` inside the built game (never into the config of your project), and the game then opens a console window with its log and reloads a Lua script when its file is saved while the game runs. The game for players has no console. Debug asserts exist only in the Debug configuration of the engine, a built game never has them.

**Building from the command line.** The same build can run without any window, for scripts and CI:
```bash
SuntaEditor --build-game Projects/MyGame
SuntaEditor --build-game Projects/MyGame --development
```
The folder is the project (the one with `res` inside). The exit code is `0` when the game was built and `1` when not, the reason is printed. The result is in `<project>/Build/` like with the menu, but the folder is not opened. The GitHub workflow does exactly this for `MyGame` on Windows, Linux and macOS and uploads the finished game as an artifact (Linux and macOS as `.tar.gz`, because a zip loses the executable flag).

**Game log.** A game on Windows opens without a console window (unless it is a development build, see above). Everything it logs, including `print` from Lua scripts, goes to `<GameName>.log` next to the executable (the log of the previous run stays as `<GameName>.previous.log`), so ask players for this file when something goes wrong. If the folder of the game can't be written to (for example a game started straight from a zip file), the log goes to the temporary folder of the system instead, and the first lines of the log say where. Messages of `print` are blue in the console of a development game. When the game can't start at all (missing game files, a graphics card without OpenGL 3.3, no start scene) it shows a window with the reason instead of just not opening.

**Game icon.** `Project Settings > Icon > Choose icon (PNG)...` is all you do: you give one PNG (square, at least 256x256, ideally 1024x1024) and the editor makes the icon for the system it runs on:
* Windows: the icon is written into the `.exe` (sizes 16 to 256 px).
* macOS: an `.icns` file (sizes 16 to 1024 px) and `Info.plist` are made inside `<GameName>.app`.
* Linux: the PNG is used as it is, a `<GameName>.desktop` file next to the game points to it (an executable on Linux can't hold an icon).

The window of the game gets the icon at runtime from the same PNG on every system. Windows and macOS cache icons, so the old one may stay visible for a while. Build the game on the system you want to run it on: the editor packs the game for the system it runs on.

---

## Commit Convention
```text
SUN-11 Commit description
```

---

## Roadmap
* Camera component (the game currently uses a free WASD camera)
* Compiling and testing the macOS packaging (`.app` with icon)
* Compiling and testing on Linux and macOS, CI on all three systems
