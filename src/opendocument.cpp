#include <opendocument.h>

#include <codecvt>
#include <locale>
#include <sstream>

OpenDocument::OpenDocument(
    int index,
    ServiceProvider *services)
    : _services(services),
      _index(NextIndex(index))
{}

int OpenDocument::NextIndex(
    int index)
{
    if (index >= 0)
    {
        _indexer = index + 1;
        return index;
    }

    return _indexer++;
}

int OpenDocument::_indexer = 0;

void OpenDocument::Render()
{
    _changeToPath = _documentPath;

    OnRender();

    if (_changeToPath != _documentPath)
    {
        auto oldPath = _documentPath;

        _documentPath = std::filesystem::canonical(_changeToPath);

        OnPathChanged(oldPath);
    }
}

void OpenDocument::Open(
    const std::filesystem::path &path)
{
    if (_documentPath.empty())
    {
        _documentPath = path;

        OnPathChanged(std::filesystem::path());
    }
    else
    {
        _changeToPath = path;
    }
}

static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

std::string OpenDocument::ConstructWindowID()
{
    std::wstringstream wss;
    if (!_documentPath.filename().empty())
    {
        wss << _documentPath.filename().wstring();
    }
    else
    {
        wss << _documentPath.root_path().wstring();
    }
    wss << L"###" << _index;

    return OpenDocument::Convert(wss.str());
}

std::string OpenDocument::Convert(
    const std::wstring &str)
{
    return converter.to_bytes(str);
}

std::wstring OpenDocument::Convert(
    const std::string &str)
{
    return converter.from_bytes(str);
}

void OpenDocument::RenderButton(
    const char *text,
    bool disabled,
    std::function<void()> action)
{
    StyleGuard style;
    if (disabled)
    {
        style.Push(ImGuiCol_Text, IM_COL32(0, 0, 0, 55));
    }
    if (ImGui::Button(text))
    {
        action();
    }
}

void OpenDocument::RenderYesNoDialog(
    bool &show,
    const std::wstring &caption,
    const std::wstring &text,
    std::function<void()> actionOnYes)
{
    if (show)
    {
        ImGui::OpenPopup(Convert(caption).c_str());
    }

    ImGui::SetNextWindowSize(ImVec2(250, 120));
    if (ImGui::BeginPopupModal(Convert(caption).c_str(), &show))
    {
        ImGui::Text("%s", Convert(text).c_str());
        if (ImGui::Button("Yes"))
        {
            show = false;
            actionOnYes();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();

        if (ImGui::Button("No"))
        {
            show = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
