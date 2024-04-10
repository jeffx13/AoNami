#include "CLI11.hpp"

#include <filesystem>
#include <iostream>
#include <regex>
#include <windows.h>
void open(std::string args = "")
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    auto commandline = "kyokou.exe "+ args;

    TCHAR commandLine[commandline.length ()+1];
    strcpy(commandLine, commandline.c_str());
//    printf(commandLine);

    if (!CreateProcess(NULL, // No module name (use command line)
                       commandLine, // Command line
                       NULL, // Process handle not inheritable
                       NULL, // Thread handle not inheritable
                       FALSE, // Set handle inheritance to FALSE
                       0, // No creation flags
                       NULL, // Use parent's environment block
                       NULL, // Use parent's starting directory
                       &si, // Pointer to STARTUPINFO structure
                       &pi) // Pointer to PROCESS_INFORMATION structure
        )
    {
        printf("CreateProcess failed (%d).\n", GetLastError());
        return;
    }

    // Wait until child process exits.
    WaitForSingleObject( pi.hProcess, INFINITE );

    // Close process and thread handles.
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
}

bool isValidFile(const std::filesystem::path& path){
    std::set<std::string> validExtensions{".mp4", ".mkv", ".avi", ".mp3", ".flac", ".wav", ".ogg", ".webm"};
    if (!validExtensions.count(path.extension().string())){
        printf("File %s does not have a valid extension",path.c_str ());
        return false;
    }
    return true;
}

std::string cleanPath(std::string& pathStr,bool& isDir){
    std::filesystem::path path = std::filesystem::path(pathStr).make_preferred ();
    if(path.is_relative ()) //handles relative path
        path = (std::filesystem::current_path()/std::filesystem::path(path));
    if (!std::filesystem::exists(path))
    {std::cout << "Invalid path" << path << std::endl; return "";}
    isDir = std::filesystem::is_directory(path);
    if(!isDir && !isValidFile(path))return "";
    return pathStr;
}

int main(int argc, char *argv[]){
    if(argc == 1){
        open();
        return 0;
    }else if(argc != 2){
        std::cout << "Usage: -d/--dir path/to/directory/to/load\n"
                  << "       -p/--play path/to/file/to/play\n";
        return -1;
    }
    std::string path = argv[1];
    if(path.starts_with ("http")){     // is an url
        open("--play " + path);
        return 0;
    }

    bool isDir = false;
    std::string cleanedPath = cleanPath(path,isDir);
    cleanedPath = "\"" + cleanedPath + "\"";
    if(cleanedPath.empty ())
        return -1;
    if (isDir) {
        printf("Opening directory %s\n",cleanedPath.c_str ());
        open("--dir " + cleanedPath);
    }else{
        printf("Opening file %s\n",cleanedPath.c_str ());
        open("--play " + cleanedPath);
    }
    return -1;
}
