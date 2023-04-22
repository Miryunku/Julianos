#include "pch.h"

#include "SHA.h"

#include <io.h>
#include <fcntl.h>

/*
bool is_valid_extension(const wchar_t* path)
{
    static const wchar_t* valid_extensions[2] = {L".wav", L".ogg"};
    wchar_t extension[MAX_PATH]; // Once I found an extension with 24 chars, so I'll just go with MAX_PATH.
    const wchar_t* dot = std::wcsrchr(path, L'.');
    if (!dot) { return false; }
    wcscpy_s(extension, MAX_PATH, dot);
    for (int32_t i = 0; i < std::wcslen(extension); ++i) { std::towlower(extension[i]); }

    for (int32_t i = 0; i < 2; ++i)
    {
        if (std::wcscmp(extension, valid_extensions[i]) == 0) { return true; }
    }
    return false;
}
*/

bool is_valid_extension(const wchar_t* path)
{
    static const wchar_t* valid_extensions[2] = {L".wav", L".ogg"};
    const wchar_t* dot = std::wcsrchr(path, L'.');
    if (!dot) { return false; }
    for (int32_t i = 0; i < 2; ++i)
    {
        if (_wcsnicmp(dot, valid_extensions[i], 4) == 0) { return true; }
    }
    return false;
}

int main()
{
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);

    std::wcout << L"Julianos (ver 1.0)\n\n"
        << L"Enter the absolute path of the folders that you want to merge.\n"
        << L"Enter \"merge\" to merge the folders.\n"
        << L"Enter \"exit\" to exit the program.\n"
        << L"Note 1: The first folder will be the one in where the others are merged to.\n"
        << L"Note 2: For better performance, you should process no more than 2000 song folders.\n\n";
    
    bool exit = false;
    wchar_t event_folder_path[MAX_PATH];
    std::vector<std::pair<std::array<uint32_t, 8>, std::wstring>> song_folders;
    song_folders.reserve(2000);
    while (!exit)
    {
        std::wcin.getline(event_folder_path, MAX_PATH);

        if (wcscmp(event_folder_path, L"exit") == 0)
        {
            exit = true;
            continue;
        }
        else if (wcscmp(event_folder_path, L"merge") == 0)
        {
            if (song_folders.size() < 2)
            {
                std::wcout << L"The merge operation couldn't be performed.\n";
                continue;
            }

            const std::wstring& first_directory = song_folders[0].second.substr(0, song_folders[0].second.find_last_of(L'\\'));
            for (const auto& song_folder : song_folders)
            {
                const std::wstring& dir = song_folder.second.substr(0, song_folder.second.find_last_of(L'\\'));
                if (first_directory.compare(dir) == 0) { continue; }

                wchar_t destination[MAX_PATH];
                wcscpy_s(destination, MAX_PATH, first_directory.data());
                wcscat_s(destination, MAX_PATH, L"\\");

                size_t offset = song_folder.second.find_last_of(L'\\') + 1;
                wcscat_s(destination, MAX_PATH, song_folder.second.data() + offset);

                MoveFileEx(song_folder.second.c_str(), destination, 0u);
            }

            song_folders.clear();
            std::wcout << L"\nThe merge operation was successful. You can continue using the program.\n\n";
            continue;
        }

        // If we are here, an event folder path was entered.
        wcscat_s(event_folder_path, MAX_PATH, L"\\*.*");
        WIN32_FIND_DATA ffd;
        HANDLE song_folder_file_handle = FindFirstFileEx(event_folder_path, FindExInfoBasic, &ffd, FindExSearchNameMatch, nullptr, 0);
        if (song_folder_file_handle == INVALID_HANDLE_VALUE)
        {
            std::wcout << L"\nInvalid folder path.\n";
            continue;
        }
        int32_t hashed_folders = 0;
        do
        {
            wchar_t song_folder_path[MAX_PATH];
            wcsncpy_s(song_folder_path, MAX_PATH, event_folder_path, wcslen(event_folder_path) - 3);
            wcscat_s(song_folder_path, MAX_PATH, ffd.cFileName);
            wcscat_s(song_folder_path, MAX_PATH, L"\\*.*");
            WIN32_FIND_DATA ffd2;
            HANDLE item_handle = FindFirstFileEx(song_folder_path, FindExInfoBasic, &ffd2, FindExSearchNameMatch, nullptr, 0);
            if (item_handle == INVALID_HANDLE_VALUE)
            {
                std::wcout << L"\nAn error ocurred with trying to access a child folder.\n";
                continue;
            }
            std::wstring str_for_hash;
            do
            {
                if (!is_valid_extension(ffd2.cFileName)) { continue; }

                wchar_t* dot = std::wcsrchr(ffd2.cFileName, L'.');
                std::wstring filename(ffd2.cFileName, dot - ffd2.cFileName);
                str_for_hash.append(filename);
            } while (FindNextFile(item_handle, &ffd2));
            if (GetLastError() != ERROR_NO_MORE_FILES)
            {
                std::wcout << L"\nAn error ocurred with traversing a child folder.\n";
                continue;
            }
            if (str_for_hash.empty()) { continue; }
            ++hashed_folders;
            std::wcout << L"\rHashed folders: " << hashed_folders;
            // Store the song folder hash and its path
            std::array<uint32_t, 8> hash = sha2_256(str_for_hash);
            bool already_in = false;
            for (const auto& folder : song_folders)
            {
                if (std::memcmp(hash.data(), folder.first.data(), 32) == 0)
                {
                    already_in = true;
                    break;
                }
            }
            if (already_in) { continue; }
            wchar_t* backslash = std::wcsrchr(song_folder_path, L'\\');
            song_folders.emplace_back(std::make_pair(hash, std::wstring(song_folder_path, backslash - song_folder_path)));
        } while (FindNextFile(song_folder_file_handle, &ffd));
        std::wcout << L'\n';
        if (GetLastError() != ERROR_NO_MORE_FILES)
        {
            std::wcout << L"\nRegistering the folder failed because an error ocurred while traversing the folder.\n";
            continue;
        }
    }
    return 0;
}
