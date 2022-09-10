#include "openfolderwidget.h"

#include <IconsMaterialDesign.h>
#include <imgui.h>
#include <iostream>
#include <string>

OpenFolderWidget::OpenFolderWidget(
    int index,
    ServiceProvider *services,
    std::function<void(const std::filesystem::path &)> onActivatePath)
    : OpenDocument(index, services),
      _onActivatePath(onActivatePath)
{
    _bookmarkService = services->Resolve<IBookmarkService *>();
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

void OpenFolderWidget::OnPathChanged(
    const std::filesystem::path &oldPath)
{
    if (!oldPath.empty())
    {
        if (!_pathChangeIsTravel)
        {
            _travelledPaths.push_back(oldPath);
            _pathsToTravel.clear();
        }
        else
        {
            _pathsToTravel.push_back(oldPath);
        }
    }
    _pathChangeIsTravel = false;

    _activeSubPath.clear();
    _showFind = false;
    _findBuffer = L"";
    _buffer[0] = 0;
    _isBookmark = _bookmarkService->IsBookmarked(_documentPath);

    _itemsInFolder.clear();
    for (auto const &dir_entry : std::filesystem::directory_iterator{_documentPath})
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
    auto tmp = _documentPath;
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

class StyleGuard
{
public:
    virtual ~StyleGuard()
    {
        ImGui::PopStyleColor(_stylesToPop);
    }

    void Push(
        ImGuiCol idx,
        ImU32 col)
    {
        ImGui::PushStyleColor(idx, col);
        _stylesToPop++;
    }

private:
    int _stylesToPop = 0;
};

void OpenFolderWidget::OnRender()
{
    if (_documentPath.empty())
    {
        return;
    }

    bool shiftFocusToFind = false;

    ImGui::Begin(ConstructWindowID().c_str(), &_isOpen);

    auto pos = ImGui::GetCursorPos();

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_DockHierarchy))
    {
        if (ImGui::IsKeyPressed(ImGuiKey_F) && ImGui::GetIO().KeyCtrl)
        {
            _showFind = shiftFocusToFind = true;
        }
        else if (_showFind && ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            _showFind = false;
            _findBuffer = L"";
            _buffer[0] = 0;
        }
        else if (!_showFind)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
            {
                if (ImGui::GetIO().KeyAlt)
                {
                    Open(_documentPath.parent_path());
                }
                else
                {
                    MoveSelectionUp(ImGui::GetIO().KeyCtrl ? 5 : 1);
                }
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
            {
                MoveSelectionDown(ImGui::GetIO().KeyCtrl ? 5 : 1);
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_Enter))
            {
                ActivateItem(_activeSubPath);
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_Backspace))
            {
                Open(_documentPath.parent_path());
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_End))
            {
                MoveSelectionToEnd();
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_Home))
            {
                MoveSelectionToStart();
            }
        }
    }

    ImGui::SetCursorPos(ImVec2(ImGui::GetContentRegionAvail().x - 30, pos.y));
    {
        StyleGuard style;
        if (!_isBookmark)
        {
            style.Push(ImGuiCol_Text, IM_COL32(0, 0, 0, 55));
        }
        if (ImGui::Button(ICON_MD_STAR))
        {
            _isBookmark = !_isBookmark;
            _bookmarkService->SetBookmarked(_documentPath, _isBookmark);
        }
    }

    ImGui::SetCursorPos(ImVec2(ImGui::GetContentRegionAvail().x - 80, pos.y));
    {
        StyleGuard style;
        if (!_showInfo)
        {
            style.Push(ImGuiCol_Text, IM_COL32(0, 0, 0, 55));
        }
        if (ImGui::Button(ICON_MD_INFO))
        {
            ToggleShowInfo();
        }
    }

    ImGui::SetCursorPos(pos);
    {
        StyleGuard style;
        if (_travelledPaths.empty())
        {
            style.Push(ImGuiCol_Text, IM_COL32(0, 0, 0, 55));
        }
        if (ImGui::Button(ICON_MD_WEST))
        {
            TravelBack();
        }
    }

    ImGui::SameLine(0.0f, 5.0f);

    {
        StyleGuard style;
        if (_pathsToTravel.empty())
        {
            style.Push(ImGuiCol_Text, IM_COL32(0, 0, 0, 55));
        }
        if (ImGui::Button(ICON_MD_EAST))
        {
            TravelForward();
        }
    }

    ImGui::SameLine(0.0f, 5.0f);

    {
        StyleGuard style;
        if (_documentPath == _documentPath.root_path())
        {
            style.Push(ImGuiCol_Text, IM_COL32(0, 0, 0, 55));
        }
        if (ImGui::Button(ICON_MD_NORTH))
        {
            OpenParentDirectory();
        }
    }

    ImGui::SameLine(0.0f, 5.0f);

    ImGui::Text(" | ");

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

        if (ImGui::Button(Convert(filename).c_str()))
        {
            Open(section);
        }

        ImGui::SameLine(0.0f, 5.0f);

        ImGui::Text("/");
    }

    ImGui::BeginChild("entries", ImVec2(_showInfo ? -200.0f : 0.0f, _showFind ? -50.0f : 0.0f));

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
            isSelected = ImGui::Selectable(ICON_MD_FOLDER, _activeSubPath == dir_entry.path);
        }
        else
        {
            isSelected = ImGui::Selectable(ICON_MD_DESCRIPTION, _activeSubPath == dir_entry.path);
        }

        if (ImGui::BeginPopupContextItem())
        {
            if (isDir)
            {
                auto bookmarked = _bookmarkService->IsBookmarked(dir_entry.path);
                if (!bookmarked)
                {
                    if (ImGui::MenuItem("Bookmark"))
                    {
                        _bookmarkService->SetBookmarked(dir_entry.path, true);
                    }
                }
                else
                {
                    if (ImGui::MenuItem("Unbookmark"))
                    {
                        _bookmarkService->SetBookmarked(dir_entry.path, false);
                    }
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Properties"))
            {}
            ImGui::EndPopup();
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            if (!isDir || ImGui::GetIO().KeyCtrl)
            {
                _onActivatePath(dir_entry.path);
            }
            else
            {
                Open(dir_entry.path);
            }
        }

        if (isSelected)
        {
            _activeSubPath = dir_entry.path;
        }

        ImGui::SameLine();
        ImGui::Text("%s", Convert(entryName).c_str());

        ImGui::PopID();
    }

    ImGui::EndChild();

    if (_showInfo)
    {
        ImGui::SameLine();

        ImGui::PushStyleColor(
            ImGuiCol_ChildBg,
            IM_COL32(0, 0, 0, 25));

        ImGui::BeginChild("Info", ImVec2(0.0f, _showFind ? -50.0f : 0.0f));

        ImGui::Text("Filename:\n\t%s", Convert(_documentPath.filename().wstring()).c_str());

        ImGui::Text("TODO");

        ImGui::EndChild();

        ImGui::PopStyleColor();
    }

    if (_showFind)
    {
        ImGui::BeginChild("Find");
        if (shiftFocusToFind)
        {
            ImGui::SetKeyboardFocusHere(0);
        }
        if (ImGui::InputText("Find filename", _buffer, 64))
        {
            _findBuffer = Convert(_buffer);
            _activeSubPath.clear();
        }
        ImGui::EndChild();
    }

    ImGui::End();
}

