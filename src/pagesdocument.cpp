#include "pagesdocument.h"

#include <iostream>
#include <miniz.h>

PagesDocument::PagesDocument() = default;

PagesDocument::~PagesDocument() = default;

std::unique_ptr<PagesDocument> PagesDocument::Load(
    const std::filesystem::path &path)
{
    if (!std::filesystem::exists(path))
    {
        return nullptr;
    }

    mz_zip_archive zip_archive;

    memset(&zip_archive, 0, sizeof(zip_archive));
    auto status = mz_zip_reader_init_file_v2(&zip_archive, path.string().c_str(), 0, 0, 0);
    if (!status)
    {
        std::cerr << "Failed to open zip file" << std::endl;

        return nullptr;
    }

    mz_uint32 manifestIndex;
    if (mz_zip_reader_locate_file_v2(&zip_archive, "META-INF/manifest.xml", "test", 0, &manifestIndex))
    {
        std::cerr << "Could not find manifest" << std::endl;

        mz_zip_reader_end(&zip_archive);

        return nullptr;
    }

    mz_uint32 mimetypesIndex;
    if (mz_zip_reader_locate_file_v2(&zip_archive, "mimetype", "test", 0, &mimetypesIndex))
    {
        std::cerr << "Could not find mimetype" << std::endl;

        mz_zip_reader_end(&zip_archive);

        return nullptr;
    }

    auto result = std::make_unique<PagesDocument>();

    mz_zip_reader_end(&zip_archive);

    return std::move(result);
}
