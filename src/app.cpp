#include <app.hpp>
#include <fstream>
#include <glad/glad.h>
#include <imgui.h>
#include <iostream>
#include <sstream>

#include <IconsMaterialDesign.h>
#include <openfindwidget.h>
#include <openfolderwidget.h>
#include <openimagewidget.h>
#include <opentextwidget.h>
#include <process.hpp>

#define OPEN_FILES_FILENAME "open-files.txt"

struct ExampleAppLog
{
    ImGuiTextBuffer Buf;
    ImGuiTextFilter Filter;
    ImVector<int> LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
    bool AutoScroll;           // Keep scrolling if already at the bottom.

    ExampleAppLog()
    {
        AutoScroll = true;
        Clear();
    }

    void Clear()
    {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);
    }

    std::unique_ptr<std::thread> t1;

    void FinishThread()
    {
        if (t1.get() != nullptr)
        {
            t1->join();
            t1 = nullptr;
            this->exit_status = nullptr;
        }
    }

    std::unique_ptr<int> exit_status = nullptr;

    void StartProcess(
        const std::wstring &command,
        const std::wstring &path)
    {
        FinishThread();

        t1 = std::make_unique<std::thread>([this, command, path]() {
            this->exit_status = nullptr;

            TinyProcessLib::Process process1(command.c_str(), path.c_str(), [this](const char *bytes, size_t n) {
                this->AddLog("%s", std::string(bytes, n).c_str());
            });

            this->exit_status = std::make_unique<int>();
        });
    }

    void AddLog(const char *fmt, ...) IM_FMTARGS(2)
    {
        int old_size = Buf.size();
        va_list args;
        va_start(args, fmt);
        Buf.appendfv(fmt, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1);
    }

    void Draw(const char *title, bool *p_open = NULL)
    {
        if (!ImGui::Begin(title, p_open))
        {
            ImGui::End();
            return;
        }

        // Options menu
        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto-scroll", &AutoScroll);
            ImGui::EndPopup();
        }

        // Main window
        if (ImGui::Button("Options"))
            ImGui::OpenPopup("Options");
        ImGui::SameLine();
        bool clear = ImGui::Button("Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        Filter.Draw("Filter", -100.0f);

        ImGui::Separator();
        ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (clear)
            Clear();
        if (copy)
            ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char *buf = Buf.begin();
        const char *buf_end = Buf.end();
        if (Filter.IsActive())
        {
            // In this example we don't use the clipper when Filter is enabled.
            // This is because we don't have a random access on the result on our filter.
            // A real application processing logs with ten of thousands of entries may want to store the result of
            // search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
            for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
            {
                const char *line_start = buf + LineOffsets[line_no];
                const char *line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                if (Filter.PassFilter(line_start, line_end))
                    ImGui::TextUnformatted(line_start, line_end);
            }
        }
        else
        {
            // The simplest and easy way to display the entire buffer:
            //   ImGui::TextUnformatted(buf_begin, buf_end);
            // And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
            // to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
            // within the visible area.
            // If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
            // on your side is recommended. Using ImGuiListClipper requires
            // - A) random access into your data
            // - B) items all being the  same height,
            // both of which we can handle since we an array pointing to the beginning of each line of text.
            // When using the filter (in the block of code above) we don't have random access into the data to display
            // anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
            // it possible (and would be recommended if you want to search through tens of thousands of entries).
            ImGuiListClipper clipper;
            clipper.Begin(LineOffsets.Size);
            while (clipper.Step())
            {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    const char *line_start = buf + LineOffsets[line_no];
                    const char *line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                    ImGui::TextUnformatted(line_start, line_end);
                }
            }
            clipper.End();
        }
        ImGui::PopStyleVar();

        if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }
};

static ExampleAppLog AppLog;

