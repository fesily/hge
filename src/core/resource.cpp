/*
** Haaf's Game Engine 1.8
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** Core functions implementation: resources management
*/


#include "hge_impl.h"

#define NOCRYPT
//#define NOUNCRYPT
#include <minizip\unzip.h>
#include "ResDecrypt.h"


bool HGE_CALL HGE_Impl::Resource_AttachPack(const char* filename, const char* password) {
    auto res_item = res_list_;

    const auto sz_name = Resource_MakePath(filename);
    strupr(sz_name);

    while (res_item) {
        if (!strcmp(sz_name, res_item->filename)) {
            return false;
        }
        res_item = res_item->next;
    }

    const auto zip = unzOpen(sz_name);
    if (!zip) {
        return false;
    }
    unzClose(zip);

    res_item = new CResourceList;
    strcpy(res_item->filename, sz_name);
    if (password) {
        strcpy(res_item->password, password);
    }
    else {
        res_item->password[0] = 0;
    }
    res_item->next = res_list_;
    res_list_ = res_item;

    return true;
}

void HGE_CALL HGE_Impl::Resource_RemovePack(const char* filename) {
    auto res_item = res_list_;
    CResourceList* res_prev = nullptr;

    const auto sz_name = Resource_MakePath(filename);
    strupr(sz_name);

    while (res_item) {
        if (!strcmp(sz_name, res_item->filename)) {
            if (res_prev) {
                res_prev->next = res_item->next;
            }
            else {
                res_list_ = res_item->next;
            }
            delete res_item;
            break;
        }

        res_prev = res_item;
        res_item = res_item->next;
    }
}

void HGE_CALL HGE_Impl::Resource_RemoveAllPacks() {
    auto res_item = res_list_;

    while (res_item) {
        const auto res_next_item = res_item->next;
        delete res_item;
        res_item = res_next_item;
    }

    res_list_ = nullptr;
}

void* HGE_CALL HGE_Impl::Resource_Load(const char* filename, uint32_t* size) {
    static char* res_err = "Can't load resource: %s";

    auto res_item = res_list_;
    char sz_name[_MAX_PATH];
    char sz_zip_name[_MAX_PATH];
    unz_file_info file_info;
    int i;
    void* ptr;

    if (filename[0] == '\\' || filename[0] == '/' || filename[1] == ':') {
        goto _fromfile; // skip absolute paths
    }

    // Load from pack

    strcpy(sz_name, filename);
    strupr(sz_name);
    for (i = 0; sz_name[i]; i++) {
        if (sz_name[i] == '/') {
            sz_name[i] = '\\';
        }
    }

    while (res_item) {
        const auto zip = unzOpen(res_item->filename);
        auto done = unzGoToFirstFile(zip);
        while (done == UNZ_OK) {
            unzGetCurrentFileInfo(zip, &file_info, sz_zip_name, sizeof(sz_zip_name), nullptr, 0,
                                  nullptr, 0);
            strupr(sz_zip_name);
            for (i = 0; sz_zip_name[i]; i++) {
                if (sz_zip_name[i] == '/') {
                    sz_zip_name[i] = '\\';
                }
            }
            if (!strcmp(sz_name, sz_zip_name)) {
                if (unzOpenCurrentFilePassword(zip, res_item->password[0] ? res_item->password : 0)
                    !=
                    UNZ_OK) {
                    unzClose(zip);
                    sprintf(sz_name, res_err, filename);
                    post_error(sz_name);
                    return nullptr;
                }

                ptr = malloc(file_info.uncompressed_size);
                if (!ptr) {
                    unzCloseCurrentFile(zip);
                    unzClose(zip);
                    sprintf(sz_name, res_err, filename);
                    post_error(sz_name);
                    return nullptr;
                }

                if (unzReadCurrentFile(zip, ptr, file_info.uncompressed_size) < 0) {
                    unzCloseCurrentFile(zip);
                    unzClose(zip);
                    free(ptr);
                    sprintf(sz_name, res_err, filename);
                    post_error(sz_name);
                    return nullptr;
                }
                unzCloseCurrentFile(zip);
                unzClose(zip);
                if (size) {
                    *size = file_info.uncompressed_size;
                }
                return ptr;
            }

            done = unzGoToNextFile(zip);
        }

        unzClose(zip);
        res_item = res_item->next;
    }

    // Load from file
_fromfile:
    const auto h_f = CreateFile(Resource_MakePath(filename), GENERIC_READ,
                                FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                                nullptr);
    if (h_f == INVALID_HANDLE_VALUE) {
        sprintf(sz_name, res_err, filename);
        post_error(sz_name);
        return nullptr;
    }
    file_info.uncompressed_size = GetFileSize(h_f, nullptr);
    ptr = malloc(file_info.uncompressed_size);
    if (!ptr) {
        CloseHandle(h_f);
        sprintf(sz_name, res_err, filename);
        post_error(sz_name);
        return nullptr;
    }
    if (ReadFile(h_f, ptr, file_info.uncompressed_size, &file_info.uncompressed_size,
                 nullptr) == 0) {
        CloseHandle(h_f);
        free(ptr);
        sprintf(sz_name, res_err, filename);
        post_error(sz_name);
        return nullptr;
    }
    // 资源解密
    if (is_res_encrypt_) DecryptData((BYTE*)ptr, file_info.uncompressed_size);

    CloseHandle(h_f);
    if (size) {
        *size = file_info.uncompressed_size;
    }
    return ptr;
}


