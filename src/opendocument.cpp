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

    return converter.to_bytes(wss.str());
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