void App::OnInit()
{
    _services.Add<ISettingsService *>(
        [&](ServiceProvider &sp) -> GenericServicePtr {
            return (GenericServicePtr)&_settingsService;
        });

    auto openFiles = _settingsService.GetOpenFiles();

    for (const auto &pair : openFiles)
    {
        if (!std::filesystem::exists(pair.second))
        {
            continue;
        }

        ActivatePath(pair.second, pair.first);
    }

    if (_openDocuments.empty() && _queuedDocuments.empty())
    {
        ActivatePath(std::filesystem::current_path());
    }

    glClearColor(0.56f, 0.6f, 0.58f, 1.0f);
}

void App::OnExit()
{
    std::map<int, std::filesystem::path> res;

    for (const auto &openDoc : _openDocuments)
    {
        if (dynamic_cast<OpenFindWidget *>(openDoc.get()) != nullptr)
        {
            continue;
        }

        res.insert(std::make_pair(openDoc->Id(), openDoc->DocumentPath()));
    }

    _settingsService.SetOpenFiles(res);

    AppLog.FinishThread();
}

bool replace(
    std::wstring &str,
    const std::wstring &from,
    const std::wstring &to)
{
    size_t start_pos = str.find(from);

    if (start_pos == std::wstring::npos)
    {
        return false;
    }

    str.replace(start_pos, from.length(), to);

    return true;
}

std::wstring PatchCommand(
    const std::filesystem::path &path,
    const std::wstring &command)
{
    std::wstring patchedCommand = command;

    replace(patchedCommand, L"$fullpath", path.wstring());
    replace(patchedCommand, L"$parentpath", path.parent_path().wstring());
    replace(patchedCommand, L"$filename", path.filename().wstring());

    return patchedCommand;
}

void ExecuteRunInCommand(
    const std::filesystem::path &path,
    const std::wstring &command)
{
    std::wstring patchedCommand = PatchCommand(path, command);

    std::wcout << L"Running file in:" << patchedCommand << std::endl;

    AppLog.AddLog("Starting command:\n");

    std::wstringstream wss;
    wss << L"cmd /C " << patchedCommand;

    AppLog.StartProcess(wss.str(), L"");
}

void ExecuteOpenWithCommand(
    const std::filesystem::path &path,
    const std::wstring &command)
{
    std::wstring patchedCommand = PatchCommand(path, command);

    std::wcout << L"Opening file with:" << patchedCommand << std::endl;

    system(OpenDocument::Convert(patchedCommand).c_str());
}

void App::ActivatePath(
    const std::filesystem::path &path,
    int index)
{
    if (std::filesystem::is_directory(path))
    {
        auto widget = std::make_unique<OpenFolderWidget>(
            index,
            &_services);

        widget->ActivatePath = [&](const std::filesystem::path &p, bool a) {
            _lastOpenedPath = p;

            if (std::filesystem::is_directory(p))
            {
                _lastOpenedDirectoryPath = p;
            }

            if (a)
            {
                this->ActivatePath(std::move(p));
            }
        };
        widget->ExecuteOpenWithCommand = ExecuteOpenWithCommand;
        widget->ExecuteRunInCommand = ExecuteRunInCommand;

        widget->Open(path);

        _queuedDocuments.push_back(std::move(widget));

        _lastOpenedPath = path;
    }
    else if (OpenImageWidget::IsImage(path))
    {
        auto widget = std::make_unique<OpenImageWidget>(
            index,
            &_services);

        widget->Open(path);

        _queuedDocuments.push_back(std::move(widget));
    }
    else if (path.extension() == L".exe" || path.extension() == L".bat" || path.extension() == L".cmd")
    {
        ExecuteRunInCommand(path, L"start $fullpath");
    }
    else
    {
        auto widget = std::make_unique<OpenTextWidget>(
            index,
            &_services,
            _monoSpaceFont);
        widget->Open(path);

        _queuedDocuments.push_back(std::move(widget));
    }
}

void App::OnResize(
    int width,
    int height)
{
    _width = width;
    _height = height;

    glViewport(0, 0, width, height);
}

