#ifndef OPENIMAGEWIDGET_H
#define OPENIMAGEWIDGET_H

#include <filesystem>

class OpenImageWidget
{
public:
    OpenImageWidget(
        const std::filesystem::path &path);

    void Render();

    const char *Id() const { return _openImagePath.string().c_str(); }

private:
    std::filesystem::path _openImagePath;

    void SetPath(
        const std::filesystem::path &path);
};

#endif // OPENIMAGEWIDGET_H
