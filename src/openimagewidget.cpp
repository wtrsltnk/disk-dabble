#include "openimagewidget.h"

#include <EXIF.H>
#include <IconsMaterialDesign.h>
#include <algorithm>
#include <cmath>
#include <glad/glad.h>
#include <imgui.h>
#include <iostream>
#include <stb_image.h>

OpenImageWidget::OpenImageWidget(
    int index,
    ServiceProvider *services)
    : OpenDocument(index, services)
{}

void OpenImageWidget::OnPathChanged(
    const std::filesystem::path &oldPath)
{
    _zoom = 1.0f;
    _pan = ImVec2();

    if (_textureId != 0)
    {
        glDeleteTextures(1, &_textureId);
        _textureId = 0;
    }

    int x, y, channels;
    auto imageData = stbi_load(_documentPath.string().c_str(), &x, &y, &channels, 4);
    if (imageData == nullptr)
    {
        return;
    }

    _textureSize.x = x;
    _textureSize.y = y;

    glGenTextures(1, &_textureId);
    glBindTexture(GL_TEXTURE_2D, _textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

    stbi_image_free(imageData);

    _prevImage.clear();
    _nextImage.clear();
    bool foundCurrentEntry = false;
    for (auto const &dir_entry : std::filesystem::directory_iterator{_documentPath.parent_path()})
    {
        if (!IsImage(dir_entry))
        {
            continue;
        }
        if (foundCurrentEntry)
        {
            _nextImage = dir_entry;
            break;
        }

        if (dir_entry == _documentPath)
        {
            foundCurrentEntry = true;
            continue;
        }

        _prevImage = dir_entry;
    }

    auto ext = _documentPath.extension().wstring();

    std::transform(
        ext.begin(),
        ext.end(),
        ext.begin(),
        [](unsigned char c) { return std::tolower(c); });

    if (ext == L".jpg" || ext == L".jpeg")
    {
        _rotate = 0.0f;

        auto file = fopen(_documentPath.string().c_str(), "rb");

        Cexif exif;
        if (exif.DecodeExif(file))
        {
            if (exif.m_exifinfo->Orientation == 1)
            {
                _rotate = 0.0;
            }
            else if (exif.m_exifinfo->Orientation == 8)
            {
                _rotate += -90.0 * 4.0 * atan(1.0) / 180.0;
            }
            else if (exif.m_exifinfo->Orientation == 3)
            {
                _rotate += 180.0 * 4.0 * atan(1.0) / 180.0;
            }
            else if (exif.m_exifinfo->Orientation == 6)
            {
                _rotate += 90.0 * 4.0 * atan(1.0) / 180.0;
            }
        }

        fclose(file);
    }
}

namespace ImGui
{
#include <math.h>
    static inline ImVec2 operator+(const ImVec2 &lhs, const ImVec2 &rhs)
    {
        return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
    }
    static inline ImVec2 ImRotate(const ImVec2 &v, float cos_a, float sin_a)
    {
        return ImVec2(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a);
    }
    void ImageRotated(ImTextureID tex_id, ImVec2 center, ImVec2 size, float angle)
    {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        float cos_a = cosf(angle);
        float sin_a = sinf(angle);
        ImVec2 pos[4] =
            {
                center + ImRotate(ImVec2(-size.x * 0.5f, -size.y * 0.5f), cos_a, sin_a),
                center + ImRotate(ImVec2(+size.x * 0.5f, -size.y * 0.5f), cos_a, sin_a),
                center + ImRotate(ImVec2(+size.x * 0.5f, +size.y * 0.5f), cos_a, sin_a),
                center + ImRotate(ImVec2(-size.x * 0.5f, +size.y * 0.5f), cos_a, sin_a)};
        ImVec2 uvs[4] =
            {
                ImVec2(0.0f, 0.0f),
                ImVec2(1.0f, 0.0f),
                ImVec2(1.0f, 1.0f),
                ImVec2(0.0f, 1.0f)};

        draw_list->AddImageQuad(tex_id, pos[0], pos[1], pos[2], pos[3], uvs[0], uvs[1], uvs[2], uvs[3], IM_COL32_WHITE);
    }
} // namespace ImGui

void OpenImageWidget::OnRender()
{
    const float buttonSize = 40.f;

    ImGui::Begin(ConstructWindowID().c_str(), &_isOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    auto available = ImGui::GetContentRegionAvail();

    available.x -= 2; // correct for the border

    ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
    ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
    ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
    ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white

    auto availableForImage = ImVec2(
        available.x,
        available.y - (buttonSize + ImGui::GetStyle().ItemSpacing.y + ImGui::GetStyle().ItemSpacing.y));

    auto scale = ImVec2(availableForImage.x / _textureSize.x, availableForImage.y / _textureSize.y);
    if (scale.x < scale.y)
    {
        scale.y = scale.x;
    }
    else
    {
        scale.x = scale.y;
    }

    scale.x *= _zoom;
    scale.y *= _zoom;

    auto spos = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton("###panning", availableForImage);

    if (ImGui::IsItemHovered())
    {
        _zoom += (ImGui::GetIO().MouseWheel / 10.0f);
        if (_zoom < 0.1f) _zoom = 0.1f;

        if (ImGui::IsKeyPressed(ImGuiKey_R))
        {
            _rotate += 90.0 * 4.0 * atan(1.0) / 180.0;
        }
    }
    if (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        _pan.x += ImGui::GetIO().MouseDelta.x;
        _pan.y += ImGui::GetIO().MouseDelta.y;
    }

    auto imageSize = ImVec2(
        scale.x * _textureSize.x,
        scale.y * _textureSize.y);

    auto imagePos = ImVec2(
        spos.x + _pan.x + (availableForImage.x - imageSize.x) / 2.0f,
        spos.y + _pan.y + (availableForImage.y - imageSize.y) / 2.0f);

    ImGui::SetCursorPos(imagePos);

    ImGui::ImageRotated(
        (void *)(intptr_t)(GLuint)_textureId,
        ImVec2(imagePos.x + (imageSize.x / 2.0f), imagePos.y + (imageSize.y / 2.0f)),
        imageSize,
        _rotate);

    ImGui::SetCursorPos(
        ImVec2(ImGui::GetStyle().ItemSpacing.y,
               available.y - ImGui::GetStyle().ItemSpacing.y));
    if (!_prevImage.empty())
    {
        if (ImGui::Button(ICON_MD_WEST, ImVec2(buttonSize, buttonSize)))
        {
            OpenPreviousImageInParentDirectory();
        }
    }
    else
    {
        ImGui::Text(ICON_MD_WEST);
    }

    ImGui::SetCursorPos(
        ImVec2(available.x / 2.0f,
               available.y - ImGui::GetStyle().ItemSpacing.y));

    if (ImGui::Button(ICON_MD_FULLSCREEN, ImVec2(buttonSize, buttonSize)))
    {
        _zoom *= 1.f / scale.x;
        _pan = ImVec2();
        _rotate = 0.0f;
    }

    ImGui::SetCursorPos(
        ImVec2(available.x / 2.0f - buttonSize,
               available.y - ImGui::GetStyle().ItemSpacing.y));

    if (ImGui::Button(ICON_MD_ROTATE_90_DEGREES_CW, ImVec2(buttonSize, buttonSize)))
    {
        _rotate += 90.0 * 4.0 * atan(1.0) / 180.0;
    }

    ImGui::SetCursorPos(
        ImVec2(available.x + ImGui::GetStyle().ItemSpacing.y - buttonSize,
               available.y - ImGui::GetStyle().ItemSpacing.y));
    if (!_nextImage.empty())
    {
        if (ImGui::Button(ICON_MD_EAST, ImVec2(buttonSize, buttonSize)))
        {
            OpenNextImageInParentDirectory();
        }
    }
    else
    {
        ImGui::Text(ICON_MD_EAST);
    }

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_DockHierarchy))
    {
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
        {
            OpenPreviousImageInParentDirectory();
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
        {
            OpenNextImageInParentDirectory();
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            _pan = ImVec2();
            _zoom = 1.0f;
        }
    }

    ImGui::End();
}

void OpenImageWidget::OpenPreviousImageInParentDirectory()
{
    if (_prevImage.empty())
    {
        return;
    }

    Open(_prevImage);
}

void OpenImageWidget::OpenNextImageInParentDirectory()
{
    if (_nextImage.empty())
    {
        return;
    }

    Open(_nextImage);
}
bool OpenImageWidget::IsImage(
    const std::filesystem::path &path)
{
    auto ext = path.extension().wstring();

    std::transform(
        ext.begin(),
        ext.end(),
        ext.begin(),
        [](unsigned char c) { return std::tolower(c); });

    if (ext == L".png") return true;
    if (ext == L".jpg") return true;
    if (ext == L".jpeg") return true;
    if (ext == L".bmp") return true;
    if (ext == L".gif") return true;
    if (ext == L".tga") return true;
    if (ext == L".pic") return true;
    if (ext == L".ppm") return true;
    if (ext == L".pgm") return true;

    return false;
}
