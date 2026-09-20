#include "Cabinet.hpp"
#include "App/Constants.hpp"
#include <windows.h>
#include <fdi.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <set>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <new>

#pragma comment(lib, "cabinet.lib")

namespace AnyFSE::ToolsEx::Cabinet
{
    namespace fs = std::filesystem;
    namespace c = App::Constants;

    namespace
    {
        struct Extraction
        {
            std::wstring archive;
            fs::path destination;
            std::set<int> outputFiles;
            const void *data = nullptr;
            size_t size = 0;

            ~Extraction()
            {
                for (int file : outputFiles) _close(file);
            }
        };

        // FDI's file-open callback has no user context. Each extraction runs synchronously on its thread.
        thread_local Extraction *current = nullptr;

        void *DIAMONDAPI Allocate(ULONG size) { return std::malloc(size); }
        void DIAMONDAPI Free(void *memory) { std::free(memory); }

        INT_PTR DIAMONDAPI Open(char *name, int flags, int)
        {
            if (!current || std::strcmp(name, c::CabinetStreamName) || (flags & (_O_WRONLY | _O_RDWR | _O_CREAT))) return -1;
            int file = -1;
            return _wsopen_s(&file, current->archive.c_str(), _O_RDONLY | _O_BINARY, _SH_DENYWR, 0) ? -1 : file;
        }

        UINT DIAMONDAPI Read(INT_PTR file, void *buffer, UINT size) { return _read(static_cast<int>(file), buffer, size); }
        UINT DIAMONDAPI Write(INT_PTR file, void *buffer, UINT size) { return _write(static_cast<int>(file), buffer, size); }
        int DIAMONDAPI Close(INT_PTR file)
        {
            if (current) current->outputFiles.erase(static_cast<int>(file));
            return _close(static_cast<int>(file));
        }
        long DIAMONDAPI Seek(INT_PTR file, long distance, int origin) { return _lseek(static_cast<int>(file), distance, origin); }

        struct MemoryFile
        {
            const unsigned char *data;
            size_t size;
            size_t position = 0;
        };

        INT_PTR DIAMONDAPI OpenMemory(char *name, int flags, int)
        {
            if (!current || std::strcmp(name, c::CabinetStreamName) || (flags & (_O_WRONLY | _O_RDWR | _O_CREAT))) return -1;
            auto file = new (std::nothrow) MemoryFile{static_cast<const unsigned char *>(current->data), current->size};
            return file ? reinterpret_cast<INT_PTR>(file) : -1;
        }

        UINT DIAMONDAPI ReadMemory(INT_PTR handle, void *buffer, UINT size)
        {
            auto &file = *reinterpret_cast<MemoryFile *>(handle);
            const size_t count = (std::min)(static_cast<size_t>(size), file.size - file.position);
            if (count) std::memcpy(buffer, file.data + file.position, count);
            file.position += count;
            return static_cast<UINT>(count);
        }

        int DIAMONDAPI CloseMemory(INT_PTR handle)
        {
            // On an extraction error FDI closes the unfinished output through this callback too.
            if (current && handle >= 0 && handle <= (std::numeric_limits<int>::max)()
                && current->outputFiles.count(static_cast<int>(handle))) return Close(handle);
            delete reinterpret_cast<MemoryFile *>(handle);
            return 0;
        }

        long DIAMONDAPI SeekMemory(INT_PTR handle, long distance, int origin)
        {
            auto &file = *reinterpret_cast<MemoryFile *>(handle);
            int64_t base = 0;
            switch (origin)
            {
                case SEEK_SET: break;
                case SEEK_CUR: base = file.position; break;
                case SEEK_END: base = file.size; break;
                default: return -1;
            }
            const int64_t position = base + distance;
            if (position < 0 || static_cast<uint64_t>(position) > file.size) return -1;
            file.position = static_cast<size_t>(position);
            return static_cast<long>(position);
        }

        bool IsPlainDirectory(const fs::path &path)
        {
            const DWORD attributes = GetFileAttributesW(path.c_str());
            return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)
                && !(attributes & FILE_ATTRIBUTE_REPARSE_POINT);
        }

