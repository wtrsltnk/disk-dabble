#ifndef OPENFOLDERWIDGET_H
#define OPENFOLDERWIDGET_H

#include "opendocument.h"
#include <filesystem>
#include <functional>
#include <settingsservice.h>

struct folderItem
{
    std::filesystem::path path;
    std::wstring name;
    bool isDir;
};

class OpenFolderWidget : public OpenDocument
{
public:
    OpenFolderWidget(
        int index,
        ServiceProvider *services,
        std::function<void(const std::filesystem::path &)> onActivatePath);

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

protected:
    std::function<void(const std::filesystem::path &)> _onActivatePath;
    std::filesystem::path _activeSubPath;
    std::vector<struct folderItem> _itemsInFolder;
    std::vector<std::filesystem::path> _pathInSections;
    bool _isBookmark = false;
    IBookmarkService *_bookmarkService = nullptr;
    bool _showFind = false;
    bool _showInfo = false;
    struct
    {
        std::filesystem::file_status status;
    } _fileInfo;
    std::wstring _findBuffer;
    char _buffer[64] = {0};

    bool _pathChangeIsTravel = false;
    std::vector<std::filesystem::path> _travelledPaths;
    std::vector<std::filesystem::path> _pathsToTravel;

    virtual void OnRender();

    virtual void OnPathChanged(
        const std::filesystem::path &oldPath);
};

#endif // OPENFOLDERWIDGET_H
