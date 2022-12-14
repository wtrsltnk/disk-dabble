#include "openfolderwidget.h"

#include <IconsMaterialDesign.h>
#include <imgui.h>
#include <iostream>
#include <sstream>
#include <string>

// Find implementations at the end of this file
bool recycle_file_folder(
    std::wstring path);

void SelectionState::Clear()
{
    activePath.clear();
    selectedPaths.clear();
}

void SelectionState::SetSelection(
    const std::filesystem::path &path)
{
    Clear();
    activePath = path;
    selectedPaths.insert(path);
}

void SelectionState::AddToSelection(
    const std::filesystem::path &path)
{
    activePath = path;
    selectedPaths.insert(path);
}

bool SelectionState::IsSelected(
    const std::filesystem::path &path)
{
    //    if (activePath == path)
    //    {
    //        return true;
    //    }

    for (const auto &p : selectedPaths)
    {
        if (p == path)
        {
            return true;
        }
    }

    return false;
}

void SelectionState::ToggleActivePathSelection()
{
    for (const auto &p : selectedPaths)
    {
        if (p == activePath)
        {
            selectedPaths.erase(p);

            return;
        }
    }

    selectedPaths.insert(activePath);
}

OpenFolderWidget::OpenFolderWidget(
    int index,
    ServiceProvider *services)
    : OpenDocument(index, services)
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

