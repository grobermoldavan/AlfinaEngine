
#include "platform_file_system_win32.h"

#include <assert.h>

namespace al
{
    [[nodiscard]] Result<void> platform_file_get_std_out(PlatformFile* file)
    {
        dbg (if (!file) return err<void>("Can't get std out - file is a nullptr."));

        file->mode = PlatformFileMode::WRITE;
        file->handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
        file->flags = PlatformFile::Flags::STD_IO;

        dbg (if (file->handle == INVALID_HANDLE_VALUE) return err<void>("Can't get std out - os call failed."));
        return ok();
    }

    [[nodiscard]] Result<void> platform_file_load(PlatformFile* file, const PlatformFilePath& path, PlatformFileMode loadMode)
    {
        dbg (if (!file) return err<void>("Can't load file - file is a nullptr."));

        auto toDesiredAccess = [](PlatformFileMode loadMode) -> DWORD
        {
            switch (loadMode)
            {
                case PlatformFileMode::READ: return GENERIC_READ;
                case PlatformFileMode::WRITE: return GENERIC_WRITE;
            }
            // @TODO : assert here
            return DWORD(0);
        };
        auto toShareMode = [](PlatformFileMode loadMode) -> DWORD
        {
            return FILE_SHARE_READ; // Allow file read for other processes
        };
        auto toCreationDisposition = [](PlatformFileMode loadMode) -> DWORD
        {
            switch (loadMode)
            {
                case PlatformFileMode::READ: return OPEN_EXISTING;
                case PlatformFileMode::WRITE: return CREATE_ALWAYS;
            }
            // @TODO : assert here
            return DWORD(0);
        };
        file->mode = loadMode;
        file->flags = 0;
        file->handle = ::CreateFileA
        (
            path.memory,
            toDesiredAccess(loadMode),
            toShareMode(loadMode),
            NULL,
            toCreationDisposition(loadMode),
            FILE_ATTRIBUTE_NORMAL /*| FILE_FLAG_WRITE_THROUGH // ? */,
            NULL
        );

        dbg (if (file->handle == INVALID_HANDLE_VALUE) return err<void>("Can't load file - os call failed."));
        file->flags = PlatformFile::Flags::IS_LOADED;
        return ok();
    }

    Result<void> platform_file_unload(PlatformFile* file)
    {
        dbg (if (!file)                                         return err<void>("Can't unload file - file is a nullptr."));
        dbg (if (file->handle == INVALID_HANDLE_VALUE)          return err<void>("Can't unload file - file handle is invalid."));
        dbg (if (file->flags & PlatformFile::Flags::STD_IO)     return err<void>("Can't unload file - it is an std io handle."));
        dbg (if (file->flags & PlatformFile::Flags::IS_LOADED)  return err<void>("Can't unload file - it is not loaded."));

        ::CloseHandle(file->handle);

        return ok();
    }

    [[nodiscard]] Result<bool> platform_file_is_valid(PlatformFile* file)
    {
        dbg (if (!file) return err<bool>("Can't check if file is valid - file is a nullptr."));
        return ok<bool>(file->handle != NULL && file->handle != INVALID_HANDLE_VALUE);
    }

    [[nodiscard]] Result<PlatformFileContent> platform_file_read(PlatformFile* file, AllocatorBindings* allocator)
    {
        dbg (if (!file)                                  return err<PlatformFileContent>("Can't read file - file is a nullptr."));
        dbg (if (!allocator)                             return err<PlatformFileContent>("Can't read file - allocator is a nullptr."));
        dbg (if (file->mode != PlatformFileMode::READ)   return err<PlatformFileContent>("Can't read file - file must be opended with PlatformFileMode::READ."));

        u64 fileSize = 0;
        {
            LARGE_INTEGER size = {};
            const BOOL result = ::GetFileSizeEx(file->handle, &size);
            if (!result) return err<PlatformFileContent>("Can't read file - os file size call failed.");
            fileSize = size.QuadPart;
        }
        void* buffer = allocate(allocator, fileSize + 1);
        {
            DWORD bytesRead = 0;
            const BOOL result = ::ReadFile
            (
                file->handle,
                buffer,
                DWORD(fileSize), // @TODO : safe cast
                &bytesRead,
                NULL
            );
            dbg(if (!result)                return err<PlatformFileContent>("Can't read file - os file read call failed."));
            dbg(if (bytesRead != fileSize)  return err<PlatformFileContent>("Can't read file - number of bytes read != file size bytes."));
        }
        ((u8*)buffer)[fileSize] = 0;

        return ok<PlatformFileContent>
        ({
            .allocator = *allocator,
            .memory = buffer,
            .sizeBytes = fileSize,
        });
    }

    Result<void> platform_file_free_content(PlatformFileContent* content)
    {
        dbg (if (!content)               return err<void>("Can't free file content - content is a nullptr."));
        dbg (if (!content->memory)       return err<void>("Can't free file content - content's memory is a nullptr."));
        dbg (if (!content->sizeBytes)    return err<void>("Can't free file content - content's size is zero."));

        deallocate(&content->allocator, content->memory, content->sizeBytes + 1);

        return ok();
    }

    [[nodiscard]] Result<void> platform_file_write(PlatformFile* file, const void* data, uSize dataSizeBytes)
    {
        dbg (if (!file)                                  return err<void>("Can't write file - file is a nullptr."));
        dbg (if (!data)                                  return err<void>("Can't write file - data is a nullptr."));
        dbg (if (!dataSizeBytes)                         return err<void>("Can't write file - data size is zero."));
        dbg (if (file->mode != PlatformFileMode::WRITE)  return err<void>("Can't write file - file must be opended with PlatformFileMode::WRITE."));

        DWORD bytesWritten;
        const BOOL result = ::WriteFile
        (
            file->handle,
            data,
            DWORD(dataSizeBytes), // @TODO : safe cast
            &bytesWritten,
            NULL
        );

        dbg (if (!result)                        return err<void>("Can't write file - os write file call failed."));
        dbg (if (bytesWritten != dataSizeBytes)  return err<void>("Can't write file - number of bytes written != dataSizeBytes."));
        return ok();
    }
}
