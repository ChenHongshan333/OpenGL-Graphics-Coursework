# OpenGL Graphics Coursework (VS2022 + FreeGLUT)

A Visual Studio 2022 (C++) collection of OpenGL/FreeGLUT assignments for CS3241, organized as a single portfolio solution.

**Project Link**: <https://chenhongshan333.github.io/OpenGL-Graphics-Coursework/>

## Table of Contents
- [Background](#background)
- [What’s Included](#whats-included)
- [Install](#install)
- [Usage](#usage)
- [Controls](#controls)
- [Repository Layout](#repository-layout)
- [Notes on Assets (Textures / Input TXT)](#notes-on-assets-textures--input-txt)
- [Contributing](#contributing)
- [License](#license)

## Background
This repository bundles multiple OpenGL assignments into one portfolio-friendly codebase. Each assignment is an independent Visual Studio project, and they can be built/run from the main solution.

## What’s Included
- **Assignment 1 — Transformations (2D)**

- Translation, rotation, and scaling via transformation matrices
- Create a cute 2D cat using basic OpenGL primitives

- **Assignment 2 — Animation (2D)**
- Simulate a 2D solar system with planets and comets
- Optional “time mode”: planets are arranged to display the real-world time
=======
  - Translation, rotation, and scaling via transformation matrices
  - Create a cute 2D cat using basic OpenGL primitives

- **Assignment 2 — Animation (2D)**
  - Simulate a 2D solar system with planets and comets
  - Optional “time mode”: planets are arranged to display the real-world time

- **Assignment 3 — Lighting & Shading**
  - Apply different lighting and shading models on multiple subjects

- **Assignment 4 — Bezier Curves**
  - Draw Bezier curves interactively
  - Or generate more complex Bezier graphs from an input text file

- **Assignment 5 — Ray Tracing**
  - Ray tracing on different subjects
  - Includes shadow effects and texture mapping

## Install
### Requirements
- Windows
- Visual Studio 2022
- Platform: **x64**

### Dependency (FreeGLUT)
This repo uses a shared FreeGLUT setup stored in `deps/` and referenced via a shared property sheet:
- `deps/freeglut/` (include/lib/dll)
- `props/freeglut.props`

> Tip: If you cloned the repo and the projects build but fail to run due to missing `freeglut.dll`,
> ensure the property sheet is applied and that the post-build step copies the DLL into `$(OutDir)`.

### Build
1. Clone the repository
2. Open the main solution (e.g., `OpenGL-Coursework.sln`)
3. Select configuration: `Debug` / `Release`
4. Select platform: **x64**
5. Build the solution

## Usage
1. Open the main solution: `OpenGL-Coursework.sln`
2. In Solution Explorer, right-click an assignment project
3. Choose **Set as Startup Project**
4. Run with **Local Windows Debugger**

## Controls
Controls vary by assignment. Please open each project and refer to the list below.

- **Assignment 2 — Animation (2D)**
  - Toggle Time Mode: T
  - Scale: +/-
  - Translate: W/A/S/D
  - Exit: ESC/q

- **Assignment 3 — Lighting & Shading**
  - 1-2: Draw different objects
  - S: Toggle Smooth Shading
  - H: Toggle Highlight
  - W: Draw Wireframe
  - P: Draw Polygon (filled)
  - V: Draw Vertices
  - Q: Quit
  - Left mouse click and drag: rotate the object
  - Right mouse click and drag: zooming

- **Assignment 4 — Bezier Curves**
  - P: Toggle displaying control points
  - L: Toggle displaying control lines
  - E: Erase all points (Clear)
  - C: Toggle C1 continuity (within a segment)
  - T: Toggle displaying tangent vectors
  - O: Toggle displaying objects (Cute Cat)
  - Z: Undo last operation
  - Q: Quit
  - Left mouse click: Add a control point

- **Assignment 5 — Ray Tracing**
  - S : go to next scene
  - Q : quit
  - A/B/C/D : toggle textures (Second scene only)


## Repository Layout
- `OpenGL-Coursework.sln` — main portfolio solution to run all assignments
- `projects/` — assignment projects (each has its own `.vcxproj`, source code, and assets)
- `deps/freeglut/` — shared FreeGLUT dependency (include/lib/dll)
- `props/freeglut.props` — shared build configuration (include/lib + linker deps + DLL copy)

## Notes on Assets (Textures / Input TXT)
Some assignments load assets (e.g., textures and `.txt` inputs) using relative paths.
Recommended setup:
- Keep textures / input files inside the corresponding assignment folder under `projects/...`
- Ensure the project working directory is set to `$(ProjectDir)` so relative paths resolve correctly.

## Contributing
This repository is primarily for portfolio and learning purposes.
- PRs are welcome for improvements like documentation, build cleanup, or portability.
- Please respect academic integrity rules if you are taking the same course.

## License
MIT License. See `LICENSE` for details.

## Academic Integrity
This repository is shared for portfolio and reference purposes. If you are currently taking a similar course, please follow your institution’s academic integrity policy and do not copy solutions for graded work.