        fs::path EntryPath(const char *name, bool utf8)
        {
            const UINT codePage = utf8 ? CP_UTF8 : CP_ACP;
            const int size = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, name, -1, nullptr, 0);
            if (!size) return {};
            std::wstring text(size, L'\0');
            if (!MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, name, -1, text.data(), size)) return {};
            text.pop_back();
            if (text.empty() || text.find_first_of(L":*?\"<>|") != std::wstring::npos) return {};
            fs::path relative(text);
            if (relative.has_root_path()) return {};
            for (const auto &part : relative)
            {
                const auto component = part.wstring();
                if (component.empty() || component == L"." || component == L".." || component.back() == L'.' || component.back() == L' ')
                    return {};
            }
            return relative;
        }

        INT_PTR DIAMONDAPI Notify(FDINOTIFICATIONTYPE type, PFDINOTIFICATION notification)
        {
            auto &context = *static_cast<Extraction *>(notification->pv);
            try
            {
                if (type == fdintNEXT_CABINET || type == fdintPARTIAL_FILE) return -1;
                if (type == fdintCOPY_FILE)
                {
                    const auto relative = EntryPath(notification->psz1, (notification->attribs & _A_NAME_IS_UTF) != 0);
                    if (relative.empty()) return -1;
                    auto parent = context.destination;
                    for (const auto &part : relative.parent_path())
                    {
                        parent /= part;
                        fs::create_directory(parent);
                        if (!IsPlainDirectory(parent)) return -1;
                    }

                    // Never overwrite existing files, symlinks, or a second entry with the same name.
                    const auto target = context.destination / relative;
                    HANDLE handle = CreateFileW(target.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
                    if (handle == INVALID_HANDLE_VALUE) return -1;
                    if (GetFileType(handle) != FILE_TYPE_DISK) { CloseHandle(handle); return -1; }
                    const int file = _open_osfhandle(reinterpret_cast<intptr_t>(handle), _O_WRONLY | _O_BINARY);
                    if (file == -1) { CloseHandle(handle); return -1; }
                    try { context.outputFiles.insert(file); }
                    catch (...) { _close(file); throw; }
                    return file;
                }
                if (type == fdintCLOSE_FILE_INFO)
                {
                    const int file = static_cast<int>(notification->hf);
                    const bool success = _close(file) == 0;
                    context.outputFiles.erase(file);
                    return success ? TRUE : FALSE;
                }
                return 0;
            }
            catch (...)
            {
                // Exceptions must not cross the Cabinet API callback boundary.
                return -1;
            }
        }
    }

    static bool ExtractContext(Extraction &context)
    {
        fs::create_directories(context.destination);
        for (auto path = context.destination; !path.empty(); path = path.parent_path())
        {
            if (!IsPlainDirectory(path)) return false;
            if (path == path.parent_path()) break;
        }

        ERF error = {};
        const bool memory = context.data != nullptr;
        HFDI fdi = FDICreate(Allocate, Free, memory ? OpenMemory : Open, memory ? ReadMemory : Read, Write,
            memory ? CloseMemory : Close, memory ? SeekMemory : Seek, cpuUNKNOWN, &error);
        if (!fdi) return false;
        auto previous = current;
        current = &context;
        char name[sizeof(c::CabinetStreamName)];
        std::memcpy(name, c::CabinetStreamName, sizeof(name));
        char path[] = "";
        const BOOL result = FDICopy(fdi, name, path, 0, Notify, nullptr, &context);
        FDIDestroy(fdi);
        current = previous;
        return result != FALSE;
    }

    bool Extract(const std::wstring &archive, const std::wstring &destination)
    {
        Extraction context{archive, fs::absolute(destination), {}};
        return ExtractContext(context);
    }

    bool Extract(const void *data, size_t size, const std::wstring &destination)
    {
        // FDI uses signed 32-bit offsets for seeking within a cabinet.
        if (!data || !size || size > static_cast<size_t>((std::numeric_limits<long>::max)())) return false;
        Extraction context{{}, fs::absolute(destination), {}, data, size};
        return ExtractContext(context);
    }
}
