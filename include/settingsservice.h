#ifndef SETTINGSSERVICE_H
#define SETTINGSSERVICE_H

#include <filesystem>
#include <map>
#include <set>
#include <sqlitelib.h>
#include <string>

#include <codecvt>
#include <locale>

class OpenWithOption
{
public:
    int id;
    std::wstring name;
    std::wstring extensionPatterns;
    std::wstring command;
};

class ISettingsService
{
public:
    virtual ~ISettingsService() = default;

    virtual bool IsBookmarked(
        const std::filesystem::path &path) = 0;

    virtual void SetBookmarked(
        const std::filesystem::path &path,
        bool isBookmarked) = 0;

    virtual const std::set<std::filesystem::path> &Bookmarks() const = 0;

    virtual std::map<int, std::filesystem::path> GetOpenFiles() = 0;

    virtual void SetOpenFiles(
        const std::map<int, std::filesystem::path> &openFiles) = 0;

    virtual std::vector<OpenWithOption> GetOpenWithOptions(
        const std::wstring &extension) = 0;

    virtual int AddOpenWithOption(
        const std::wstring &name,
        const std::wstring &pattern,
        const std::wstring &command) = 0;

    virtual void UpdateOpenWithOption(
        int id,
        const std::wstring &name,
        const std::wstring &pattern,
        const std::wstring &command) = 0;

    virtual void DeleteOpenWithOption(
        int id) = 0;
};

class SettingsService :
    public ISettingsService
{
public:
    SettingsService();

    virtual bool IsBookmarked(
        const std::filesystem::path &path);

    virtual void SetBookmarked(
        const std::filesystem::path &path,
        bool isBookmarked);

    virtual const std::set<std::filesystem::path> &Bookmarks() const { return _bookmarks; }

    virtual std::map<int, std::filesystem::path> GetOpenFiles();

    virtual void SetOpenFiles(
        const std::map<int, std::filesystem::path> &openFiles);

    virtual std::vector<OpenWithOption> GetOpenWithOptions(
        const std::wstring &extension);

    virtual int AddOpenWithOption(
        const std::wstring &name,
        const std::wstring &pattern,
        const std::wstring &command);

    virtual void UpdateOpenWithOption(
        int id,
        const std::wstring &name,
        const std::wstring &pattern,
        const std::wstring &command);

    virtual void DeleteOpenWithOption(
        int id);

private:
    std::unique_ptr<sqlitelib::Sqlite> _db;
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> _wstringConverter;

    void EnsureTables();
    std::set<std::filesystem::path> _bookmarks;
};

#endif // SETTINGSSERVICE_H
