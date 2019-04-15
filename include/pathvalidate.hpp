#ifndef PATHVALIDATE_H
#define PATHVALIDATE_H
#include <iostream>
#include <unistd.h>
#include <string>
#include <fstream>
#include <sys/stat.h>


namespace pathvalidate
{
    bool exists (const std::string& path);

    //Will check if the newPath is within the specified base directory
    bool valid_relative_to_base(std::string base, std::string newPath);

    //Check if the specified dir is a directory
    bool isDir(std::string dir);

    //Check is if the filename is a file
    bool isFile(std::string filename);
} // pathvalidate
#endif  