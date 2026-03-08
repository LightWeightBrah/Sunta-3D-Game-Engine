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

---

## Project Management
I manage the development process using industry-standard tools to maintain a clear roadmap and code quality.

* **Jira Roadmap:** Every feature and refactor is tracked as a technical task on a Kanban Board.
* **Continuous Integration (CI):** Automated build pipelines verify code stability on every commit.
* **Build System:** Managed via CMake for consistent cross-platform configuration.

<div align="center">
  <a href="https://julianlabanowski.atlassian.net/jira/software/c/projects/SUN/issues/">
    <img src="https://github.com/user-attachments/assets/9ac6ac7e-f0c1-4642-b00f-6ad443dc1172" width="1200" alt="Jira Kanban board" />
  </a>
  <br />
  <em><b>Interactive Roadmap:</b> Click the image to view real-time project progress and technical tasks on Jira.</em>
</div>

---

## Tech Stack and Dependencies
Sunta Engine integrates several industry-standard libraries:

* [**GLFW**](https://www.glfw.org/) - Window and context management.
* [**GLAD**](https://glad.dav1d.de/) - OpenGL function loading.
* [**GLM**](https://github.com/g-truc/glm) - Linear algebra and mathematics.
* [**ImGui**](https://github.com/ocornut/imgui) - Editor interface and tooling.
* [**Assimp**](https://www.assimp.org/) - 3D model and animation loading.
* [**stb_image**](https://github.com/nothings/stb) - Texture and image parsing.

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
The repository is split into two distinct projects:
1. **Sunta:** The core engine containing all rendering and systems logic.
2. **Game:** A separate application built upon the Sunta core, demonstrating its use as a standalone product.

---

## Build Instructions (Visual Studio 2022)
```bash
# 1. Clone the repository
git clone https://github.com/LightWeightBrah/Sunta-3D-Game-Engine.git

# 2. Open Project
Launch Visual Studio 2022 and select Open a local folder

# 3. Select Directory
Choose the root Sunta-3D-Game-Engine folder. Visual Studio will automatically detect CMakeLists.txt and begin the configuration process

# 4. Set Startup Item
Once configuration is complete, locate the Select Startup Item dropdown in the top toolbar (next to the green play button) and select Game.exe

# 5. Build and Run
Press F5 or click the Start button to compile and launch the application
```
