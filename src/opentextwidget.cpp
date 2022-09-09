#include "opentextwidget.h"

#include <fstream>
#include <imgui.h>

OpenTextWidget::OpenTextWidget(
    ImFont *monoSpaceFont)
    : _monoSpaceFont(monoSpaceFont)
{}

void OpenTextWidget::OnPathChanged(
    const std::filesystem::path &oldPath)
{
    std::ifstream t(_documentPath);

    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    _textEditor.SetText(str);
    _textEditor.SetPalette(_textEditor.GetLightPalette());
}

void OpenTextWidget::OnRender()
{
    ImGui::Begin(ConstructWindowID().c_str(), &_isOpen);

    ImGui::PushFont(_monoSpaceFont);

    _textEditor.Render("TextEditor");

    //    ImGui::Text(converter.to_bytes(_content).c_str());

    ImGui::PopFont();

    ImGui::End();
}
