#include <app.hpp>

// Make sure GLAD is included before glfw3
#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <iostream>
#include <memory>

#define OPENGL_LATEST_VERSION_MAJOR 4
#define OPENGL_LATEST_VERSION_MINOR 6

static char szProgramName[] = "Glfw.Imgui.Minimal";

struct WindowHandle
{
    GLFWwindow *window;
};

App::App(const std::vector<std::string> &args)
    : _args(args)
{}

App::~App() = default;

template <class T>
T *App::GetWindowHandle() const
{
    return reinterpret_cast<T *>(_windowHandle);
}

template <class T>
void App::SetWindowHandle(T *handle)
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
    (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

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

void window_resize(GLFWwindow *window, int width, int height)
{
    reinterpret_cast<App *>(glfwGetWindowUserPointer(window))->OnResize(width, height);
}

int App::Run()
{
    bool running = true;

    auto windowHandle = std::unique_ptr<WindowHandle>(GetWindowHandle<WindowHandle>());

    glfwSetWindowSizeCallback(windowHandle->window, window_resize);

    while (glfwWindowShouldClose(windowHandle->window) == 0 && running)
    {
#if ONLY_RENDER_ON_MESSAGE
        glfwWaitEvents();
#else
        glfwPollEvents();
#endif
        glfwMakeContextCurrent(windowHandle->window);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        OnFrame();

        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(windowHandle->window);
    }

    ClearWindowHandle();

    return 0;
}
