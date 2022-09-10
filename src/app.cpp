#include <app.hpp>
#include <fstream>
#include <glad/glad.h>
#include <imgui.h>
#include <iostream>

#include <IconsMaterialDesign.h>
#include <openfolderwidget.h>
#include <openimagewidget.h>
#include <opentextwidget.h>

#define OPEN_FILES_FILENAME "open-files.txt"

void App::OnInit()
{
    _services.Add<IBookmarkService *>(
        [&](ServiceProvider &sp) -> GenericServicePtr {
            return (GenericServicePtr)&_settingsService;
        });

    auto openFiles = _settingsService.GetOpenFiles();

    for (const auto &pair : openFiles)
    {
        if (!std::filesystem::exists(pair.second))
        {
            continue;
        }

        ActivatePath(pair.second, pair.first);
    }

    if (_openDocuments.empty() && _queuedDocuments.empty())
    {
        ActivatePath(std::filesystem::current_path());
    }

    glClearColor(0.56f, 0.6f, 0.58f, 1.0f);
}

void App::OnExit()
{
    std::map<int, std::filesystem::path> res;

    for (const auto &openDoc : _openDocuments)
    {
        res.insert(std::make_pair(openDoc->Id(), openDoc->DocumentPath()));
    }

    _settingsService.SetOpenFiles(res);
}

void App::ActivatePath(
    const std::filesystem::path &path,
    int index)
{
    if (std::filesystem::is_directory(path))
    {
        auto widget = std::make_unique<OpenFolderWidget>(
            index,
            &_services,
            [&](const std::filesystem::path &p) {
                this->ActivatePath(std::move(p));
            });

        widget->Open(path);

        _queuedDocuments.push_back(std::move(widget));
    }
    else if (OpenImageWidget::IsImage(path))
    {
        auto widget = std::make_unique<OpenImageWidget>(
            index,
            &_services);

        widget->Open(path);

        _queuedDocuments.push_back(std::move(widget));
    }
    else
    {
        auto widget = std::make_unique<OpenTextWidget>(
            index,
            &_services,
            _monoSpaceFont);
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

void App::OnFrame()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto viewPort = ImGui::GetMainViewport();

    //ImGui::ShowDemoWindow(nullptr);

    for (const auto &item : _openDocuments)
    {
        ImGui::SetNextWindowDockID(_dockId, ImGuiCond_FirstUseEver);
        item->Render();
    }

    static bool open = true;
    if (ImGui::IsKeyPressed(ImGuiKey_LeftAlt))
    {
        if (!ImGui::IsPopupOpen("Save?"))
            ImGui::OpenPopup("Save?");
    }

    ImGui::SetNextWindowPos(viewPort->Pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(250, _height));
    auto flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::BeginPopupModal("Save?", &open, flags))
    {
        auto buttonSize = ImVec2(250 - (2 * ImGui::GetStyle().ItemSpacing.x), 0);

        auto pos = ImGui::GetCursorPos();

        ImGui::PushFont(_largeFont);
        ImGui::Text("Disk Dabble");
        ImGui::PopFont();

        if (!_openDocuments.empty())
        {
            ImGui::SetCursorPos(ImVec2(buttonSize.x - 30, pos.y));
            if (ImGui::Button(ICON_MD_CLOSE))
            {
                ImGui::CloseCurrentPopup();
            }
        }

        if (ImGui::Button("Open current working directory", buttonSize))
        {
            ActivatePath(std::filesystem::current_path());
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Open user home directory", buttonSize))
        {
            ActivatePath(std::filesystem::path(getenv("USERPROFILE")));
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Open temp directory", buttonSize))
        {
            ActivatePath(std::filesystem::path(getenv("TEMP")));
            ImGui::CloseCurrentPopup();
        }

        if (!_settingsService.Bookmarks().empty())
        {
            ImGui::Separator();

            for (auto &bookmark : _settingsService.Bookmarks())
            {
                if (ImGui::Button(bookmark.filename().string().c_str(), buttonSize))
                {
                    ActivatePath(bookmark);
                    ImGui::CloseCurrentPopup();
                }
            }
        }

        ImGui::Separator();

        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    AddQueuedItems();
}

std::filesystem::path App::GetUserProfileDir()
{
    auto userProfileDir = std::filesystem::path(getenv("USERPROFILE")) / "disk-dabble";

    if (!std::filesystem::exists(userProfileDir))
    {
        std::filesystem::create_directory(userProfileDir);
    }

    return userProfileDir;
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

    if (_openDocuments.empty())
    {
        ImGui::OpenPopup("Save?");
    }
}
