#include "openimagewidget.h"

#include <imgui.h>

OpenImageWidget::OpenImageWidget(
    const std::filesystem::path &path)
{
    SetPath(path);
}

void OpenImageWidget::SetPath(
    const std::filesystem::path &path)
{
    _openImagePath = path;
}

void OpenImageWidget::Render()
{
    ImGui::PushID(Id());
    ImGui::Begin("Open Image Widget");

    ImGui::End();
    ImGui::PopID();
}
