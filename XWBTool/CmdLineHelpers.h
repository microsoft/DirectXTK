//--------------------------------------------------------------------------------------
// File: CmdLineHelpers.h
//
// Command-line tool shared functions
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#pragma once

#if __cplusplus < 201703L
#error Requires C++17 (and /Zc:__cplusplus with MSVC)
#endif

#include <algorithm>
#include <cstdio>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <list>
#include <memory>
#include <set>
#include <string>

#ifndef TOOL_VERSION
#error Define TOOL_VERSION before including this header
#endif


namespace Helpers
{
    struct handle_closer { void operator()(HANDLE h) { if (h) CloseHandle(h); } };

    using ScopedHandle = std::unique_ptr<void, handle_closer>;

    inline HANDLE safe_handle(HANDLE h) noexcept { return (h == INVALID_HANDLE_VALUE) ? nullptr : h; }

    struct find_closer { void operator()(HANDLE h) noexcept { assert(h != INVALID_HANDLE_VALUE); if (h) FindClose(h); } };

    using ScopedFindHandle = std::unique_ptr<void, find_closer>;

#ifdef _PREFAST_
#pragma prefast(disable : 26018, "Only used with static internal arrays")
#endif

    struct SConversion
    {
        std::wstring szSrc;
        std::wstring szFolder;
    };

    template<typename T>
    struct SValue
    {
        const wchar_t*  name;
        T               value;
    };

    template<typename T>
    T LookupByName(const wchar_t _In_z_ *pName, const SValue<T> *pArray)
    {
        while (pArray->name)
        {
            if (!_wcsicmp(pName, pArray->name))
                return pArray->value;

            pArray++;
        }

        return static_cast<T>(0);
    }

    template<typename T>
    const wchar_t* LookupByValue(T value, const SValue<T> *pArray)
    {
        while (pArray->name)
        {
            if (value == pArray->value)
                return pArray->name;

            pArray++;
        }

        return L"";
    }

    void PrintFormat(DXGI_FORMAT Format, const SValue<DXGI_FORMAT>* pFormatList)
    {
        for (auto pFormat = pFormatList; pFormat->name; pFormat++)
        {
            if (pFormat->value == Format)
            {
                wprintf(L"%ls", pFormat->name);
                return;
            }
        }

        wprintf(L"*UNKNOWN*");
    }

    void PrintFormat(DXGI_FORMAT Format, const SValue<DXGI_FORMAT>* pFormatList1, const SValue<DXGI_FORMAT>* pFormatList2)
    {
        for (auto pFormat = pFormatList1; pFormat->name; pFormat++)
        {
            if (pFormat->value == Format)
            {
                wprintf(L"%ls", pFormat->name);
                return;
            }
        }

        for (auto pFormat = pFormatList2; pFormat->name; pFormat++)
        {
            if (pFormat->value == Format)
            {
                wprintf(L"%ls", pFormat->name);
                return;
            }
        }

        wprintf(L"*UNKNOWN*");
    }

    template<typename T>
    void PrintList(size_t cch, const SValue<T> *pValue)
    {
        while (pValue->name)
        {
            const size_t cchName = wcslen(pValue->name);

            if (cch + cchName + 2 >= 80)
            {
                wprintf(L"\n      ");
                cch = 6;
            }

            wprintf(L"%ls ", pValue->name);
            cch += cchName + 2;
            pValue++;
        }

        wprintf(L"\n");
    }

    void PrintLogo(bool versionOnly, _In_z_ const wchar_t* name, _In_z_ const wchar_t* desc)
    {
        wchar_t version[32] = {};

        wchar_t appName[_MAX_PATH] = {};
        if (GetModuleFileNameW(nullptr, appName, _MAX_PATH))
        {
            const DWORD size = GetFileVersionInfoSizeW(appName, nullptr);
            if (size > 0)
            {
                auto verInfo = std::make_unique<uint8_t[]>(size);
                if (GetFileVersionInfoW(appName, 0, size, verInfo.get()))
                {
                    LPVOID lpstr = nullptr;
                    UINT strLen = 0;
                    if (VerQueryValueW(verInfo.get(), L"\\StringFileInfo\\040904B0\\ProductVersion", &lpstr, &strLen))
                    {
                        wcsncpy_s(version, reinterpret_cast<const wchar_t*>(lpstr), strLen);
                    }
                }
            }
        }

        if (!*version || wcscmp(version, L"1.0.0.0") == 0)
        {
            swprintf_s(version, L"%03d (library)", TOOL_VERSION);
        }

        if (versionOnly)
        {
            wprintf(L"%ls version %ls\n", name, version);
        }
        else
        {
            wprintf(L"%ls Version %ls\n", desc, version);
            wprintf(L"Copyright (C) Microsoft Corp.\n");
        #ifdef _DEBUG
            wprintf(L"*** Debug build ***\n");
        #endif
            wprintf(L"\n");
        }
    }