void OpenFolderWidget::Refresh(
    const std::filesystem::path &oldPath)
{
    _currentSelection.Clear();
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

        if (oldPath == dir_entry.path())
        {
            _currentSelection.SetSelection(dir_entry.path());
        }

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

    Refresh(oldPath);
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

void OpenFolderWidget::RenderPathItemContextMenu(
    const std::filesystem::path &file)
{
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Open"))
        {
            ActivatePath(file, true);
        }

        if (ImGui::BeginMenu("Open with..."))
        {
            auto openWithOptions = _settingsService->GetOpenWithOptions(file);

            for (const auto &openWithOption : openWithOptions)
            {
                ImGui::PushID(openWithOption.id);
                if (ImGui::MenuItem(Convert(openWithOption.name).c_str()))
                {
                    if (openWithOption.isCommandLineApp)
                    {
                        ExecuteRunInCommand(file, openWithOption.command, [this]() { Refresh(); });
                    }
                    else
                    {
                        ExecuteOpenWithCommand(file, openWithOption.command);
                    }
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
            _pathsToMove.clear();
            _pathsToCopy.clear();
            if (file.empty())
            {
                _pathsToCopy.push_back(_documentPath);
            }
            else
            {
                _pathsToCopy.push_back(file);
            }
        }

        if (ImGui::MenuItem("Cut"))
        {
            _pathsToMove.clear();
            _pathsToCopy.clear();
            if (file.empty())
            {
                _pathsToMove.push_back(_documentPath);
            }
            else
            {
                _pathsToMove.push_back(file);
            }
        }

        if (ImGui::MenuItem("Paste"))
        {
            if (!_pathsToCopy.empty())
            {
                Paste(_pathsToCopy, false);
            }
            else if (!_pathsToMove.empty())
            {
                Paste(_pathsToMove, true);
            }
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

        if (std::filesystem::is_directory(file))
        {
            ImGui::Separator();

            auto bookmarked = _settingsService->IsBookmarked(file);
            if (!bookmarked)
            {
                if (ImGui::MenuItem("Bookmark"))
                {
                    _settingsService->SetBookmarked(file, true);
                }
            }
            else
            {
                if (ImGui::MenuItem("Unbookmark"))
                {
                    _settingsService->SetBookmarked(file, false);
                }
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Properties"))
        {}

        ImGui::EndPopup();
    }
}

void OpenFolderWidget::Paste(
    const std::vector<std::filesystem::path> &paths,
    bool move)
{
    if (!paths.empty() && std::filesystem::is_directory(_currentSelection.activePath))
    {
        std::vector<std::filesystem::path> files;
        for (const auto &path : paths)
        {
            if (std::filesystem::is_directory(path))
            {
                std::wstringstream ss;

                ss << L"robocopy \"" << path.wstring() << L"\" \"" << (_currentSelection.activePath / path.filename()).wstring() << "\" /e /z";

                if (move)
                {
                    ss << L" /move";
                }

                ExecuteRunInCommand("", ss.str(), [this]() { Refresh(); });
            }
            else
            {
                files.push_back(path);
            }
        }

        if (!files.empty())
        {
            std::wstringstream ss;
            ss << L"robocopy \"" << files.front().parent_path().wstring() << L"\" \"" << _currentSelection.activePath.wstring() << "\"";

            for (const auto &file : files)
            {
                ss << " \"" << file.filename().wstring() << L"\"";
            }

            ss << " /e /z";

            if (move)
            {
                ss << L" /mov";
            }

            ExecuteRunInCommand("", ss.str(), [this]() { Refresh(); });
        }
    }
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
                ActivateItem(_currentSelection.activePath);
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_Backspace))
            {
                Open(_documentPath.parent_path());
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_Space) && ImGui::GetIO().KeyShift)
            {
                _currentSelection.ToggleActivePathSelection();
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
    RenderButton(
        ICON_MD_STAR,
        !_isBookmark,
        [&]() { ToggleBookmark(); });

    ImGui::SetCursorPos(ImVec2(ImGui::GetContentRegionAvail().x - 80, pos.y));
    RenderButton(
        ICON_MD_INFO,
        !_showInfo,
        [&]() { ToggleShowInfo(); });

    ImGui::SetCursorPos(pos);
    RenderButton(
        ICON_MD_WEST,
        _travelledPaths.empty(),
        [&]() { TravelBack(); });

    ImGui::SameLine(0.0f, 5.0f);

    RenderButton(
        ICON_MD_EAST,
        _pathsToTravel.empty(),
        [&]() { TravelForward(); });

    ImGui::SameLine(0.0f, 5.0f);

    RenderButton(
        ICON_MD_NORTH,
        _documentPath == _documentPath.root_path(),
        [&]() { OpenParentDirectory(); });

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

        auto selectableMin = ImGui::GetCursorScreenPos();
        auto selectableWidth = ImGui::GetContentRegionAvail().x;

        bool isSelected = _currentSelection.IsSelected(dir_entry.path);
        if (isDir)
        {
            isSelected = ImGui::Selectable(ICON_MD_FOLDER, isSelected);
        }
        else
        {
            isSelected = ImGui::Selectable(ICON_MD_DESCRIPTION, isSelected);
        }

        RenderPathItemContextMenu(dir_entry.path);

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            if (!isDir || ImGui::GetIO().KeyCtrl)
            {
                ActivatePath(dir_entry.path, true);
            }
            else
            {
                Open(dir_entry.path);
                ActivatePath(dir_entry.path, false);
            }
        }

        if (isSelected)
        {
            _currentSelection.SetSelection(dir_entry.path);
        }

        ImGui::SameLine();
        if (dir_entry.path == _currentSelection.activePath)
        {
            ImGui::GetForegroundDrawList()->AddRect(
                ImVec2(
                    selectableMin.x,
                    selectableMin.y - ImGui::GetStyle().ItemSpacing.y / 2),
                ImVec2(
                    selectableMin.x + selectableWidth,
                    selectableMin.y + ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y / 2),
                IM_COL32(0, 20, 50, 255));
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 20, 50, 255));
        }
        ImGui::Text("%s", Convert(entryName).c_str());

        if (dir_entry.path == _currentSelection.activePath)
        {
            ImGui::PopStyleColor();
        }

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
            _currentSelection.activePath.clear();
        }
        ImGui::EndChild();
    }

    ImGui::End();

    RenderYesNoDialog(
        _showDeletePopup,
        L"Delete item?",
        L"Are you sure?",
        [this]() {
            DeleteSelection();
        });

    RenderYesNoDialog(
        _showMoveToTrashPopup,
        L"Move item to trash?",
        L"Are you sure?",
        [this]() {
            MoveSelectionToTrash();
        });

    RenderOpenWithoptionsDialog();
}

