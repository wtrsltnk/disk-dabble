#include "opentextwidget.h"

#include <IconsMaterialDesign.h>
#include <fstream>
#include <imgui.h>

OpenTextWidget::OpenTextWidget(
    int index,
    ServiceProvider *services,
    ImFont *monoSpaceFont)
    : OpenDocument(index, services),
      _monoSpaceFont(monoSpaceFont)
{}

void OpenTextWidget::OnPathChanged(
    const std::filesystem::path &oldPath)
{
    std::ifstream t(_documentPath);

    auto str = std::string(
        (std::istreambuf_iterator<char>(t)),
        std::istreambuf_iterator<char>());

    _content.resize(str.size() + 1);
    memcpy(_content.Data, str.data(), str.size());
    _content.Data[str.size()] = 0;
}

void OpenTextWidget::SaveFile()
{
    std::ofstream t(_documentPath);

    t << _content.Data;

    _isDirty = false;
}

void OpenTextWidget::SaveFileAs()
{
}

void OpenTextWidget::OnRender()
{
    ImGui::Begin(ConstructWindowID().c_str(), &_isOpen);

    RenderButton(
        ICON_MD_SAVE,
        !_isDirty,
        [&]() { SaveFile(); });

    ImGui::SameLine();

    RenderButton(
        ICON_MD_SAVE_AS,
        false,
        [&]() { SaveFileAs(); });

    ImGui::PushFont(_monoSpaceFont);

    struct Funcs
    {
        static int MyResizeCallback(ImGuiInputTextCallbackData *data)
        {
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
            {
                ImVector<char> *my_str = (ImVector<char> *)data->UserData;
                IM_ASSERT(my_str->begin() == data->Buf);
                my_str->resize(data->BufSize); // NB: On resizing calls, generally data->BufSize == data->BufTextLen + 1
                data->Buf = my_str->begin();
            }
            return 0;
        }

        // Note: Because ImGui:: is a namespace you would typically add your own function into the namespace.
        // For example, you code may declare a function 'ImGui::InputText(const char* label, MyString* my_str)'
        static bool MyInputTextMultiline(const char *label, ImVector<char> *my_str, const ImVec2 &size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0)
        {
            IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
            return ImGui::InputTextMultiline(label, my_str->begin(), (size_t)my_str->size(), size, flags | ImGuiInputTextFlags_CallbackResize, Funcs::MyResizeCallback, (void *)my_str);
        }
    };

    if (_content.empty())
        _content.push_back(0);

    if (Funcs::MyInputTextMultiline(
            "##SearchResults",
            &_content,
            ImGui::GetContentRegionAvail()))
    {
        _isDirty = true;
    }

    ImGui::PopFont();

    ImGui::End();
}
