#ifndef OPENTEXTWIDGET_H
#define OPENTEXTWIDGET_H

#include "opendocument.h"
#include <filesystem>
#include <imgui.h>

class OpenTextWidget : public OpenDocument
{
public:
    OpenTextWidget(
        int index,
        ServiceProvider *services,
        ImFont *monoSpaceFont);

    void SaveFile();

    void SaveFileAs();

private:
    ImFont *_monoSpaceFont;
    ImVector<char> _content;
    bool _isDirty = false;

    virtual void OnRender();

    virtual void OnPathChanged(
        const std::filesystem::path &oldPath);
};

#endif // OPENTEXTWIDGET_H
