#ifndef OPENDOCUMENT_H
#define OPENDOCUMENT_H

#include <filesystem>
#include <serviceprovider.h>
#include <imgui.h>

class StyleGuard
{
public:
    virtual ~StyleGuard()
    {
        ImGui::PopStyleColor(_stylesToPop);
    }

    void Push(
        ImGuiCol idx,
        ImU32 col)
    {
        ImGui::PushStyleColor(idx, col);
        _stylesToPop++;
    }

private:
    int _stylesToPop = 0;
};

class OpenDocument
{
public:
    void Render();

    void Open(
        const std::filesystem::path &path);

    const std::filesystem::path &DocumentPath() const { return _documentPath; }

    bool IsOpen() const { return _isOpen; }

    int Id() const { return _index; }

    static std::string Convert(
        const std::wstring &str);

    static std::wstring Convert(
        const std::string &str);

    static void RenderButton(
        const char *text,
        bool disabled,
        std::function<void()> action);

    static void RenderYesNoDialog(
        bool &show,
        const std::wstring &caption,
        const std::wstring &text,
        std::function<void()> actionOnYes);

protected:
    OpenDocument(
        int index,
        ServiceProvider *services);

    std::filesystem::path _documentPath;
    bool _isOpen = true;
    ServiceProvider *_services;

    virtual void OnRender() = 0;

    virtual void OnPathChanged(
        const std::filesystem::path &oldPath) = 0;

    virtual std::string ConstructWindowID();

private:
    std::filesystem::path _changeToPath;
    int _index = 0;

    static int _indexer;

    int NextIndex(
        int index);
};

#endif // OPENDOCUMENT_H
