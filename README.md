# Physically Based Animations
## Building
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

```
cmake --build build -j
```

## Notes
### DearImGui Begin() API
DearImGUI has a bit unfortunate API design for "Begin*" type functions.

For things like `ImGui::Begin()`, `ImGui::BeginChild()` they ALWAYS pop onto the stack and ALWAYS have to have the corresponding `ImGui::End*()` call, so they must be unconditionally closed while containers like `ImGui::BeginTable()`, `ImGui::BeginMenu()` only get created if and only if that call returns true so they should be conditionally closed:
```cpp
if(ImGui::BeginChild(/*args*/)){
    /*content*/
}
ImGui::EndChild();
```
vs.
```cpp
if(ImGui::BeginTable(/*args*/)){
    /*content*/
    ImGui::EndTable();
}
```

## References
* https://pbr-book.org/4ed/

### Assets
* PolyHaven
    * https://polyhaven.com/a/marble_bust_01
* Fonts
    * https://monaspace.githubnext.com