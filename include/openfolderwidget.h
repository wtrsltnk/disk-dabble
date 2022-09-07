#ifndef OPENFOLDERWIDGET_H
#define OPENFOLDERWIDGET_H

#include <filesystem>
#include <functional>

struct folderItem
{
    std::filesystem::path path;
    std::wstring name;
    bool isDir;
};

class OpenFolderWidget
{
public:
    OpenFolderWidget(
        const std::filesystem::path &path,
        std::function<void(const std::filesystem::path &)> onActivatePath);

    void Render();

    const char *Id() const { return _openFolderPath.string().c_str(); }

private:
    std::function<void(const std::filesystem::path &)> _onActivatePath;
    std::filesystem::path _openFolderPath;
    std::filesystem::path _selectedSubPath;
    std::vector<struct folderItem> _itemsInFolder;
    std::vector<std::filesystem::path> _pathInSections;
    bool _showFind = false;
    std::wstring _findBuffer;

    void SetPath(
        const std::filesystem::path &path);
};

#endif // OPENFOLDERWIDGET_H
