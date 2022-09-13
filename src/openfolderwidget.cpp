#include "openfolderwidget.h"

#include <IconsMaterialDesign.h>
#include <imgui.h>
#include <iostream>
#include <sstream>
#include <string>

// Find implementations at teh end of this file
bool recycle_file_folder(
    std::wstring path);

OpenFolderWidget::OpenFolderWidget(
    int index,
    ServiceProvider *services,
    std::function<void(const std::filesystem::path &)> onActivatePath)
    : OpenDocument(index, services),
      _onActivatePath(onActivatePath)
{
    _settingsService = services->Resolve<ISettingsService *>();
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

void OpenFolderWidget::Refresh()
{
    _activeSubPath.clear();
    _showFind = false;
    _findBuffer = L"";
    _buffer[0] = 0;
    _isBookmark = _settingsService->IsBookmarked(_documentPath);

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

    Refresh();
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
            else if (ImGui::IsKeyPressed(ImGuiKey_Delete))
            {
                if (ImGui::GetIO().KeyShift)
                {
                    _showDeletePopup = true;
                }
                else
                {
#ifdef _WIN32
                    MoveSelectionToTrash();
#endif
                }
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
            _settingsService->SetBookmarked(_documentPath, _isBookmark);
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

    ImGui::BeginChild("entries", ImVec2(0.0f, (_showFind ? -60.0f : 0.0f) + (_showInfo ? -130.0f : 0.0f)));

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

        if (!_activeSubPath.empty() && ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Open"))
            {
                _onActivatePath(dir_entry.path);
            }

            if (ImGui::BeginMenu("Open with..."))
            {
                auto openWithOptions = _settingsService->GetOpenWithOptions(_activeSubPath.extension().wstring());

                for (const auto &openWithOption : openWithOptions)
                {
                    ImGui::PushID(openWithOption.id);
                    if (ImGui::MenuItem(Convert(openWithOption.name).c_str()))
                    {
                        ExecuteCommand(_activeSubPath, openWithOption.command);
                    }
                    ImGui::PopID();
                }

                if (!openWithOptions.empty())
                {
                    ImGui::Separator();
                }

                if (ImGui::MenuItem("Manage all options..."))
                {
                    _showManageOpenWithOptionsPopup = true;
                }
                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Copy"))
            {
            }

            if (ImGui::MenuItem("Cut"))
            {
            }

            if (ImGui::MenuItem("Paste"))
            {
            }

#ifdef _WIN32
            if (ImGui::MenuItem("Move to trash"))
            {
                _showMoveToTrashPopup = true;
            }
#endif
            if (ImGui::MenuItem("Delete"))
            {
                _showDeletePopup = true;
            }

            if (isDir)
            {
                ImGui::Separator();

                auto bookmarked = _settingsService->IsBookmarked(dir_entry.path);
                if (!bookmarked)
                {
                    if (ImGui::MenuItem("Bookmark"))
                    {
                        _settingsService->SetBookmarked(dir_entry.path, true);
                    }
                }
                else
                {
                    if (ImGui::MenuItem("Unbookmark"))
                    {
                        _settingsService->SetBookmarked(dir_entry.path, false);
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
        ImGui::PushStyleColor(
            ImGuiCol_ChildBg,
            IM_COL32(0, 0, 0, 15));

        ImGui::BeginChild("Info", ImVec2(0.0f, 120.0f));

        ImGui::Text("Filename:\t%s", Convert(_documentPath.filename().wstring()).c_str());

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

    if (_showDeletePopup)
    {
        ImGui::OpenPopup("Delete item?");
    }

    ImGui::SetNextWindowSize(ImVec2(250, 120));
    if (ImGui::BeginPopupModal("Delete item?", &_showDeletePopup))
    {
        ImGui::Text("Are you sure?");
        if (ImGui::Button("Yes"))
        {
            _showDeletePopup = false;
            DeleteSelection();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();

        if (ImGui::Button("No"))
        {
            _showDeletePopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (_showMoveToTrashPopup)
    {
        ImGui::OpenPopup("Move item to trash?");
    }

    ImGui::SetNextWindowSize(ImVec2(250, 120));
    if (ImGui::BeginPopupModal("Move item to trash?", &_showMoveToTrashPopup))
    {
        ImGui::Text("Are you sure?");
        if (ImGui::Button("Yes"))
        {
            _showMoveToTrashPopup = false;
            MoveSelectionToTrash();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();

        if (ImGui::Button("No"))
        {
            _showMoveToTrashPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (_showManageOpenWithOptionsPopup)
    {
        ImGui::OpenPopup("Manage Open With options");
    }

    ImGui::SetNextWindowSize(ImVec2(750, 0));
    if (ImGui::BeginPopupModal("Manage Open With options", &_showManageOpenWithOptionsPopup, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        static char name_buffer[64] = {0};
        static char pattern_buffer[512] = {0};
        static char command_buffer[512] = {0};
        static int item_current_idx = -1;
        static bool form_is_dirty = false;

        ImGui::Columns(2, "mycolumns");
        ImGui::SetColumnWidth(0, 200.0f);

        ImGui::Text("Open With options for %s", Convert(_activeSubPath.extension()).c_str());

        if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, 10 * ImGui::GetTextLineHeightWithSpacing())))
        {
            auto openWithOptions = _settingsService->GetOpenWithOptions(L"*");

            for (const auto &openWithOption : openWithOptions)
            {
                std::wstringstream wss;
                wss << openWithOption.name << "###" << openWithOption.id;
                const bool is_selected = (item_current_idx == openWithOption.id);
                if (ImGui::Selectable(Convert(wss.str()).c_str(), is_selected))
                {
                    item_current_idx = openWithOption.id;

                    strcpy_s(name_buffer, 63, Convert(openWithOption.name).c_str());
                    strcpy_s(pattern_buffer, 511, Convert(openWithOption.extensionPatterns).c_str());
                    strcpy_s(command_buffer, 511, Convert(openWithOption.command).c_str());
                    form_is_dirty = false;
                }

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndListBox();

            if (ImGui::Button("Add option"))
            {
                std::wstringstream wss;
                wss << L"*" << _activeSubPath.extension();

                strcpy_s(name_buffer, 63, "New option");
                strcpy_s(pattern_buffer, 511, Convert(wss.str()).c_str());
                strcpy_s(command_buffer, 511, "");
                form_is_dirty = false;

                item_current_idx = _settingsService->AddOpenWithOption(
                    Convert(name_buffer),
                    wss.str(),
                    L"");
            }
        }
        ImGui::NextColumn();

        if (ImGui::InputText("name", name_buffer, 64))
        {
            form_is_dirty = true;
        }
        if (ImGui::InputTextMultiline("pattern", pattern_buffer, 512))
        {
            form_is_dirty = true;
        }
        if (ImGui::InputTextMultiline("command", command_buffer, 512))
        {
            form_is_dirty = true;
        }

        if (form_is_dirty && ImGui::Button("Save"))
        {
            _settingsService->UpdateOpenWithOption(
                item_current_idx,
                Convert(name_buffer),
                Convert(pattern_buffer),
                Convert(command_buffer));

            form_is_dirty = false;
        }

        if (!form_is_dirty && ImGui::Button("Delete"))
        {
            _settingsService->DeleteOpenWithOption(
                item_current_idx);

            item_current_idx = -1;

            strcpy_s(name_buffer, 63, "");
            strcpy_s(pattern_buffer, 511, "");
            strcpy_s(command_buffer, 511, "");
            form_is_dirty = false;
        }

        ImGui::Columns(1);
        if (ImGui::Button("Close"))
        {
            _showManageOpenWithOptionsPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
bool replace(std::wstring &str, const std::wstring &from, const std::wstring &to)
{
    size_t start_pos = str.find(from);
    if (start_pos == std::wstring::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

void OpenFolderWidget::ExecuteCommand(
    const std::filesystem::path &path,
    const std::wstring &command)
{
    std::wstring patchedCommand = command;

    replace(patchedCommand, L"$1", path.wstring());

    system(Convert(patchedCommand).c_str());
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

void OpenFolderWidget::DeleteSelection()
{
    std::filesystem::remove(_activeSubPath);

    Refresh();
}

void OpenFolderWidget::MoveSelectionToTrash()
{
#ifdef _WIN32
    recycle_file_folder(_activeSubPath);
#endif

    Refresh();
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

#ifdef _WIN32
#include <windows.h>

bool recycle_file_folder(
    std::wstring path)
{

    std::wstring widestr = path + std::wstring(1, L'\0');

    SHFILEOPSTRUCT fileOp;
    fileOp.hwnd = NULL;
    fileOp.wFunc = FO_DELETE;
    fileOp.pFrom = widestr.c_str();
    fileOp.pTo = NULL;
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOERRORUI | FOF_NOCONFIRMATION | FOF_SILENT;
    int result = SHFileOperation(&fileOp);

    if (result != 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}
#endif
