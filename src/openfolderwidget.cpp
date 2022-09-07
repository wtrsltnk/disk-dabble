#include "openfolderwidget.h"

#include <IconsMaterialDesign.h>
#include <codecvt>
#include <imgui.h>
#include <locale>
#include <string>

OpenFolderWidget::OpenFolderWidget(
    const std::filesystem::path &path,
    std::function<void(const std::filesystem::path &)> onActivatePath)
    : _onActivatePath(onActivatePath),
      _openFolderPath(path)
{
    SetPath(path);
}

bool folderFirst(
    const struct folderItem &a,
    const struct folderItem &b)
{
    if (a.isDir == b.isDir)
    {
        return a.name < b.name;
    }

    return a.isDir > b.isDir;
}

void OpenFolderWidget::SetPath(
    const std::filesystem::path &path)
{
    _openFolderPath = std::filesystem::canonical(path);

    _itemsInFolder.clear();
    for (auto const &dir_entry : std::filesystem::directory_iterator{_openFolderPath})
    {
        struct folderItem item({
            dir_entry.path(),
            dir_entry.path().filename().wstring(),
            std::filesystem::is_directory(dir_entry),
        });

        _itemsInFolder.push_back(item);
    }

    std::sort(_itemsInFolder.begin(), _itemsInFolder.end(), folderFirst);

    _pathInSections.clear();
    auto tmp = _openFolderPath;
    while (tmp.root_path() != tmp)
    {
        _pathInSections.push_back(tmp);

        tmp = tmp.parent_path();
    }

    _pathInSections.push_back(tmp.parent_path());

    std::reverse(_pathInSections.begin(), _pathInSections.end());
}

inline bool ends_with(
    std::wstring const &value,
    std::wstring const &ending)
{
    if (ending.size() > value.size())
    {
        return false;
    }

    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

void OpenFolderWidget::Render()
{
    bool shiftFocusToFind = false;
    static char buffer[64] = {0};

    std::filesystem::path changeToPath = _openFolderPath;

    ImGui::PushID(Id());

    ImGui::Begin("Open Folder Widget");

    bool isFocusedWindow = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);

    if (isFocusedWindow)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_F) && ImGui::GetIO().KeyCtrl)
        {
            _showFind = shiftFocusToFind = true;
        }
        else if (_showFind && ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            _showFind = false;
            _findBuffer = L"";
            buffer[0] = 0;
        }
    }

    if (ImGui::Button(ICON_MD_NORTH))
    {
        changeToPath = _openFolderPath.parent_path();
    }

    for (auto &section : _pathInSections)
    {
        ImGui::SameLine(0.0f, 5.0f);

        auto filename = section.filename().wstring();

        if (filename.empty())
        {
            filename = section.wstring();
        }

        if (ends_with(filename, L"/") || ends_with(filename, L"\\"))
        {
            filename = filename.substr(0, filename.size() - 1);
        }

        if (ImGui::Button(converter.to_bytes(filename).c_str()))
        {
            changeToPath = section;
        }

        ImGui::SameLine(0.0f, 5.0f);

        ImGui::Text("/");
    }

    ImGui::BeginChild("entries", ImVec2(0, _showFind ? -50 : 0));

    for (auto const &dir_entry : _itemsInFolder)
    {
        if (dir_entry.name.find(_findBuffer) == std::string::npos)
        {
            continue;
        }

        ImGui::PushID(dir_entry.name.c_str());

        auto entryName = dir_entry.name;
        auto isDir = std::filesystem::is_directory(dir_entry.path);

        bool isSelected = false;
        if (isDir)
        {
            isSelected = ImGui::Selectable(ICON_MD_FOLDER, _selectedSubPath == dir_entry.path);
        }
        else
        {
            isSelected = ImGui::Selectable(ICON_MD_DESCRIPTION, _selectedSubPath == dir_entry.path);
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            if (!isDir || ImGui::GetIO().KeyCtrl)
            {
                _onActivatePath(dir_entry.path);
            }
            else
            {
                changeToPath = dir_entry.path;
            }
        }

        if (isSelected)
        {
            _selectedSubPath = dir_entry.path;
        }

        ImGui::SameLine();
        ImGui::Text("%s", converter.to_bytes(entryName).c_str());

        ImGui::PopID();
    }

    ImGui::EndChild();

    if (_showFind)
    {
        ImGui::BeginChild("Find");
        if (shiftFocusToFind)
        {
            ImGui::SetKeyboardFocusHere(0);
        }
        if (ImGui::InputText("Find filename", buffer, 64))
        {
            _findBuffer = converter.from_bytes(buffer);
        }
        ImGui::EndChild();
    }

    ImGui::End();

    ImGui::PopID();

    if (changeToPath != _openFolderPath)
    {
        SetPath(changeToPath);
    }
}
