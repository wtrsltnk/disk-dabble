#ifndef OPENFOLDERWIDGET_H
#define OPENFOLDERWIDGET_H

#include "opendocument.h"
#include <filesystem>
#include <functional>
#include <set>
#include <settingsservice.h>

struct folderItem
{
    std::filesystem::path path;
    std::wstring name;
    bool isDir;
};

class SelectionState
{
public:
    std::filesystem::path activePath;
    std::set<std::filesystem::path> selectedPaths;

    void Clear();

    void SetSelection(
        const std::filesystem::path &path);

    void AddToSelection(
        const std::filesystem::path &path);

    bool IsSelected(
        const std::filesystem::path &path);

    void ToggleActivePathSelection();
};

class OpenFolderWidget : public OpenDocument
{
public:
    OpenFolderWidget(
        int index,
        ServiceProvider *services);

    void MoveSelectionUp(
        int count = 1);

    void MoveSelectionDown(
        int count = 1);

    void MoveSelectionToEnd();

    void MoveSelectionToStart();

    void OpenParentDirectory();

    void ActivateItem(
        const std::filesystem::path &path);

    void TravelBack();

    void TravelForward();

    void ToggleShowInfo();

    void ToggleBookmark();

    void DeleteSelection();

    void MoveSelectionToTrash();

    std::function<void(const std::filesystem::path &, bool)> ActivatePath;
    std::function<void(const std::filesystem::path &, const std::wstring &)> ExecuteOpenWithCommand;
    std::function<void(const std::filesystem::path &, const std::wstring &, std::function<void()> done)> ExecuteRunInCommand;

protected:
    SelectionState _currentSelection;
    std::vector<struct folderItem> _itemsInFolder;
    std::vector<std::filesystem::path> _pathInSections;
    bool _isBookmark = false;
    ISettingsService *_settingsService = nullptr;
    bool _showFind = false;
    bool _showInfo = false;
    bool _showDeletePopup = false;
    bool _showMoveToTrashPopup = false;
    bool _showManageOpenWithOptionsPopup = false;
    struct
    {
        std::filesystem::file_status status;
    } _fileInfo;
    std::wstring _findBuffer;
    char _buffer[64] = {0};

    std::vector<std::filesystem::path> _pathsToCopy;
    std::vector<std::filesystem::path> _pathsToMove;

    bool _pathChangeIsTravel = false;
    std::vector<std::filesystem::path> _travelledPaths;
    std::vector<std::filesystem::path> _pathsToTravel;

    virtual void OnRender();

    void RenderPathItemContextMenu(
        const std::filesystem::path &file);

    void RenderOpenWithoptionsDialog();

    void Refresh(
        const std::filesystem::path &oldPath = std::filesystem::path());

    void Paste(
        const std::vector<std::filesystem::path> &files,
        bool move);

    virtual void OnPathChanged(
        const std::filesystem::path &oldPath);
};

#endif // OPENFOLDERWIDGET_H