void App::OnFrame()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto viewPort = ImGui::GetMainViewport();

    ImGui::ShowDemoWindow(nullptr);

    for (const auto &item : _openDocuments)
    {
        ImGui::SetNextWindowDockID(_dockId, ImGuiCond_FirstUseEver);
        item->Render();
    }

    ImGui::SetNextWindowDockID(_dockId, ImGuiCond_FirstUseEver);
    ImGui::PushFont(_monoSpaceFont);
    AppLog.Draw("Output");
    ImGui::PopFont();

    static bool open = true;
    if (ImGui::IsKeyPressed(ImGuiKey_LeftAlt))
    {
        if (!ImGui::IsPopupOpen("Save?"))
            ImGui::OpenPopup("Save?");
    }

    if (ImGui::IsKeyPressed(ImGuiKey_F3))
    {
        if (!_lastOpenedDirectoryPath.empty())
        {
            auto widget = std::make_unique<OpenFindWidget>(
                -1,
                &_services,
                _monoSpaceFont);

            widget->ActivatePath = [&](const std::filesystem::path &p, bool a) {
                _lastOpenedPath = p;

                if (std::filesystem::is_directory(p))
                {
                    _lastOpenedDirectoryPath = p;
                }

                if (a)
                {
                    this->ActivatePath(std::move(p));
                }
            };

            widget->Open(_lastOpenedDirectoryPath);

            _queuedDocuments.push_back(std::move(widget));
        }
    }

    ImGui::SetNextWindowPos(viewPort->Pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(250, _height));
    auto flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::BeginPopupModal("Save?", &open, flags))
    {
        auto buttonSize = ImVec2(250 - (2 * ImGui::GetStyle().ItemSpacing.x), 0);

        auto pos = ImGui::GetCursorPos();

        ImGui::PushFont(_largeFont);
        ImGui::Text("Disk Dabble");
        ImGui::PopFont();

        if (!_openDocuments.empty())
        {
            ImGui::SetCursorPos(ImVec2(buttonSize.x - 30, pos.y));
            if (ImGui::Button(ICON_MD_CLOSE))
            {
                ImGui::CloseCurrentPopup();
            }
        }

        if (ImGui::Button("Check TCC version", buttonSize))
        {
            AppLog.AddLog("Starting ping:\n\n");

            AppLog.StartProcess(L"cmd /C ping www.saaltink.net", L"");

            ImGui::CloseCurrentPopup();
        }

        if (ImGui::Button("Open current working directory", buttonSize))
        {
            ActivatePath(std::filesystem::current_path());
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Open user home directory", buttonSize))
        {
            ActivatePath(std::filesystem::path(getenv("USERPROFILE")));
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Open temp directory", buttonSize))
        {
            ActivatePath(std::filesystem::path(getenv("TEMP")));
            ImGui::CloseCurrentPopup();
        }

        if (!_settingsService.Bookmarks().empty())
        {
            ImGui::Separator();

            for (auto &bookmark : _settingsService.Bookmarks())
            {
                if (ImGui::Button(bookmark.filename().string().c_str(), buttonSize))
                {
                    ActivatePath(bookmark);
                    ImGui::CloseCurrentPopup();
                }
            }
        }

        ImGui::Separator();

        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    AddQueuedItems();
}

std::filesystem::path App::GetUserProfileDir()
{
    auto userProfileDir = std::filesystem::path(getenv("USERPROFILE")) / "disk-dabble";

    if (!std::filesystem::exists(userProfileDir))
    {
        std::filesystem::create_directory(userProfileDir);
    }

    return userProfileDir;
}

void App::AddQueuedItems()
{
    while (!_queuedDocuments.empty())
    {
        auto folder = std::move(_queuedDocuments.back());
        _queuedDocuments.pop_back();
        _openDocuments.push_back(std::move(folder));
    }

    while (true)
    {
        auto const &found = std::find_if(
            _openDocuments.begin(),
            _openDocuments.end(),
            [](std::unique_ptr<OpenDocument> &item) -> bool {
                return !item->IsOpen();
            });

        if (found == _openDocuments.cend())
        {
            break;
        }

        _openDocuments.erase(found);
    }

    if (_openDocuments.empty())
    {
        ImGui::OpenPopup("Save?");
    }
}
