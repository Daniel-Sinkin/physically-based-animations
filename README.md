# Physically Based Animations – OpenGL Viewer

Minimal C++ OpenGL viewer using **GLFW**, **ImGui**, and **glad** (vendored).
Designed to be boring, predictable, and free of Python / code-generation at
build time.

Target platform: **macOS** (OpenGL 3.2 Core Profile, Apple-supported baseline).

## Requirements

- macOS
- Xcode Command Line Tools  
  ```bash
  xcode-select --install

## Building
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```
```
cmake --build build -j
```