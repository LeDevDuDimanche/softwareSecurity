#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "parsing.hpp"



namespace Parsing
{

    BadPathException::BadPathException(std::string desc) {
        this->desc = desc;
    }
    
    BadPathException::~BadPathException()
    {
    }

    std::string BadPathException::getDesc() {
        return this->desc;
    }

    std::string get_relative_path(std::string base, std::string fp) {
        size_t found = fp.find(base);
        if (found != 0) {
            return "";
        }
        int l = base.size();
        return fp.substr(l, fp.size());
    }
    std::vector<std::string> split_string(std::string s, char delim) {
        std::vector<std::string> vec;
        std::stringstream ss(s);
        std::string token;
        while(std::getline(ss, token, delim)) {
            if (token.empty()) {
                continue;
            }
            vec.push_back(token);
        }
        return vec;
    }
    std::string join_vector(std::vector<std::string> v, std::string join) {
        std::stringstream ss;
        ss << join;
        for(size_t i = 0; i < v.size(); ++i)
        {
          if(i != 0)
            ss << join;
          ss << v[i];
        }
        std::string s = ss.str();
        return s;
    }

    std::string resolve_path(std::string base, std::string currentDir, std::string path) {
        std::vector<std::string> baseVec = split_string(base, slash);
        std::vector<std::string> currentDirVec = split_string(currentDir, slash);
        std::vector<std::string> pathVec = split_string(path, slash);
        for (std::string dir: pathVec) {
            if (currentDirVec.empty()) {
                break;
            }
            if (dir == this_dir) {
                continue;
            }
            if (dir == parent_dir) {
                currentDirVec.pop_back();
                continue;
            }
            currentDirVec.push_back(dir);
        }
        std::string newCurrentDir = join_vector(currentDirVec, join_path);
        size_t found = newCurrentDir.find(base);
        if (found != 0) {
            BadPathException e{badPath};
            throw e;
        }
        return newCurrentDir;
    }
}