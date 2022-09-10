#ifndef BOOKMARKSERVICE_H
#define BOOKMARKSERVICE_H

#include <filesystem>
#include <set>
#include <sqlitelib.h>

#include <codecvt>
#include <locale>

#include <map>

class IBookmarkService
{
public:
    virtual ~IBookmarkService() = default;

    virtual bool IsBookmarked(
        const std::filesystem::path &path) = 0;

    virtual void SetBookmarked(
        const std::filesystem::path &path,
        bool isBookmarked) = 0;
};

class ISettingsService
{
public:
    virtual std::map<int, std::filesystem::path> GetOpenFiles() = 0;

    virtual void SetOpenFiles(
        const std::map<int, std::filesystem::path> &openFiles) = 0;
};

class BookmarkService :
    public IBookmarkService,
    public ISettingsService
{
public:
    BookmarkService();

    virtual bool IsBookmarked(
        const std::filesystem::path &path);

    virtual void SetBookmarked(
        const std::filesystem::path &path,
        bool isBookmarked);

    const std::set<std::filesystem::path> &Bookmarks() const { return _bookmarks; }

    virtual std::map<int, std::filesystem::path> GetOpenFiles();

    virtual void SetOpenFiles(
        const std::map<int, std::filesystem::path> &openFiles);

private:
    std::unique_ptr<sqlitelib::Sqlite> _db;
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> _wstringConverter;

    void EnsureTables();
    std::set<std::filesystem::path> _bookmarks;
};

#endif // BOOKMARKSERVICE_H
