// pba/gfx/gfx_init.cpp
#include "pba/gfx/gfx_context.hpp"
#include "pba/util/scope_timer.hpp"

namespace ds_pba
{
namespace
{
[[nodiscard]] auto default_imgui_ini_path() -> std::string
{
    if (const auto* home = std::getenv("HOME"); home != nullptr && home[0] != '\0')
    {
        return std::format("{}/.pba_imgui.ini", home);
    }
    return "imgui.ini";
}
}  // namespace

[[nodiscard]] auto GfxContext::setup() -> bool
{
    using namespace ds_pba;

    {
        const ScopeTimer st_glfw{"init glfw"};
        auto glfw_error_callback = [](int error, const char* description)
        { std::println(stderr, "GLFW Error {}: {}", error, description); };

        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
        {
            std::println(stderr, "Failed to init glfw");
            return false;
        }
        initialised_glfw = true;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);

    {
        const ScopeTimer st_glfw{"create window glfw"};
        window = glfwCreateWindow(1600, 900, "Physically Based Animations", nullptr, nullptr);
        if (!window)
        {
            std::println(stderr, "Failed to create window");
            return false;
        }
        window_created = true;
    }
    {  // Place on second monitor if possible
        int monitor_count{0};
        GLFWmonitor* const* monitors = glfwGetMonitors(&monitor_count);

        if (monitor_count >= 2)
        {
            GLFWmonitor* monitor{monitors[1]};

            int monitor_x{0};
            int monitor_y{0};
            glfwGetMonitorPos(monitor, &monitor_x, &monitor_y);

            const GLFWvidmode* mode{glfwGetVideoMode(monitor)};

            const int desired_fb_w{2400};
            const int desired_fb_h{1350};
            f32 sx{1.0f};
            f32 sy{1.0f};
            glfwGetMonitorContentScale(monitor, &sx, &sy);

            const int window_width{std::max(1, static_cast<int>(std::lround(desired_fb_w / sx)))};
            const int window_height{std::max(1, static_cast<int>(std::lround(desired_fb_h / sy)))};
            glfwSetWindowSize(window, window_width, window_height);

            const int window_x{monitor_x + (mode->width - window_width) / 2};
            const int window_y{monitor_y + (mode->height - window_height) / 2};
            glfwSetWindowPos(window, window_x, window_y);
        }
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::println(stderr, "Failed to init glad");
        shutdown();
        return false;
    }
    loaded_glad = true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    initialised_imgui = true;

    ImGuiIO& io = ImGui::GetIO();
    imgui_ini_path = default_imgui_ini_path();
    io.IniFilename = imgui_ini_path.c_str();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    {  // Load Fonts
        default_font = io.Fonts->AddFontDefault();

        fonts_by_id["default"] = default_font;

        if (ImFont* f =
                io.Fonts->AddFontFromFileTTF("assets/fonts/MonaspaceKrypton-Regular.otf", 14.0f))
        {
            fonts_by_id["krypton-14"] = f;
        }

        if (ImFont* f =
                io.Fonts->AddFontFromFileTTF("assets/fonts/MonaspaceArgon-Regular.otf", 14.0f))
        {
            fonts_by_id["argon-14"] = f;
        }
    }
    {  // Load Themes
        auto res = load_theme_pack_json("assets/ui/themes.json");
        if (res && !res->themes.empty())
        {
            theme_pack = std::move(*res);
            theme_loaded = true;

            theme_index = theme_pack.default_index.value_or(0zu);
            theme_index = std::min(theme_index, theme_pack.themes.size() - 1zu);

            apply_theme(theme_pack.themes[theme_index]);
        }
        else
        {
            ImGui::StyleColorsDark();
        }
    }

    const char* glsl_version{"#version 330"};
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
    {
        std::println(stderr, "ImGui_ImplGlfw_InitForOpenGL failed");
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init(glsl_version))
    {
        std::println(stderr, "ImGui_ImplOpenGL3_Init failed");
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    {
        // clang-format off
    if (!create_programs()) { return false; }
    if (!create_meshes())   { return false; }
    if (!create_textures()) { return false; }
    //clang-format on
    }

    last_scene_poll = std::chrono::steady_clock::now();
    return true;
}
}  // namespace ds_pba
