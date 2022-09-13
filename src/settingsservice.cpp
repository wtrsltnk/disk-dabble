#include "settingsservice.h"

#include <filesystem>
#include <iostream>
#include <sstream>

SettingsService::SettingsService()
{
    auto userProfileDir = std::filesystem::path(getenv("USERPROFILE")) / "disk-dabble";

    if (!std::filesystem::exists(userProfileDir))
    {
        std::filesystem::create_directory(userProfileDir);
    }

    auto settingFilename = userProfileDir / "settings.sqlitedb";

    _db = std::make_unique<sqlitelib::Sqlite>(settingFilename.string().c_str());

    if (!_db->is_open())
    {
        _db = std::make_unique<sqlitelib::Sqlite>(":memory:");
    }

    if (_db->is_open())
    {
        EnsureTables();
    }

    auto query = LR"(SELECT b.path FROM Bookmarks b)";

    try
    {
        auto queryBytes = _wstringConverter.to_bytes(query);
        auto statement = _db->prepare<std::string>(queryBytes.c_str(), queryBytes.size());

        auto rows = statement.execute();

        for (const auto &row : rows)
        {
            auto path = _wstringConverter.from_bytes(row);
            _bookmarks.insert(std::filesystem::path(path));
        }
    }
    catch (std::exception &ex)
    {
        std::cout << _db->errmsg() << std::endl;
    }
}

void EnsureTable(
    std::unique_ptr<sqlitelib::Sqlite> &db,
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> &converter,
    const std::wstring &query)
{
    auto queryBytes = converter.to_bytes(query);
    db->execute(queryBytes.c_str(), queryBytes.size());
}

void SettingsService::EnsureTables()
{
    auto bookmarkQuery = std::wstring(LR"(CREATE TABLE IF NOT EXISTS Bookmarks (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        path TEXT NOT NULL
    );)");

    try
    {
        EnsureTable(_db, _wstringConverter, bookmarkQuery);
    }
    catch (std::exception &ex)
    {
        std::cout << ex.what() << std::endl;
    }

    auto openDocumentsQuery = std::wstring(LR"(CREATE TABLE IF NOT EXISTS OpenDocuments (
        id INTEGER PRIMARY KEY,
        path TEXT NOT NULL
    );)");

    try
    {
        EnsureTable(_db, _wstringConverter, openDocumentsQuery);
    }
    catch (std::exception &ex)
    {
        std::cout << ex.what() << std::endl;
    }

    auto openWithOptionsQuery = std::wstring(LR"(CREATE TABLE IF NOT EXISTS OpenWithOptions (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL,
        pattern TEXT NOT NULL,
        command TEXT NOT NULL
    );)");

    try
    {
        EnsureTable(_db, _wstringConverter, openWithOptionsQuery);
    }
    catch (std::exception &ex)
    {
        std::cout << ex.what() << std::endl;
    }
}

bool SettingsService::IsBookmarked(
    const std::filesystem::path &path)
{
    if (_bookmarks.find(path) != _bookmarks.end())
    {
        return true;
    }

    auto query = LR"(SELECT
    b.id,
    b.path
FROM
    Bookmarks b
WHERE
    b.path = ?)";

    try
    {
        auto queryBytes = _wstringConverter.to_bytes(query);
        auto statement = _db->prepare<int, std::string>(queryBytes.c_str(), queryBytes.size());

        auto rows = statement.execute(_wstringConverter.to_bytes(path.wstring()));

        if (!rows.empty())
        {
            _bookmarks.insert(path);

            return true;
        }
    }
    catch (std::exception &ex)
    {
        std::cout << _db->errmsg() << std::endl;
    }

    return false;
}

void SettingsService::SetBookmarked(
    const std::filesystem::path &path,
    bool isBookmarked)
{
    if (isBookmarked && !IsBookmarked(path))
    {
        _bookmarks.insert(path);

        auto query = LR"(INSERT INTO Bookmarks (path) VALUES (?))";

        auto queryBytes = _wstringConverter.to_bytes(query);

        try
        {
            _db->prepare<std::string>(queryBytes.c_str(), queryBytes.size())
                .execute(_wstringConverter.to_bytes(path.wstring()));
        }
        catch (std::exception &ex)
        {
            std::cout << _db->errmsg() << std::endl;
        }
    }
    else
    {
        _bookmarks.erase(path);

        auto query = LR"(DELETE FROM Bookmarks WHERE path = ?)";

        auto queryBytes = _wstringConverter.to_bytes(query);

        try
        {
            _db->prepare<std::string>(queryBytes.c_str(), queryBytes.size())
                .execute(_wstringConverter.to_bytes(path.wstring()));
        }
        catch (std::exception &ex)
        {
            std::cout << _db->errmsg() << std::endl;
        }
    }
}

