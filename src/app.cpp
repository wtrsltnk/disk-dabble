#include <app.hpp>
#include <glad/glad.h>
#include <imgui.h>
#include <iostream>

#include <openfolderwidget.h>
#include <openimagewidget.h>
#include <opentextwidget.h>

void App::OnInit()
{
    ActivatePath(std::filesystem::current_path());

    glClearColor(0.56f, 0.6f, 0.58f, 1.0f);
}

void App::ActivatePath(
    const std::filesystem::path &path)
{
    if (std::filesystem::is_directory(path))
    {
        auto widget = std::make_unique<OpenFolderWidget>([&](const std::filesystem::path &p) {
            this->ActivatePath(std::move(p));
        });

        widget->Open(path);

        _queuedDocuments.push_back(std::move(widget));
    }
    else if (OpenImageWidget::IsImage(path))
    {
        auto widget = std::make_unique<OpenImageWidget>();

        widget->Open(path);

        _queuedDocuments.push_back(std::move(widget));
    }
    else
    {
        auto widget = std::make_unique<OpenTextWidget>(_monoSpaceFont);
        widget->Open(path);

        _queuedDocuments.push_back(std::move(widget));
    }
}

void App::OnResize(
    int width,
    int height)
{
    _width = width;
    _height = height;

    glViewport(0, 0, width, height);
}

void App::MainMenu()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open current working directory", "CTRL+N"))
            {
                ActivatePath(std::filesystem::current_path());
            }
            if (ImGui::MenuItem("Open user home directory"))
            {
                ActivatePath(std::filesystem::path(getenv("USERPROFILE")));
            }
            if (ImGui::MenuItem("Open temp directory"))
            {
                ActivatePath(std::filesystem::path(getenv("TEMP")));
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "CTRL+Z"))
            {}
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false))
            {} // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X"))
            {}
            if (ImGui::MenuItem("Copy", "CTRL+C"))
            {}
            if (ImGui::MenuItem("Paste", "CTRL+V"))
            {}
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void App::OnFrame()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ImGui::ShowDemoWindow(nullptr);

    for (const auto &item : _openDocuments)
    {
        ImGui::SetNextWindowDockID(_dockId, ImGuiCond_FirstUseEver);
        item->Render();
    }

    AddQueuedItems();
}

void App::OnExit()
{
}

void App::AddQueuedItems()
{
    while (!_queuedDocuments.empty())
    {
        auto folder = std::move(_queuedDocuments.back());
        _queuedDocuments.pop_back();
        _openDocuments.push_back(std::move(folder));
    }

    while (true)
    {
        auto const &found = std::find_if(
            _openDocuments.begin(),
            _openDocuments.end(),
            [](std::unique_ptr<OpenDocument> &item) -> bool {
                return !item->IsOpen();
            });

        if (found == _openDocuments.cend())
        {
            break;
        }

        _openDocuments.erase(found);
    }
}
