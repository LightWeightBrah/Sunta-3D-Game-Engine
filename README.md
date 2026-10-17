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
* **Project Settings:** game name, start scene, window size and game icon are edited in the editor (`Project > Project Settings`) and stored in the project's `game.config`.
* **Build Game:** packages the project into a folder for players with one click, no compiler needed. The configuration (Debug or Release) is chosen from a dropdown, and the icon of the game is written into the `.exe` on Windows.

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

---

## Project Structure
```text
Sunta/                    the engine (static library) and engine resources (Sunta/res)
  src/Sunta/
    Core/ Scene/ ECS/ Renderer/ Physics/ Serialization/ Scripting/ Events/ Utilities/
    Editor/               editor code (separate library, SuntaEditorModule)
    Runtime/              layer that runs a game
    Platform/             Windows, Linux, Mac, GLFW, OpenGL
Apps/Editor/              SuntaEditor (the editor program)
Apps/Runtime/             SuntaRuntime (the game executable template)
Projects/MyGame/          sample game project
cmake/                    CMake helpers (program icons)
Dependencies/             third-party libraries
bin/  out/                build output (git-ignored)
```
CMake targets: `Sunta` (engine core), `SuntaEditorModule` (editor code), `SuntaEditor` and `SuntaRuntime`.

---

## Building the Engine

**Requirements:** CMake 3.21+, a C++17 compiler (Visual Studio 2022 on Windows, GCC or Clang on Linux and macOS) and Ninja. Third-party libraries are in `Dependencies/`.

```bash
git clone https://github.com/LightWeightBrah/Sunta-3D-Game-Engine.git
cd Sunta-3D-Game-Engine
```

`CMakePresets.json` defines these presets:

| Preset | System | Configuration |
|---|---|---|
| `x64-Debug`, `x64-Release` | Windows | Debug, Release |
| `linux-debug`, `linux-release` | Linux | Debug, Release |
| `macos-debug`, `macos-release` | macOS | Debug, Release |

Output goes to `bin/<Configuration>-<System>-<Processor>/`, for example `bin/Release-Windows-AMD64/` or `bin/Release-Linux-x86_64/`. It contains `SuntaEditor` and `SuntaRuntime`.
Debug is much slower (debug heap, no optimizations), so judge performance in Release.

### Windows: Visual Studio 2022
1. `File > Open > Folder` and choose the repository root. Visual Studio detects `CMakeLists.txt` and `CMakePresets.json`.
2. Select the `x64-Debug` or `x64-Release` preset in the toolbar and wait for CMake to configure.
3. Select `SuntaEditor.exe` as the startup item and press F5.

### Windows: command line
Open "x64 Native Tools Command Prompt for VS 2022" (the `cl` compiler needs its environment):
```bat
cmake --preset x64-Release
cmake --build --preset x64-Release
bin\Release-Windows-AMD64\SuntaEditor.exe
```

### Linux
Packages for Ubuntu / Debian (use the equivalents on other distributions):
```bash
sudo apt install build-essential cmake ninja-build pkg-config \
    libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    libgtk-3-dev
```
`libgtk-3-dev` is needed for the native file and folder dialogs. If CMake reports missing Wayland or xkbcommon, also install `libwayland-dev libxkbcommon-dev wayland-protocols extra-cmake-modules`.

```bash
cmake --preset linux-release
cmake --build --preset linux-release
./bin/Release-Linux-x86_64/SuntaEditor
```
For Debug use `linux-debug` and the `Debug-Linux-x86_64` folder. Without Ninja:
`cmake -S . -B out/build/linux-release -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release && cmake --build out/build/linux-release -j`

> The Linux port is not well tested yet. If something fails to build, the cause is most likely a missing system package or a file in `Sunta/src/Sunta/Platform/Linux`.

### macOS
Use the same commands with the `macos-debug` and `macos-release` presets. The macOS port exists in the code but has never been compiled.

### CMake options
| Option | Default | Meaning |
|---|---|---|
| `SUNTA_PROJECT` | `Projects/MyGame` | Project that `SuntaRuntime` opens when started straight from the IDE, and the sample project of the launcher |
| `SUNTA_DEV_MODE` | `ON` | A game started from the IDE (no `res` folder next to it) reads assets from the source folders |

Example: `cmake --preset linux-release -DSUNTA_PROJECT=/home/me/MyGame`

---

## Running the Editor
```text
SuntaEditor                      shows the project launcher (or opens the last project, if you enabled that)
SuntaEditor <project folder>     opens the project at once
SuntaEditor --launcher           always shows the project launcher
```
The launcher remembers projects in per-user settings (`%APPDATA%/SuntaEngine/editor_settings.json` on Windows, `~/.config/SuntaEngine/` on Linux). Enabling "Open the last project at startup" skips the launcher, which is handy when starting from an IDE.

---

## Game Projects
A project is a folder with a `res` folder inside:
```text
MyGame/
  res/
    game.config      game settings (JSON)
    Scenes/          scenes (.scene)
    Scripts/         Lua scripts
    App/             game icon (window_icon.png)
    ...              models, textures, materials
  Build/             where the editor builds the game
```
`game.config`:
```json
{
	"name": "My Game",
	"start_scene": "@game/Scenes/Main.scene",
	"window_width": 1600,
	"window_height": 900,
	"window_icon": "@game/App/window_icon.png"
}
```
All of these fields can be edited in the editor (`Project > Project Settings`). New projects are created in the launcher (`New Project...`). A sample project lives in `Projects/MyGame`.

---

## Building a Game
In the editor open `Project > Project Settings`, choose the **Configuration** (Debug or Release) and click **Save and Build Game**. `Project > Build Game` builds with the last chosen configuration.
The editor needs no CMake and no compiler, it assembles a ready folder:
```text
<project>/Build/<GameName>/
  <GameName>.exe        copy of SuntaRuntime with the name and icon of the game
  res/Sunta/            engine resources (without icons and images used only by the editor)
  res/Game/             project resources
```
Zip this folder and send it to players. A Debug build goes to `<GameName>_Debug`, so it is not mistaken for the version to share.

**Debug and Release.** The game is a copy of the `SuntaRuntime` program compiled by CMake, so the editor can only build with a configuration that was compiled. The configuration of the editor itself is always available. Compile the other one with:
```bash
cmake --preset x64-Release                              # or linux-release
cmake --build --preset x64-Release --target SuntaRuntime
```
(and the same with `x64-Debug`). The editor finds it in the neighbouring `bin/` folder. A Debug game needs the compiler's debug runtime on the player's computer, so use Release to share a game.

**Game icon.** `Project Settings > Icon > Choose icon (PNG)...` copies the image to `res/App/window_icon.png`. The game window gets the icon at runtime. When building on Windows, the editor also writes the icon into the `.exe` (sizes 16 to 256 px from one image, ideally square and at least 256x256). Windows caches icons, so the old one may stay visible for a while. On Linux a `.desktop` file with the icon is created next to the game.

---

## Commit Convention
```text
SUN-11 Commit description
```

---

## Roadmap
* Camera component (the game currently uses a free WASD camera)
* Fullscreen, vsync and more window settings in `game.config`
* Executable icon on Linux and macOS, macOS packaging
* Compiling and testing on Linux and macOS, CI on all three systems
