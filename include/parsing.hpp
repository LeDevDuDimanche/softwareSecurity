#ifndef PARSING_HPP
#define PARSING_HPP
#include <iostream>
#include <vector> 
namespace Parsing
{
    class BadPathException
    {
    private:
        std::string desc;
    public:
        BadPathException(std::string desc);
        ~BadPathException();
        std::string getDesc();
    };
    
    std::string format(std::string str);
    std::vector<std::string> split_string(std::string s, char delim);
    std::string join_vector(std::vector<std::string> vec, std::string join);
    std::string resolve_path(std::string base, std::string currentDir, std::string path);
    std::string get_relative_path(std::string base, std::string fp);
    bool isPrintable(char ch);

    //To split and join a filepath
    const char slash = '/';
    const char space = ' ';
    const char delimiter = '\0';
    const char new_line_char = '\n';
    //A constant to represent when go to parent directory
    const std::string parent_dir = "..";
    //A constant to represent this directory
    const std::string FormatCharacter = "'";
    const std::string this_dir = ".";
    const std::string join_path = "/";
    const std::string new_line = "\n";

    //Error messages
    const std::string badPath = "This is not a valid directory/file";
    const std::string entryExists = "There already exists an entry with that name";
    const std::string entryDoesNotExist = "The parent directory of this directory does not exist";
    const std::string entryInUse = "This directory or file is in use";
    const std::string entryCantBeDeleted = "This entry can't be deleted";
} // Parsing
#endif