<div align="center">
  <h1>Sunta Engine 3D</h1>
  <img src="Sunta/res/App/engine_logo.png" width="400" alt="Sunta Engine Logo" />
  <p align="center">
    <a href="https://github.com/LightWeightBrah/Sunta-3D-Game-Engine/actions/workflows/build.yml"><img src="https://github.com/LightWeightBrah/Sunta-3D-Game-Engine/actions/workflows/build.yml/badge.svg" alt="CI Status" valign="middle"></a>
    <a href="https://github.com/LightWeightBrah/Sunta-3D-Game-Engine/releases"><img src="https://img.shields.io/github/v/tag/LightWeightBrah/Sunta-3D-Game-Engine?label=version&color=orange" alt="Version" valign="middle"></a>
    <a href="LICENSE"><img src="https://img.shields.io/github/license/LightWeightBrah/Sunta-3D-Game-Engine?color=blue" alt="License" valign="middle"></a>
  </p>

  <p><i>A custom 3D game engine in C++ and OpenGL with a built-in editor, Lua scripting and one-click game builds.</i></p>
</div>

---

## Quick Start
1. Download the engine for your system from [Releases](https://github.com/LightWeightBrah/Sunta-3D-Game-Engine/releases) (a `.zip` for Windows, a `.tar.gz` for Linux and macOS) and unpack the whole folder.
2. Start `SuntaEditor`.
3. In the launcher click **Open Sample Project**, or create a project of your own.
4. Press the play button in the toolbar to run the scene, or choose `Project > Project Settings > Save and Build Game` to make a game you can send to players.

Do you want to compile the engine yourself? See [Building the Engine](#building-the-engine).

---

## Showcase

<div align="center">
  <h3>Editor</h3>
  <img src="docs/media/editor.gif" width="800" alt="The Sunta editor: hierarchy, inspector, gizmos and docking windows" />
  <p><i>A dockable workspace made with ImGui: a hierarchy of entities, an inspector generated from the C++ components, a file browser and transform gizmos for moving, rotating and scaling objects.</i></p>
</div>

<br />

<div align="center">
  <h3>Play Mode and Lua Scripting</h3>
  <img src="docs/media/play-mode.gif" width="800" alt="Play mode with a Lua script that reloads after a change" />
  <p><i>Press play to run the scene in the editor. Gameplay is written in Lua, and a script reloads as soon as you save it, so you can change the game while it runs.</i></p>
</div>

<br />

<div align="center">
  <h3>Lighting and Materials</h3>
  <img src="docs/media/lighting.gif" width="800" alt="Directional, point and spot lights and a specular map" />
  <p><i>Phong lighting with directional, point and spot lights. Specular maps decide how shiny each pixel of a surface is, so metal and matte parts can share one material.</i></p>
</div>

<br />

<div align="center">
  <h3>Physics</h3>
  <img src="docs/media/physics.gif" width="800" alt="Falling bodies, a trigger and the collider debug view" />
  <p><i>Collisions with collision layers, triggers that report enter, stay and exit, and raycasts. A debug view draws the colliders over the scene.</i></p>
</div>

<br />

<div align="center">
  <h3>Skeletal Animation</h3>
  <img src="docs/media/skeletal-animation.gif" width="800" alt="Bone-based skeletal animation on a 3D model" />
  <p><i>Bone-based animation of imported models with smooth transitions.</i></p>
</div>

<br />

<div align="center">
  <h3>Game Camera</h3>
  <img src="docs/media/camera.gif" width="800" alt="Creating a game camera from the editor view" />
  <p><i>Place a camera in the scene or create one from the current editor view. The editor shows what it sees, and Play mode uses it.</i></p>
</div>

<br />

<div align="center">
  <h3>Project Launcher and One-Click Build</h3>
  <img src="docs/media/project-build.gif" width="800" alt="Project launcher and building a game" />
  <p><i>The launcher lists your recent projects and the sample project. When the game is ready, one click packages it into a folder for players, no compiler needed.</i></p>
</div>

---

## Features

**Editor**
* Dockable windows: hierarchy, inspector, file browser, project settings.
* Transform gizmos, a free-fly editor camera and a game camera that can be created from the current view.
* Play mode with Lua hot-reload.
* Project launcher with recent projects and a sample project.
* Project settings: game name, start scene, window size, fullscreen, vsync and game icon.
* Tracking of unsaved changes with the usual shortcuts (`Ctrl+N`, `Ctrl+O`, `Ctrl+S`, `Ctrl+Shift+S`).
* **Build Game:** packages a project into a folder for players. The game icon is made from one PNG for the system you build on. The build also works without a window: `SuntaEditor --build-game <project folder>`.

**Engine**
* **Rendering:** wrappers for OpenGL objects (VAO, VBO, EBO, textures, shaders), Phong lighting with directional, point and spot lights, specular maps, materials and a debug renderer for colliders.
* **ECS:** a data-oriented entity component system. Components: Tag, Transform, Script, Mesh, Model, Animator, Directional / Point / Spot Light, Box Collider, Physics Body and Camera.
* **Physics:** collisions (Separating Axis Theorem), collision layers, triggers (enter, stay, exit) and raycasts.
* **Scripting:** gameplay in Lua ([sol2](https://github.com/ThePhD/sol2)) with access to transforms, physics bodies and input.
* **Assets:** OBJ, FBX, glTF and Collada models through Assimp. Scenes are JSON files, and resource paths are virtual (`@engine/...`, `@game/...`), so projects can be moved between computers.
* **Platforms:** Windows, Linux and macOS, built by CI on every commit.
* Also: a resource manager, an input manager, an event bus, dirty-flag transform updates and a logger that writes to a file.

---

## Tech Stack
* [**GLFW**](https://www.glfw.org/) - window and context management.
* [**GLAD**](https://glad.dav1d.de/) - OpenGL function loading.
* [**GLM**](https://github.com/g-truc/glm) - linear algebra and mathematics.
* [**ImGui**](https://github.com/ocornut/imgui) - editor interface (docking and viewports).
* [**ImGuizmo**](https://github.com/CedricGuillemet/ImGuizmo) - transform gizmos in the editor.
* [**Assimp**](https://www.assimp.org/) - 3D model and animation loading.
* [**stb_image**](https://github.com/nothings/stb) - texture and image loading.
* [**Lua**](https://www.lua.org/) and [**sol2**](https://github.com/ThePhD/sol2) - gameplay scripting.
* [**nlohmann/json**](https://github.com/nlohmann/json) - scene and config files.
* [**nativefiledialog-extended**](https://github.com/btzy/nativefiledialog-extended) - native file and folder dialogs.

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
Install the packages (the example is for Ubuntu / Debian, use the equivalents on other distributions):
```bash
sudo apt install build-essential cmake ninja-build pkg-config \
    libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    libgtk-3-dev libwayland-dev libxkbcommon-dev wayland-protocols
```
`libgtk-3-dev` is needed for the native file and folder dialogs.

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

The results go to `bin/<Configuration>-<System>-<Processor>/` and contain `SuntaEditor` and `SuntaRuntime`.

> Linux and macOS are built by CI on every commit, but they have not been tested much at runtime yet.

---

## Projects and Game Builds

### Projects
A project is a folder with a `res` folder inside. Create one in the launcher or open an existing one (the sample project is `Projects/MyGame`):
```text
MyGame/
  res/
    game.config      game settings (edited in Project Settings)
    Scenes/          scenes (.scene)
    Scripts/         Lua scripts
    Materials/  Models/  Textures/  Shaders/  Fonts/
    App/             game icon (window_icon.png)
  Build/             where the built game goes
```
`game.config` holds the name of the game, the start scene, the window size, fullscreen, vsync and the icon.

### Building a Game
Open `Project > Project Settings` and click **Save and Build Game**. The editor needs no CMake and no compiler, it assembles a folder that you can zip and send to players:
```text
Build/<GameName>_Release/
  <GameName>.exe    the game (a <GameName>.app on macOS, no extension on Linux)
  res/              engine and project resources
```
* **Development build:** a checkbox in Project Settings. The game opens a console window with its log and reloads Lua scripts when you save them. A game for players has no console, it writes its log to `<GameName>.log` next to it.
* **Game icon:** choose one PNG in Project Settings and the editor makes the icon for the system it runs on (an `.exe` icon on Windows, an `.icns` inside the `.app` on macOS, the PNG itself and a `.desktop` file on Linux). Build the game on the system you want to run it on.
* **From the command line:** the same build runs without a window, for scripts and CI:
  ```bash
  SuntaEditor --build-game Projects/MyGame
  SuntaEditor --build-game Projects/MyGame --development
  ```
* The game is a copy of the Release `SuntaRuntime` program, so the Release configuration has to be compiled. A Release editor already has it.

---

## Project Structure
```text
Sunta/                    the engine (static library) and engine resources (Sunta/res)
  src/Sunta/
    Core/                 application, layers, virtual file system, resource loading
    Scene/                scenes and cameras
    ECS/                  entities and components
    Renderer/             rendering and OpenGL wrappers
    Physics/              collisions, triggers, raycasts
    Animation/            skeletal animation
    Scripting/            Lua scripting
    Serialization/        saving and loading scenes (JSON)
    Events/               event bus
    Utilities/            helpers
    Editor/               editor code (separate library, SuntaEditorModule)
    Runtime/              layer that runs a game
    Platform/             Windows, Linux, Mac, GLFW, OpenGL
Apps/Editor/              SuntaEditor, the editor program
Apps/Runtime/             SuntaRuntime, the game template
Projects/MyGame/          sample game project
cmake/                    CMake helpers (program icons)
Dependencies/             third-party libraries
docs/media/               images and GIFs of this README
```
CMake targets: `Sunta` (engine core), `SuntaEditorModule` (editor code), `SuntaEditor` and `SuntaRuntime`.

---

## Roadmap
* Testing on real Linux and macOS machines
* Format versions for scenes and project files
* Smaller games: no ImGui in the game executable
* A project template with a ready player

---

## Learning Resources & Credits
The following resources were used to study and implement the core systems of Sunta Engine:

* [**TheCherno**](https://www.youtube.com/@TheCherno) - modern C++ practices and engine architecture.
* [**LearnOpenGL**](https://learnopengl.com/) - the graphics pipeline, lighting models and Assimp model loading.
* [**OGLDev**](https://www.youtube.com/@OGLDEV) - skeletal animation mathematics and advanced OpenGL techniques.
* [**Udemy (CMake Mastery)**](https://www.udemy.com/course/cmake-tests-and-tooling-for-cc-projects/) - CMake architecture, C++ tooling and CI workflows.
* [**Official CMake Guide**](https://cmake.org/cmake/help/latest/guide/tutorial/index.html) - CMake documentation and best practices.

**Physics and collision detection**
* [**Two-Bit Coding**](https://www.youtube.com/playlist?list=PLSlpr6o9vURwq3oxVZSimY8iC-cdd3kIs) - a playlist about physics and collisions.
* [**GamesWithGabe**](https://www.youtube.com/watch?v=Nm1Cgmbg5SQ) - Separating Axis Theorem explained.
* [**tutorials123**](https://www.youtube.com/watch?v=Euuw72Ymu0M) - Sutherland-Hodgman polygon clipping.
* [**DLBmaths**](https://www.youtube.com/watch?v=HC5YikQxwZA) - shortest distance between two skew lines in 3D space (closest points of two segments).
