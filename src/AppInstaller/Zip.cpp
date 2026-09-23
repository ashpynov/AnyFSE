#include "Zip.hpp"
#include "Tools/miniz/miniz.h"
#include "Tools/Unicode.hpp"
#include <cstdio>
#include <filesystem>
#include <memory>
#include <stdexcept>

namespace AnyFSE::ToolsEx::Zip
{
    namespace fs = std::filesystem;

    class ZipReader
    {
    public:
        explicit ZipReader(const std::wstring &archivePath)
        {
            FILE *input = nullptr;
            if (_wfopen_s(&input, archivePath.c_str(), L"rb") != 0)
                throw std::runtime_error("Cannot open ZIP archive");
            inputFile.reset(input);
            if (!mz_zip_reader_init_cfile(&archive, inputFile.get(), 0, 0))
            {
                if (archive.m_pState) mz_zip_reader_end(&archive);
                throw std::runtime_error("Cannot initialize ZIP reader from file");
            }
        }

        explicit ZipReader(const void *data, size_t size)
        {
            if (!data || !size || !mz_zip_reader_init_mem(&archive, data, size, 0))
            {
                if (archive.m_pState) mz_zip_reader_end(&archive);
                throw std::runtime_error("Cannot initialize ZIP reader from memory");
            }
        }

        ~ZipReader() { mz_zip_reader_end(&archive); }

        ZipReader(const ZipReader &) = delete;
        ZipReader &operator=(const ZipReader &) = delete;

        bool ExtractArchive(const std::wstring &destination)
        {
            const fs::path root = fs::absolute(destination).lexically_normal();
            fs::create_directories(root);
            for (mz_uint index = 0; index < mz_zip_reader_get_num_files(&archive); ++index)
            {
                mz_zip_archive_file_stat stat{};
                if (!mz_zip_reader_file_stat(&archive, index, &stat) || !stat.m_is_supported) return false;
                const fs::path relative = EntryPath(index);
                if (relative.empty()) return false;
                const fs::path target = root / relative;
                fs::create_directories(stat.m_is_directory ? target : target.parent_path());
                if (stat.m_is_directory) continue;

                FILE *output = nullptr;
                if (_wfopen_s(&output, target.c_str(), L"wb") != 0) return false;
                std::unique_ptr<FILE, decltype(&std::fclose)> file(output, std::fclose);
                if (!mz_zip_reader_extract_to_cfile(&archive, index, file.get(), 0)) return false;
                if (std::fclose(file.release()) != 0) return false;
            }
            return true;
        }

    private:
        std::unique_ptr<FILE, decltype(&std::fclose)> inputFile{nullptr, std::fclose};
        mz_zip_archive archive{};

        fs::path EntryPath(mz_uint index)
        {
            const mz_uint count = mz_zip_reader_get_filename(&archive, index, nullptr, 0);
            if (count <= 1) return {};
            std::string name(count, '\0');
            if (mz_zip_reader_get_filename(&archive, index, name.data(), count) != count || name.find('\0') != count - 1) return {};
            name.pop_back();
            if (name.find(':') != std::string::npos) return {};

            const fs::path relative(Unicode::to_wstring(name));
            if (relative.empty() || relative.has_root_path()) return {};
            for (const auto &part : relative)
            {
                if (part == L"..") return {};
            }
            return relative;
        }
    };


    bool Extract(const std::wstring &archive, const std::wstring &destination)
    {
        try
        {
            return ZipReader(archive).ExtractArchive(destination);
        }
        catch (...) { return false; }
    }

    bool Extract(const void *data, size_t size, const std::wstring &destination)
    {
        try
        {
            return ZipReader(data, size).ExtractArchive(destination);
        }
        catch (...) { return false; }
    }
}
