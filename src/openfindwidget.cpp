#include "openfindwidget.h"

#include <fmt/format.h>
#include <fstream>
#include <sstream>

OpenFindWidget::OpenFindWidget(
    int index,
    ServiceProvider *services,
    ImFont *monoSpaceFont)
    : OpenDocument(index, services),
      _monoSpaceFont(monoSpaceFont)
{
}

void OpenFindWidget::OnPathChanged(
    const std::filesystem::path &oldPath)
{
    _justChangedPath = true;
}

std::string OpenFindWidget::ConstructWindowID()
{
    std::wstringstream wss;
    wss << "Find in files from: ";
    if (!_documentPath.filename().empty())
    {
        wss << _documentPath.filename().wstring();
    }
    else
    {
        wss << _documentPath.root_path().wstring();
    }
    wss << L"###" << Id();

    return OpenDocument::Convert(wss.str());
}

void OpenFindWidget::OnRender()
{
    _linesToAddMutex.lock();
    auto content = _content.str();
    _linesToAddMutex.unlock();

    ImGui::Begin(ConstructWindowID().c_str(), &_isOpen);

    ImGui::PushFont(_monoSpaceFont);

    ImGui::Text("Find in files from: %s", Convert(_documentPath).c_str());

    if (_justChangedPath)
    {
        _justChangedPath = false;
        ImGui::SetKeyboardFocusHere(0);
    }
    ImGui::InputText("###searchFor", _buf, 256);

    ImGui::SameLine();

    auto enterPressed = ImGui::IsKeyPressed(ImGuiKey_Enter);

    if (ImGui::Button("Find") || enterPressed)
    {
        FinishThread();

        t1 = std::make_unique<std::thread>([this]() {
            StartFind(Convert(_buf), _documentPath);
        });
    }

    static std::string selection;
    struct Funcs
    {
        static int MyCallback(ImGuiInputTextCallbackData *data)
        {
            std::string &content = *(std::string *)data->UserData;

            size_t lineStart = data->SelectionStart;
            while (lineStart > 0 && content[lineStart] != '\n')
            {
                --lineStart;
            }
            ++lineStart; // skip the \n we found

            size_t lineEnd = data->SelectionStart;
            while (lineEnd < content.size() && content[lineEnd] != '\n')
            {
                ++lineEnd;
            }

            selection = content.substr(lineStart, lineEnd - lineStart);

            if (selection.size() == 0)
            {
                return 0;
            }

            return 0;
        }
    };

    ImGui::InputTextMultiline(
        "##SearchResults",
        content.data(),
        content.size(),
        ImGui::GetContentRegionAvail(),
        ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CallbackAlways,
        Funcs::MyCallback,
        (void *)&content);

    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        if (selection[0] != ' ' && std::filesystem::exists(selection))
        {
            ActivatePath(selection, true);
        }
    }

    ImGui::PopFont();

    ImGui::End();

    if (!_isOpen)
    {
        FinishThread();
    }
}

void OpenFindWidget::FinishThread()
{
    if (t1.get() != nullptr)
    {
        t1->join();
        t1 = nullptr;
    }
}

void OpenFindWidget::AddLine(
    const std::string &line)
{
    _linesToAddMutex.lock();

    _content << line << "\n";

    _linesToAddMutex.unlock();
}

void OpenFindWidget::RecursiveFind(
    const std::wstring &searchFor,
    const std::filesystem::path &path,
    int fileCount,
    int hitCount)
{
    for (auto const &dir_entry : std::filesystem::directory_iterator{path})
    {
        if (std::filesystem::is_directory(dir_entry))
        {
            RecursiveFind(searchFor, dir_entry.path(), fileCount, hitCount);
            continue;
        }

        fileCount++;

        bool fileHasResults = false;

        std::ifstream fileInput(dir_entry.path());
        std::string line;
        unsigned int curLine = 0;
        while (getline(fileInput, line))
        {
            curLine++;
            if (line.find(Convert(searchFor), 0) != std::string::npos)
            {
                hitCount++;
                if (!fileHasResults)
                {
                    AddLine(dir_entry.path().string());
                }
                AddLine(fmt::format(" {:>5} {}", curLine, line));
                fileHasResults = true;
            }
        }
        if (fileHasResults) AddLine("");
    }
}

void OpenFindWidget::StartFind(
    const std::wstring &searchFor,
    const std::filesystem::path &path)
{
    int fileCount = 0;
    int hitCount = 0;

    AddLine(fmt::format("Starting search for \"{}\" in files from : \"{}\"\n", Convert(searchFor), path.string()));

    RecursiveFind(searchFor, path, fileCount, hitCount);

    AddLine(fmt::format("Found \"{}\" {} times in {} files\n", Convert(searchFor), hitCount, fileCount));
}
