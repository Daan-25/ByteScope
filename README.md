# ByteScope

A cross-platform C++17 GUI application that visualizes binary files as a zoomable byte heatmap and optionally compares two files with a diff overlay.

Built with **Dear ImGui + GLFW + OpenGL 3 + ImPlot** — no heavy frameworks, fully offline.

![ByteScope](https://img.shields.io/badge/C%2B%2B-17-blue) ![License](https://img.shields.io/badge/license-MIT-green)

## Features

- **Byte heatmap** – Represent a file as a 2D grid of colored blocks; each block aggregates N bytes (configurable: 64/128/256/512/1024). Color by Shannon entropy (0–8) or average byte value.
- **Diff overlay** – Load two files and see which regions changed; diff ratio per block displayed as a red overlay.
- **Zoom & pan** – Mouse-wheel zoom, right-click drag to pan, "Fit to window" button.
- **Inspector panel** – Click a block to see its offset range, entropy, diff ratio, and a hex preview; in diff mode shows side-by-side old/new hex with highlighted differences.
- **String extraction** – Extracts ASCII (0x20–0x7E) and UTF-16LE strings with configurable minimum length; searchable/filterable list; click to jump to the block.
- **Entropy chart** – Line plot of per-block entropy via ImPlot; click to jump to a block.
- **Diff summary** – File sizes, SHA-256 hashes, match status, total differing bytes, top 10 most-changed blocks (clickable).
- **Export** – Export diff ranges to a text file.
- **Drag & drop** – GLFW drop callback for 1 or 2 files.
- **Background loading** – Files are loaded and hashed in a worker thread; the UI stays responsive for large files.

## Requirements

- CMake ≥ 3.16
- C++17 compiler (GCC, Clang, MSVC)
- OpenGL 3.3+ capable driver
- **Linux**: `libgl-dev`, `libx11-dev`, `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, `libxi-dev` (and optionally wayland dev packages)
- **macOS**: Xcode command-line tools (ships with OpenGL)
- **Windows**: Visual Studio 2019+ or MinGW with OpenGL support

All other dependencies (GLFW, Dear ImGui, ImPlot) are fetched automatically via CMake FetchContent.

## Build

```bash
cmake -S . -B build
cmake --build build -j$(nproc)
```

The binary will be at `build/ByteScope`.

### Linux (Ubuntu/Debian)

```bash
sudo apt-get install -y build-essential cmake libgl-dev libx11-dev \
    libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    libwayland-dev wayland-protocols libxkbcommon-dev
cmake -S . -B build && cmake --build build -j$(nproc)
```

### macOS

```bash
brew install cmake   # if not already installed
cmake -S . -B build && cmake --build build -j$(sysctl -n hw.logicalcpu)
```

### Windows (Visual Studio)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

## Usage

```bash
./build/ByteScope
```

### Loading files

- **Drag & drop** one file onto the window to view its heatmap.
- **Drag & drop** two files onto the window to enter diff mode.
- Alternatively, type file paths in the top bar and click **Load**.

### Heatmap interaction

| Action | Control |
|---|---|
| Zoom in/out | Mouse wheel |
| Pan | Right-click drag or middle-click drag |
| Select block | Left-click |
| Fit to window | Click "Fit to window" button |

### Settings (left panel)

- **Block size**: 64, 128, 256, 512, or 1024 bytes per block.
- **Min string length**: Minimum character count for extracted strings.
- **Color by entropy**: Toggle between entropy coloring and average-byte-value coloring.

### Diff mode

When two files are loaded:
- The heatmap shows a red overlay proportional to the diff ratio per block.
- The right panel displays a diff summary with SHA-256 hashes, total differing bytes, and the top 10 most-changed blocks.
- Click **Export Diff** to write changed block ranges to a file.

## Project structure

```
CMakeLists.txt              # Top-level CMake with FetchContent
src/
  main.cpp                  # Entry point, GLFW+ImGui setup
  sha256.h / sha256.cpp     # Dependency-free SHA-256
  file_load.h / file_load.cpp   # Background file loading + hashing
  metrics.h / metrics.cpp   # Entropy, avg byte value, diff ratio
  string_extract.h / string_extract.cpp  # ASCII + UTF-16LE string extraction
  diff.h / diff.cpp         # Byte diff counting, top changed blocks, export
  ui.h / ui.cpp             # All ImGui rendering (heatmap, inspector, strings, chart)
```

## License

MIT