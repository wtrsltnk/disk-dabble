#ifndef OPENFINDWIDGET_H
#define OPENFINDWIDGET_H

#include "opendocument.h"
#include <TextEditor.h>
#include <imgui.h>
#include <memory>
#include <mutex>
#include <thread>
#include <sstream>

class OpenFindWidget : public OpenDocument
{
public:
    OpenFindWidget(
        int index,
        ServiceProvider *services,
        ImFont *monoSpaceFon);

    std::function<void(const std::filesystem::path &, bool)> ActivatePath;
protected:
    ImFont *_monoSpaceFont;
    TextEditor _textEditor;

    virtual void OnRender();

    virtual void OnPathChanged(
        const std::filesystem::path &oldPath);

    virtual std::string ConstructWindowID();

private:
    char _buf[256] = {0};
    std::unique_ptr<std::thread> t1;
    std::mutex _linesToAddMutex;
    std::stringstream _content;

    void AddLine(
        const std::string &line);

    void FinishThread();

    void StartFind(
        const std::wstring &searchFor,
        const std::filesystem::path &path);

    void RecursiveFind(
        const std::wstring &searchFor,
        const std::filesystem::path &path,
        int fileCount,
        int hitCount);
};

#endif // OPENFINDWIDGET_H
