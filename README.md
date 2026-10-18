<div align="center">
  <h1>Sunta Engine 3D</h1>
  <img src="https://github.com/user-attachments/assets/4e58ae55-28b3-4ddb-b76b-5c802ab82e38" width="600" alt="Sunta Engine Logo" />
  <p align="center">
    <a href="https://github.com/LightWeightBrah/Sunta-3D-Game-Engine/actions/workflows/build.yml"><img src="https://github.com/LightWeightBrah/Sunta-3D-Game-Engine/actions/workflows/build.yml/badge.svg" alt="CI Status" valign="middle"></a>
    <a href="https://github.com/LightWeightBrah/Sunta-3D-Game-Engine/releases"><img src="https://img.shields.io/github/v/tag/LightWeightBrah/Sunta-3D-Game-Engine?label=version&color=orange" alt="Version" valign="middle"></a>
    <a href="LICENSE"><img src="https://img.shields.io/github/license/LightWeightBrah/Sunta-3D-Game-Engine?color=blue" alt="License" valign="middle"></a>
  </p>

  <p><i>A custom 3D game engine in C++ and OpenGL with a built-in editor, Lua scripting and one-click game builds.</i></p>
</div>

---

## Quick Start
1. Download the engine for your system from [Releases](https://github.com/LightWeightBrah/Sunta-3D-Game-Engine/releases) (Windows `.zip`, Linux and macOS `.tar.gz`) and unpack the whole folder.
2. Start `SuntaEditor`.
3. In the launcher click **Open Sample Project**, or create your own project.
4. Press play in the toolbar, or build a game for players with **Project > Project Settings > Save and Build Game**.

Want to compile the engine yourself? See [Building the Engine](#building-the-engine).

---

## Showcase

<!-- GIF TODO: the editor at work: hierarchy, inspector, moving an object with the gizmo, docking windows -->
<!-- GIF TODO: Play mode: press play, the Lua script moves the player, change the script and see it reload -->
<!-- GIF TODO: lighting: directional, point and spot lights, a specular map on a textured cube -->
<!-- GIF TODO: physics: stacked bodies falling, a trigger firing, the collider debug view -->
<!-- GIF TODO: camera component: "Create Camera From View" and the camera view in Play -->
<!-- GIF TODO: project launcher and Save and Build Game, then starting the built game -->

<div align="center">
  <h3>Skeletal Animation</h3>
  <img src="https://github.com/user-attachments/assets/5192e57a-432e-4ba7-b6d8-d9f4288b2550" width="800" alt="Bone-based skeletal animation on 3D model" />
  <p><i>Bone-based animation of imported models with smooth transitions.</i></p>
</div>

---

## Features

**Editor**
* Docking workspace with a hierarchy, an inspector generated from the C++ components, a file browser and transform gizmos.
* Play mode with Lua hot-reload, and a game camera that can be created from the current view.
* Project launcher with recent projects, a sample project and project settings (name, start scene, window size, fullscreen, vsync, icon).
* Unsaved changes tracking with the usual shortcuts (`Ctrl+N`, `Ctrl+O`, `Ctrl+S`, `Ctrl+Shift+S`).
* **Build Game:** packages a project into a folder for players, no compiler needed. The game icon is made from one PNG for the system you build on. Also available without a window: `SuntaEditor --build-game <project>`.

**Engine**
* **Rendering:** OpenGL wrappers (VAO, VBO, EBO, textures, shaders), Phong lighting with directional, point and spot lights, specular maps, materials and a debug renderer for colliders.
* **ECS:** data-oriented entities and components: Tag, Transform, Script, Mesh, Model, Animator, Directional / Point / Spot Light, Box Collider, Physics Body and Camera.
* **Physics:** SAT collisions, collision layers, triggers (enter, stay, exit) and raycasts.
* **Scripting:** gameplay in Lua ([sol2](https://github.com/ThePhD/sol2)) with access to transforms, physics bodies and input.
* **Assets:** OBJ, FBX, glTF and Collada models through Assimp. Scenes are JSON, and resource paths are virtual (`@engine/...`, `@game/...`), so projects can be moved between computers.
* **Platforms:** Windows, Linux and macOS, built by CI on every commit.
* Resource manager, input manager, event bus, dirty-flag transform updates and a logger that writes to a file.

---

## Tech Stack
* [**GLFW**](https://www.glfw.org/) - window and context.
* [**GLAD**](https://glad.dav1d.de/) - OpenGL loading.
* [**GLM**](https://github.com/g-truc/glm) - math.
* [**ImGui**](https://github.com/ocornut/imgui) and [**ImGuizmo**](https://github.com/CedricGuillemet/ImGuizmo) - editor interface and gizmos.
* [**Assimp**](https://www.assimp.org/) - model and animation loading.
* [**stb_image**](https://github.com/nothings/stb) - image loading.
* [**Lua**](https://www.lua.org/) and [**sol2**](https://github.com/ThePhD/sol2) - scripting.
* [**nlohmann/json**](https://github.com/nlohmann/json) - scene and config files.
* [**nativefiledialog-extended**](https://github.com/btzy/nativefiledialog-extended) - native file dialogs.

---

## Building the Engine
**Requirements:** CMake 3.21+, Ninja and a C++17 compiler. Third-party libraries are in `Dependencies/`.

```bash
git clone https://github.com/LightWeightBrah/Sunta-3D-Game-Engine.git
cd Sunta-3D-Game-Engine
```

### Windows
Install Visual Studio 2022 with the "Desktop development with C++" workload, open **Developer PowerShell for VS 2022** in the repository folder and run:
```bat
cmake --preset x64-Release
cmake --build --preset x64-Release
bin\Release-Windows-AMD64\SuntaEditor.exe
```
In Visual Studio you can also use `File > Open > Folder`, pick the `x64-Debug` or `x64-Release` preset and start `SuntaEditor.exe`.

### Linux
```bash
sudo apt install build-essential cmake ninja-build pkg-config \
    libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    libgtk-3-dev libwayland-dev libxkbcommon-dev wayland-protocols

cmake --preset linux-release
cmake --build --preset linux-release
./bin/Release-Linux-*/SuntaEditor
```

### macOS
```bash
xcode-select --install
brew install cmake ninja

cmake --preset macos-release
cmake --build --preset macos-release
./bin/Release-Darwin-*/SuntaEditor
```

For a Debug build use the `x64-Debug`, `linux-debug` or `macos-debug` preset (Debug is much slower, test performance in Release). The output is in `bin/<Configuration>-<System>-<Processor>/`.

> Linux and macOS are built by CI, but they have not been tested much at runtime yet.

---

## Projects and Game Builds
A project is a folder with a `res` folder inside (the sample one is `Projects/MyGame`). Create one in the launcher or open an existing one, and the editor does the rest:
```text
MyGame/
  res/
    game.config     name, start scene, window size, fullscreen, vsync, icon (edited in Project Settings)
    Scenes/  Scripts/  Materials/  Models/  Textures/  Shaders/  Fonts/
  Build/            the built game goes here
```

To build a game open `Project > Project Settings` and click **Save and Build Game**. The result is a folder you can zip and send to players:
```text
Build/<GameName>_Release/
  <GameName>.exe    the game (on macOS a <GameName>.app, on Linux no extension)
  res/              engine and project resources
```
The **Development build** checkbox makes a game with a console window that reloads Lua scripts when you save them. A game for players has no console and writes its log to `<GameName>.log` next to it.

The build can also run without a window, for scripts and CI:
```bash
SuntaEditor --build-game Projects/MyGame
```
The game is a copy of the Release `SuntaRuntime` program, so the Release configuration has to be compiled (a Release editor already has it).

---

## Project Structure
```text
Sunta/                  the engine (static library) and engine resources (Sunta/res)
  src/Sunta/
    Core/  Scene/  ECS/  Renderer/  Physics/  Animation/  Scripting/
    Serialization/  Events/  Utilities/  Platform/
    Editor/             editor code (SuntaEditorModule)
    Runtime/            layer that runs a game
Apps/Editor/            SuntaEditor, the editor program
Apps/Runtime/           SuntaRuntime, the game template
Projects/MyGame/        sample project
Dependencies/           third-party libraries
```
CMake targets: `Sunta`, `SuntaEditorModule`, `SuntaEditor` and `SuntaRuntime`.

---

## Roadmap
* Testing on real Linux and macOS machines
* Format versions for scenes and project files
* Smaller games: no ImGui in the game executable
* Project template with a ready player

---

## Learning Resources & Credits
* [**TheCherno**](https://www.youtube.com/@TheCherno) - modern C++ and engine architecture.
* [**LearnOpenGL**](https://learnopengl.com/) - graphics pipeline, lighting and Assimp model loading.
* [**OGLDev**](https://www.youtube.com/@OGLDEV) - skeletal animation and advanced OpenGL.
* [**Udemy (CMake Mastery)**](https://www.udemy.com/course/cmake-tests-and-tooling-for-cc-projects/) and the [**official CMake guide**](https://cmake.org/cmake/help/latest/guide/tutorial/index.html) - CMake, tooling and CI.

**Physics and collision detection**
* [**Two-Bit Coding**](https://www.youtube.com/playlist?list=PLSlpr6o9vURwq3oxVZSimY8iC-cdd3kIs) - physics and collisions playlist.
* [**GamesWithGabe**](https://www.youtube.com/watch?v=Nm1Cgmbg5SQ) - Separating Axis Theorem.
* [**tutorials123**](https://www.youtube.com/watch?v=Euuw72Ymu0M) - Sutherland-Hodgman polygon clipping.
* [**DLBmaths**](https://www.youtube.com/watch?v=HC5YikQxwZA) - shortest distance between two skew lines in 3D space.
