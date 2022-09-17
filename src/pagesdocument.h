#ifndef PAGESDOCUMENT_H
#define PAGESDOCUMENT_H

#include <filesystem>
#include <memory>

class PagesDocument
{
public:
    PagesDocument();

    virtual ~PagesDocument();

    static std::unique_ptr<PagesDocument> Load(
        const std::filesystem::path &path);

protected:
};

#endif // PAGESDOCUMENT_H
