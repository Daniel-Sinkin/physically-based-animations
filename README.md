# Physically Based Animations
## Building
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

```
cmake --build build -j
```

## Notes
### NLohnman ADL serializer
When I want to serialise a type that I didn't define (e.g. glm::vec3) then the lookup on `to_json(glm::vec3)` is going to look at the glm namespace and never see mine. For that reason NLohmann uses a trick to make the ADL (Argument Dependent Lookup) work. See 

It does the following:
```
namespace nlohmann
{
template <>
struct adl_serializer<glm::vec3>
{
    static void to_json(json& j, const glm::vec3& v) {}
    static void from_json(const json& j, glm::vec3& v) {}
};
```

See 
* https://github.com/nlohmann/json?tab=readme-ov-file#how-do-i-convert-third-party-types
* https://en.cppreference.com/w/cpp/language/adl.html

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