std::map<int, std::filesystem::path> SettingsService::GetOpenFiles()
{
    auto query = LR"(SELECT
    d.id,
    d.path
FROM
    OpenDocuments d)";

    try
    {
        auto queryBytes = _wstringConverter.to_bytes(query);
        auto statement = _db->prepare<int, std::string>(queryBytes.c_str(), queryBytes.size());

        auto rows = statement.execute();

        if (!rows.empty())
        {
            std::map<int, std::filesystem::path> result;

            for (const auto &row : rows)
            {
                auto index = std::get<int>(row);
                auto path = _wstringConverter.from_bytes(std::get<std::string>(row));

                result.insert(std::make_pair(index, path));
            }

            return result;
        }
    }
    catch (std::exception &ex)
    {
        std::cout << _db->errmsg() << std::endl;
    }

    return std::map<int, std::filesystem::path>();
}

void SettingsService::SetOpenFiles(
    const std::map<int, std::filesystem::path> &openFiles)
{
    auto query = LR"(DELETE FROM OpenDocuments)";

    auto queryBytes = _wstringConverter.to_bytes(query);

    try
    {
        _db->prepare(queryBytes.c_str(), queryBytes.size())
            .execute();
    }
    catch (std::exception &ex)
    {
        std::cout << _db->errmsg() << std::endl;
    }

    for (const auto &openFile : openFiles)
    {
        auto query = LR"(INSERT INTO OpenDocuments (id, path) VALUES (?, ?))";

        auto queryBytes = _wstringConverter.to_bytes(query);

        try
        {
            _db->prepare<int, std::string>(queryBytes.c_str(), queryBytes.size())
                .execute(openFile.first, _wstringConverter.to_bytes(openFile.second.wstring()));
        }
        catch (std::exception &ex)
        {
            std::cout << _db->errmsg() << std::endl;
        }
    }
}
std::vector<OpenWithOption> SettingsService::GetOpenWithOptions(
    const std::wstring &extension)
{
    auto query = LR"(SELECT
    o.id,
    o.name,
    o.pattern,
    o.command
FROM
    OpenWithOptions o
WHERE
    o.pattern LIKE ?)";

    try
    {
        auto queryBytes = _wstringConverter.to_bytes(query);
        auto statement = _db->prepare<int, std::string, std::string, std::string>(queryBytes.c_str(), queryBytes.size());

        std ::wstringstream wss;
        wss << L"%" << extension << L"%";

        auto rows = statement.execute(_wstringConverter.to_bytes(wss.str()));

        if (!rows.empty())
        {
            std::vector<OpenWithOption> result;

            for (const auto &row : rows)
            {
                OpenWithOption option;

                option.id = std::get<0>(row);
                option.name = _wstringConverter.from_bytes(std::get<1>(row));
                option.extensionPatterns = _wstringConverter.from_bytes(std::get<2>(row));
                option.command = _wstringConverter.from_bytes(std::get<3>(row));

                result.push_back(option);
            }

            return result;
        }
    }
    catch (std::exception &ex)
    {
        std::cout << _db->errmsg() << std::endl;
    }

    return std::vector<OpenWithOption>();
}

int SettingsService::AddOpenWithOption(
    const std::wstring &name,
    const std::wstring &pattern,
    const std::wstring &command)
{
    auto query = LR"(INSERT INTO OpenWithOptions (name, pattern, command) VALUES (?, ?, ?))";

    auto queryBytes = _wstringConverter.to_bytes(query);

    try
    {
        _db->prepare(queryBytes.c_str(), queryBytes.size())
            .execute(
                _wstringConverter.to_bytes(name),
                _wstringConverter.to_bytes(pattern),
                _wstringConverter.to_bytes(command));

        queryBytes = _wstringConverter.to_bytes(L"SELECT last_insert_rowid()");

        return _db->prepare<int>(queryBytes.c_str(), queryBytes.size())
            .execute_value();
    }
    catch (std::exception &ex)
    {
        std::cout << _db->errmsg() << std::endl;

        return -1;
    }
}

void SettingsService::UpdateOpenWithOption(
    int id,
    const std::wstring &name,
    const std::wstring &pattern,
    const std::wstring &command)
{
    auto query = LR"(Update OpenWithOptions SET name = ?, pattern = ?, command = ? WHERE id = ?)";

    auto queryBytes = _wstringConverter.to_bytes(query);

    try
    {
        _db->prepare(queryBytes.c_str(), queryBytes.size())
            .execute(
                _wstringConverter.to_bytes(name),
                _wstringConverter.to_bytes(pattern),
                _wstringConverter.to_bytes(command),
                id);
    }
    catch (std::exception &ex)
    {
        std::cout << _db->errmsg() << std::endl;
    }
}

void SettingsService::DeleteOpenWithOption(
    int id)
{
    auto query = LR"(DELETE FROM OpenWithOptions WHERE id = ?)";

    auto queryBytes = _wstringConverter.to_bytes(query);

    try
    {
        _db->prepare(queryBytes.c_str(), queryBytes.size())
            .execute(
                id);
    }
    catch (std::exception &ex)
    {
        std::cout << _db->errmsg() << std::endl;
    }
}
