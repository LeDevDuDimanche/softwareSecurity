#include <iostream>
#include <unistd.h>
#include <string>
#include <fstream>
#include <sys/stat.h>

#include <server/pathvalidate.hpp>

namespace pathvalidate
{
    bool exists (const std::string& name) {
        struct stat buffer;
        return (stat (name.c_str(), &buffer) == 0);
    }

    bool isFile(std::string filename) {
        struct stat buf;
        stat(filename.c_str(), &buf);
        return S_ISREG(buf.st_mode);
    }

    bool isDir(std::string dir) {
        struct stat buf;
        stat(dir.c_str(), &buf);
        return S_ISDIR(buf.st_mode);
    }

    bool valid_relative_to_base(std::string base, std::string newPath) {
        base = base;    // supress compiler warnings
        newPath = newPath;    // supress compiler warnings
        return true;
    }


} // pathvalidate
