#ifndef FILEMANAGER_H
#define FILEMANAGER_H


#include <fstream>
#include <iostream>
#include <sstream>

class FileManager
{
public:
    static void writeToFile(const char* path, const char* data){
        std::ofstream outFile(path);
        outFile << data;
        outFile.close();
    }
    static std::string readFile(const char* path){
        std::ifstream t(path);
        std::stringstream buffer;
        buffer << t.rdbuf();
        return buffer.str ();
    }
private:
    FileManager();
    ~FileManager() {} // Private destructor to prevent external deletion.
    FileManager(const FileManager&) = delete; // Disable copy constructor.
    FileManager& operator=(const FileManager&) = delete; // Disable copy assignment.
};


#endif // FILEMANAGER_H
