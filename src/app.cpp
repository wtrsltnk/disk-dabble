#include <app.hpp>
#include <glad/glad.h>
#include <imgui.h>

void App::OnInit()
{
    glClearColor(0.56f, 0.7f, 0.67f, 1.0f);
}

void App::OnResize(int width, int height)
{
    _width = width;
    _height = height;

    glViewport(0, 0, width, height);
}

void App::OnFrame()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui::ShowDemoWindow(nullptr);
}

void App::OnExit()
{
}
