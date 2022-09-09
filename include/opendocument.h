#ifndef OPENDOCUMENT_H
#define OPENDOCUMENT_H

#include <filesystem>

class OpenDocument
{
public:
    OpenDocument();

    void Render();

    void Open(
        const std::filesystem::path &path);

    const std::filesystem::path &DocumentPath() const { return _documentPath; }

    bool IsOpen() const { return _isOpen; }

protected:
    std::filesystem::path _documentPath;
    bool _isOpen = true;

    virtual void OnRender() = 0;

    virtual void OnPathChanged(
        const std::filesystem::path &oldPath) = 0;

    std::string ConstructWindowID();

    std::string Convert(
        const std::wstring &str);

    std::wstring Convert(
        const std::string &str);

private:
    std::filesystem::path _changeToPath;
};

#endif // OPENDOCUMENT_H
