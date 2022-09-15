#include <app.hpp>

// Make sure GLAD is included before glfw3
#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <IconsMaterialDesign.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>

#define OPENGL_LATEST_VERSION_MAJOR 4
#define OPENGL_LATEST_VERSION_MINOR 6

static char szProgramName[] = "Disk Dabble";

struct WindowHandle
{
    GLFWwindow *window;
};

App::App(
    const std::vector<std::string> &args)
    : _args(args)
{}

App::~App() = default;

template <class T>
T *App::GetWindowHandle() const
{
    return reinterpret_cast<T *>(_windowHandle);
}

template <class T>
void App::SetWindowHandle(
    T *handle)
{
    _windowHandle = (void *)handle;
}

void App::ClearWindowHandle()
{
    _windowHandle = nullptr;
}

void OpenGLMessageCallback(
    unsigned source,
    unsigned type,
    unsigned id,
    unsigned severity,
    int length,
    const char *message,
    const void *userParam)
{
    (void)userParam;

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
            std::cout << "CRITICAL";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            std::cout << "ERROR";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            std::cout << "WARNING";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            std::cout << "DEBUG";
            break;
        default:
            std::cout << "UNKNOWN";
            break;
    }

    std::cout << "\n    source    : " << source
              << "\n    message   : " << message
              << "\n    type      : " << type
              << "\n    id        : " << id
              << "\n    length    : " << length
              << "\n";
}

bool App::Init()
{
    if (glfwInit() == GLFW_FALSE)
    {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_LATEST_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_LATEST_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    auto window = glfwCreateWindow(_width, _height, szProgramName, nullptr, nullptr);
    if (window == 0)
    {
        std::cout << "Failed to create GLFW window" << std::endl;

        glfwTerminate();

        return false;
    }

    glfwSetWindowUserPointer(window, this);

    glfwMakeContextCurrent(window);

    if (!gladLoadGL())
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;

        glfwTerminate();

        return false;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();

    io.IniFilename = nullptr;
    ImGui::LoadIniSettingsFromDisk((GetUserProfileDir() / "imgui.ini").string().c_str());

    (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

    if (std::filesystem::exists(".\\fonts\\Poppins-Regular.ttf"))
    {
        io.Fonts->AddFontFromFileTTF(".\\fonts\\Poppins-Regular.ttf", 18.0f);
    }
    else
    {
        io.Fonts->AddFontFromFileTTF("c:\\windows\\fonts\\tahoma.ttf", 18.0f);
    }

    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphOffset = ImVec2(0, 4.0f);
    config.GlyphMinAdvanceX = 18.0f; // Use if you want to make the icon monospaced

    if (std::filesystem::exists(".\\fonts\\MaterialIcons-Regular.ttf"))
    {
        static const ImWchar icon_ranges[] = {ICON_MIN_MD, ICON_MAX_MD, 0};
        io.Fonts->AddFontFromFileTTF(".\\fonts\\MaterialIcons-Regular.ttf", 18.0f, &config, icon_ranges);
    }

    if (std::filesystem::exists("fonts/SourceCodePro-Regular.ttf"))
    {
        _monoSpaceFont = io.Fonts->AddFontFromFileTTF("fonts/SourceCodePro-Regular.ttf", 18.0f);
    }
    else
    {
        _monoSpaceFont = io.Fonts->AddFontFromFileTTF("c:\\windows\\fonts\\consola.ttf", 18.0f);
    }

    if (std::filesystem::exists(".\\fonts\\Poppins-Regular.ttf"))
    {
        _largeFont = io.Fonts->AddFontFromFileTTF("fonts/Poppins-Regular.ttf", 28.0f);
    }
    else
    {
        _largeFont = io.Fonts->AddFontFromFileTTF("c:\\windows\\fonts\\tahoma.ttf", 28.0f);
    }

    // Setup Dear ImGui style
    //    ImGui::StyleColorsDark();
    //    ImGui::StyleColorsClassic();
    ImGui::StyleColorsLight();

    ImGui::GetStyle().FrameBorderSize = 1.0f;
    ImGui::GetStyle().FrameRounding = 2.0f;
    ImGui::GetStyle().ItemSpacing = ImVec2(10.0f, 10.0f);
    ImGui::GetStyle().FramePadding = ImVec2(10.0f, 6.0f);
    ImGui::GetStyle().ItemInnerSpacing = ImVec2(10.0f, 6.0f);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    SetWindowHandle(new WindowHandle({
        window,
    }));

    if (GLVersion.major >= 4 && GLVersion.minor >= 3)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(OpenGLMessageCallback, nullptr);

        glDebugMessageControl(
            GL_DONT_CARE,
            GL_DONT_CARE,
            GL_DEBUG_SEVERITY_NOTIFICATION,
            0,
            nullptr,
            GL_FALSE);
    }

    std::cout << "running opengl " << GLVersion.major << "." << GLVersion.minor << std::endl;

    OnInit();

    return true;
}

void window_resize(
    GLFWwindow *window,
    int width,
    int height)
{
    reinterpret_cast<App *>(glfwGetWindowUserPointer(window))->OnResize(width, height);
}

#include <imgui_internal.h>

namespace ImGui
{
    ImGuiID DockSpaceOverViewport2(
        const ImGuiViewport *viewport,
        ImGuiDockNodeFlags dockspace_flags,
        const ImGuiWindowClass *window_class)
    {
        if (viewport == NULL)
            viewport = GetMainViewport();

        SetNextWindowPos(viewport->WorkPos);
        SetNextWindowSize(viewport->WorkSize);
        SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags host_window_flags = 0;
        host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
        host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            host_window_flags |= ImGuiWindowFlags_NoBackground;

        char label[32];
        ImFormatString(label, IM_ARRAYSIZE(label), "DockSpaceViewport_%08X", viewport->ID);

        PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        Begin(label, NULL, host_window_flags);
        PopStyleVar(3);

        ImGuiID dockspace_id = GetID("DockSpace");
        DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags, window_class);
        End();

        return dockspace_id;
    }
} // namespace ImGui

int App::Run()
{
    bool running = true;

    auto windowHandle = std::unique_ptr<WindowHandle>(GetWindowHandle<WindowHandle>());

    glfwSetWindowSizeCallback(windowHandle->window, window_resize);

    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    while (glfwWindowShouldClose(windowHandle->window) == 0 && running)
    {
        glfwWaitEvents();
        glfwMakeContextCurrent(windowHandle->window);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        _dockId = ImGui::DockSpaceOverViewport2(
            nullptr,
            ImGuiDockNodeFlags_PassthruCentralNode,
            nullptr);

        OnFrame();

        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow *backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(windowHandle->window);
    }

    OnExit();

    ImGui::SaveIniSettingsToDisk((GetUserProfileDir() / "imgui.ini").string().c_str());

    ClearWindowHandle();

    return 0;
}