    void SearchForFiles(const std::filesystem::path& path, std::list<SConversion>& files, bool recursive, _In_opt_z_ const wchar_t* folder)
    {
        // Process files
        WIN32_FIND_DATAW findData = {};
        ScopedFindHandle hFile(safe_handle(FindFirstFileExW(path.c_str(),
            FindExInfoBasic, &findData,
            FindExSearchNameMatch, nullptr,
            FIND_FIRST_EX_LARGE_FETCH)));
        if (hFile)
        {
            for (;;)
            {
                if (!(findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY)))
                {
                    SConversion conv = {};
                    conv.szSrc = path.parent_path().append(findData.cFileName).native();
                    if (folder)
                    {
                        conv.szFolder = folder;
                    }
                    files.push_back(conv);
                }

                if (!FindNextFileW(hFile.get(), &findData))
                    break;
            }
        }

        // Process directories
        if (recursive)
        {
            auto searchDir = path.parent_path().append(L"*");

            hFile.reset(safe_handle(FindFirstFileExW(searchDir.c_str(),
                FindExInfoBasic, &findData,
                FindExSearchLimitToDirectories, nullptr,
                FIND_FIRST_EX_LARGE_FETCH)));
            if (!hFile)
                return;

            for (;;)
            {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (findData.cFileName[0] != L'.')
                    {
                        auto subfolder = (folder)
                            ? (std::wstring(folder) + std::wstring(findData.cFileName) + std::filesystem::path::preferred_separator)
                            : (std::wstring(findData.cFileName) + std::filesystem::path::preferred_separator);

                        auto subdir = path.parent_path().append(findData.cFileName).append(path.filename().c_str());

                        SearchForFiles(subdir, files, recursive, subfolder.c_str());
                    }
                }

                if (!FindNextFileW(hFile.get(), &findData))
                    break;
            }
        }
    }

    void ProcessFileList(std::wifstream& inFile, std::list<SConversion>& files)
    {
        std::list<SConversion> flist;
        std::set<std::wstring> excludes;

        for (;;)
        {
            std::wstring fname;
            std::getline(inFile, fname);
            if (!inFile)
                break;

            if (fname[0] == L'#')
            {
                // Comment
            }
            else if (fname[0] == L'-')
            {
                if (flist.empty())
                {
                    wprintf(L"WARNING: Ignoring the line '%ls' in -flist\n", fname.c_str());
                }
                else
                {
                    std::filesystem::path path(fname.c_str() + 1);
                    auto& npath = path.make_preferred();
                    if (wcspbrk(fname.c_str(), L"?*") != nullptr)
                    {
                        std::list<SConversion> removeFiles;
                        SearchForFiles(npath, removeFiles, false, nullptr);

                        for (auto& it : removeFiles)
                        {
                            std::wstring name = it.szSrc;
                            std::transform(name.begin(), name.end(), name.begin(), towlower);
                            excludes.insert(name);
                        }
                    }
                    else
                    {
                        std::wstring name = npath.c_str();
                        std::transform(name.begin(), name.end(), name.begin(), towlower);
                        excludes.insert(name);
                    }
                }
            }
            else if (wcspbrk(fname.c_str(), L"?*") != nullptr)
            {
                std::filesystem::path path(fname.c_str());
                SearchForFiles(path.make_preferred(), flist, false, nullptr);
            }
            else
            {
                SConversion conv = {};
                std::filesystem::path path(fname.c_str());
                conv.szSrc = path.make_preferred().native();
                flist.push_back(conv);
            }
        }

        inFile.close();

        if (!excludes.empty())
        {
            // Remove any excluded files
            for (auto it = flist.begin(); it != flist.end();)
            {
                std::wstring name = it->szSrc;
                std::transform(name.begin(), name.end(), name.begin(), towlower);
                auto item = it;
                ++it;
                if (excludes.find(name) != excludes.end())
                {
                    flist.erase(item);
                }
            }
        }

        if (flist.empty())
        {
            wprintf(L"WARNING: No file names found in -flist\n");
        }
        else
        {
            files.splice(files.end(), flist);
        }
    }

    const wchar_t* GetErrorDesc(HRESULT hr)
    {
        static wchar_t desc[1024] = {};

        LPWSTR errorText = nullptr;

        const DWORD result = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            nullptr, static_cast<DWORD>(hr),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&errorText), 0, nullptr);

        *desc = 0;

        if (result > 0 && errorText)
        {
            swprintf_s(desc, L": %ls", errorText);

            size_t len = wcslen(desc);
            if (len >= 1)
            {
                desc[len - 1] = 0;
            }

            if (errorText)
                LocalFree(errorText);

            for(wchar_t* ptr = desc; *ptr != 0; ++ptr)
            {
                if (*ptr == L'\r' || *ptr == L'\n')
                {
                    *ptr = L' ';
                }
            }
        }

        return desc;
    }
}