void OpenFolderWidget::ToggleShowInfo()
{
    _showInfo = !_showInfo;

    if (_showInfo)
    {
        _fileInfo.status = std::filesystem::status(_documentPath);
    }
}

void OpenFolderWidget::MoveSelectionUp(
    int count)
{
    if (_itemsInFolder.empty())
    {
        return;
    }

    if (_activeSubPath.empty())
    {
        _activeSubPath = _itemsInFolder.front().path;

        return;
    }

    auto found = std::find_if(
        _itemsInFolder.begin(),
        _itemsInFolder.end(),
        [&](struct folderItem &item) {
            return item.path == _activeSubPath;
        });

    if (found == _itemsInFolder.end() || found == _itemsInFolder.begin())
    {
        return;
    }

    for (int i = 0; i < count; i++)
    {
        --found;

        _activeSubPath = found->path;

        if (found == _itemsInFolder.begin())
        {
            break;
        }
    }
}

void OpenFolderWidget::MoveSelectionDown(
    int count)
{
    if (_itemsInFolder.empty())
    {
        return;
    }

    if (_activeSubPath.empty())
    {
        _activeSubPath = _itemsInFolder.front().path;

        return;
    }

    auto found = std::find_if(
        _itemsInFolder.begin(),
        _itemsInFolder.end(),
        [&](struct folderItem &item) {
            return item.path == _activeSubPath;
        });

    if (found == _itemsInFolder.end())
    {
        return;
    }

    for (int i = 0; i < count; i++)
    {
        ++found;

        if (found == _itemsInFolder.end())
        {
            break;
        }

        _activeSubPath = found->path;
    }
}

void OpenFolderWidget::MoveSelectionToEnd()
{
    if (_itemsInFolder.empty())
    {
        return;
    }

    _activeSubPath = _itemsInFolder.back().path;
}

void OpenFolderWidget::MoveSelectionToStart()
{
    if (_itemsInFolder.empty())
    {
        return;
    }

    _activeSubPath = _itemsInFolder.front().path;
}

void OpenFolderWidget::OpenParentDirectory()
{
    Open(_documentPath.parent_path());
}

void OpenFolderWidget::ActivateItem(
    const std::filesystem::path &path)
{
    if (path.empty())
    {
        return;
    }

    if (!std::filesystem::is_directory(path) || ImGui::GetIO().KeyCtrl)
    {
        _onActivatePath(path);
    }
    else
    {
        Open(path);
    }
}

void OpenFolderWidget::TravelBack()
{
    if (_travelledPaths.empty())
    {
        return;
    }

    auto nextPath = _travelledPaths.back();
    _travelledPaths.pop_back();

    _pathChangeIsTravel = true;

    Open(nextPath);
}

void OpenFolderWidget::TravelForward()
{
    if (_pathsToTravel.empty())
    {
        return;
    }

    auto nextPath = _pathsToTravel.back();
    _pathsToTravel.pop_back();

    Open(nextPath);
}
