#ifndef OPENIMAGEWIDGET_H
#define OPENIMAGEWIDGET_H

#include "opendocument.h"
#include <filesystem>
#include <imgui.h>

class OpenImageWidget : public OpenDocument
{
public:
    OpenImageWidget();

    void OpenPreviousImageInParentDirectory();

    void OpenNextImageInParentDirectory();

    static bool IsImage(
        const std::filesystem::path &path);

protected:
    virtual void OnRender();

    virtual void OnPathChanged(
        const std::filesystem::path &oldPath);

private:
    unsigned int _textureId = 0;
    ImVec2 _textureSize;
    std::filesystem::path _prevImage, _nextImage;
    float _zoom = 1.0f;
    ImVec2 _pan;
};

#endif // OPENIMAGEWIDGET_H
