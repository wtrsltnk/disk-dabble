#ifndef OPENTEXTWIDGET_H
#define OPENTEXTWIDGET_H

#include "opendocument.h"
#include <ImGuiColorTextEdit/TextEditor.h>
#include <filesystem>
#include <imgui.h>

class OpenTextWidget : public OpenDocument
{
public:
    OpenTextWidget(
        ImFont *monoSpaceFont);

private:
    ImFont *_monoSpaceFont;
    TextEditor _textEditor;

    virtual void OnRender();

    virtual void OnPathChanged(
        const std::filesystem::path &oldPath);
};

#endif // OPENTEXTWIDGET_H
