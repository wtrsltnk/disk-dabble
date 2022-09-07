#include <app.hpp>
#include <glad/glad.h>
#include <imgui.h>

void App::OnInit()
{
    _openFolders.push_back(std::make_unique<OpenFolderWidget>(L"c:\\temp", [&](const std::filesystem::path &path) {
        this->ActivatePath(path);
    }));

    glClearColor(0.56f, 0.7f, 0.67f, 1.0f);
}

void App::ActivatePath(
    const std::filesystem::path &path)
{
    if (path.extension() == ".png" || path.extension() == ".bmp")
    {
        _openImages.push_back(std::make_unique<OpenImageWidget>(path));
    }
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

    for (const auto &item : _openFolders)
    {
        item->Render();
    }

    for (const auto &item : _openImages)
    {
        item->Render();
    }
}

void App::OnExit()
{
}