void OpenFolderWidget::RenderOpenWithoptionsDialog()
{
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
        static bool is_cmd_app = false;
        static int item_current_idx = -1;
        static bool form_is_dirty = false;

        ImGui::Text("Open With options for %s", Convert(_currentSelection.activePath).c_str());

        ImGui::Columns(2, "mycolumns");
        ImGui::SetColumnWidth(0, 200.0f);

        if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, 15 * ImGui::GetTextLineHeightWithSpacing())))
        {
            auto openWithOptions = _settingsService->GetOpenWithOptions(_currentSelection.activePath.wstring());

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
                    is_cmd_app = openWithOption.isCommandLineApp;
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
                wss << L"*" << _currentSelection.activePath.extension();

                strcpy_s(name_buffer, 63, "New option");
                strcpy_s(pattern_buffer, 511, Convert(wss.str()).c_str());
                strcpy_s(command_buffer, 511, "");
                is_cmd_app = false;
                form_is_dirty = false;

                item_current_idx = _settingsService->AddOpenWithOption(
                    Convert(name_buffer),
                    wss.str(),
                    L"",
                    false);
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
        ImGui::TextWrapped("Add patterns to match this option to. Each line is test separately. Wildcards can be used like for example \"*.txt\".");

        if (ImGui::InputTextMultiline("command", command_buffer, 512))
        {
            form_is_dirty = true;
        }
        ImGui::TextWrapped("Use $fullpath, $filename, $extension or $parentpath in your command to be replaced with the path value the user is executing this action on.");

        if (ImGui::Checkbox("Run in command window", &is_cmd_app))
        {
            form_is_dirty = true;
        }

        if (form_is_dirty && ImGui::Button("Save"))
        {
            _settingsService->UpdateOpenWithOption(
                item_current_idx,
                Convert(name_buffer),
                Convert(pattern_buffer),
                Convert(command_buffer),
                is_cmd_app);

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

void OpenFolderWidget::ToggleShowInfo()
{
    _showInfo = !_showInfo;

    if (_showInfo)
    {
        _fileInfo.status = std::filesystem::status(_documentPath);
    }
}

void OpenFolderWidget::ToggleBookmark()
{
    _isBookmark = !_isBookmark;
    _settingsService->SetBookmarked(_documentPath, _isBookmark);
}

void OpenFolderWidget::MoveSelectionUp(
    int count)
{
    if (_itemsInFolder.empty())
    {
        return;
    }

    if (_currentSelection.activePath.empty())
    {
        _currentSelection.SetSelection(_itemsInFolder.front().path);

        return;
    }

    auto found = std::find_if(
        _itemsInFolder.begin(),
        _itemsInFolder.end(),
        [&](struct folderItem &item) {
            return item.path == _currentSelection.activePath;
        });

    if (found == _itemsInFolder.end() || found == _itemsInFolder.begin())
    {
        return;
    }

    for (int i = 0; i < count; i++)
    {
        --found;

        if (ImGui::GetIO().KeyShift)
        {
            _currentSelection.AddToSelection(found->path);
        }
        else
        {
            _currentSelection.SetSelection(found->path);
        }

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

    if (_currentSelection.activePath.empty())
    {
        _currentSelection.SetSelection(_itemsInFolder.front().path);

        return;
    }

    auto found = std::find_if(
        _itemsInFolder.begin(),
        _itemsInFolder.end(),
        [&](struct folderItem &item) {
            return item.path == _currentSelection.activePath;
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

        if (ImGui::GetIO().KeyShift)
        {
            _currentSelection.AddToSelection(found->path);
        }
        else
        {
            _currentSelection.SetSelection(found->path);
        }
    }
}

void OpenFolderWidget::MoveSelectionToEnd()
{
    if (_itemsInFolder.empty())
    {
        return;
    }

    MoveSelectionDown(_itemsInFolder.size());
}

void OpenFolderWidget::MoveSelectionToStart()
{
    if (_itemsInFolder.empty())
    {
        return;
    }

    MoveSelectionUp(_itemsInFolder.size());
}

void OpenFolderWidget::DeleteSelection()
{
    std::filesystem::remove(_currentSelection.activePath);

    Refresh();
}

void OpenFolderWidget::MoveSelectionToTrash()
{
#ifdef _WIN32
    recycle_file_folder(_currentSelection.activePath);
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
        ActivatePath(path, true);
    }
    else
    {
        Open(path);
        ActivatePath(path, false);
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
