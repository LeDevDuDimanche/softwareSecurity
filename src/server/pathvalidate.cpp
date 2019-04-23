#include <iostream>
#include <unistd.h>
#include <string>
#include <fstream>
#include <sys/stat.h>

#include <server/pathvalidate.hpp>

namespace pathvalidate
{
    bool exists (const std::string& name) {
        return ( access( name.c_str(), F_OK ) != -1 );
    }

    bool isFile(std::string filename) {
        struct stat st;
        if(stat(filename.c_str(), &st) != 0) {
            return false;
        }
        return S_ISREG(st.st_mode);
    }

    bool isDir(std::string dir) {
        struct stat st;
        std::cout << (stat(dir.c_str(), &st) != 0) << "Random bool " << std::endl;
        if(stat(dir.c_str(), &st) != 0) {
            return false;
        }
        return S_ISDIR(st.st_mode);
    }

    bool valid_relative_to_base(std::string base, std::string newPath) {
        return true;
    }


} // pathvalidate