void HGE_CALL HGE_Impl::Resource_Free(void* res) {
    if (res) {
        free(res);
    }
}


char* HGE_CALL HGE_Impl::Resource_MakePath(const char* filename) {
    if (!filename) {
        strcpy(tmp_filename_, app_path_);
    }
    else if (filename[0] == '\\' || filename[0] == '/' || filename[1] == ':') {
        strcpy(tmp_filename_, filename);
    }
    else {
        strcpy(tmp_filename_, app_path_);
        if (filename) {
            strcat(tmp_filename_, filename);
        }
    }

    for (auto i = 0; tmp_filename_[i]; i++) {
        if (tmp_filename_[i] == '/') {
            tmp_filename_[i] = '\\';
        }
    }
    return tmp_filename_;
}

char* HGE_CALL HGE_Impl::Resource_EnumFiles(const char* wildcard) {
    if (wildcard) {
        if (h_search_) {
            FindClose(h_search_);
            h_search_ = nullptr;
        }
        h_search_ = FindFirstFile(Resource_MakePath(wildcard), &search_data_);
        if (h_search_ == INVALID_HANDLE_VALUE) {
            h_search_ = nullptr;
            return nullptr;
        }

        if (!(search_data_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            return search_data_.cFileName;
        }
        return Resource_EnumFiles();
    }
    if (!h_search_) {
        return nullptr;
    }
    for (;;) {
        if (!FindNextFile(h_search_, &search_data_)) {
            FindClose(h_search_);
            h_search_ = nullptr;
            return nullptr;
        }
        if (!(search_data_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            return search_data_.cFileName;
        }
    }
}

char* HGE_CALL HGE_Impl::Resource_EnumFolders(const char* wildcard) {
    if (wildcard) {
        if (h_search_) {
            FindClose(h_search_);
            h_search_ = nullptr;
        }
        h_search_ = FindFirstFile(Resource_MakePath(wildcard), &search_data_);
        if (h_search_ == INVALID_HANDLE_VALUE) {
            h_search_ = nullptr;
            return nullptr;
        }

        if ((search_data_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            strcmp(search_data_.cFileName, ".") && strcmp(search_data_.cFileName, "..")) {
            return search_data_.cFileName;
        }
        return Resource_EnumFolders();
    }
    if (!h_search_) {
        return nullptr;
    }
    for (;;) {
        if (!FindNextFile(h_search_, &search_data_)) {
            FindClose(h_search_);
            h_search_ = nullptr;
            return nullptr;
        }
        if ((search_data_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            strcmp(search_data_.cFileName, ".") && strcmp(search_data_.cFileName, "..")) {
            return search_data_.cFileName;
        }
    }
}
