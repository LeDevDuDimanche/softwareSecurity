#include <vector>
#include <fstream>
#include <string>
#include <sstream>

#include <grass.hpp>
#include <parsing.hpp>


std::vector<std::string> getLinesThatStartWithKeyWord(std::string keyword, std::string filename) {
    std::ifstream infile;
    infile.open(filename);
    std::string line;
    std::vector<std::string> lines;
    while(std::getline(infile, line)) {
        std::vector<std::string> split_line = Parsing::split_string(line, Parsing::space);
        if (split_line.empty()) {
            continue;
        }
        std::string first = split_line[0];
        if (first == keyword) {
            lines.push_back(line);
        }
    }
    return lines;
}

bool checkIfUserExists(std::string username, std::string filename) {
    std::vector<std::string> users = getLinesThatStartWithKeyWord("user", filename);
    for(std::string line: users) {
        std::vector<std::string> userData = Parsing::split_string(line, Parsing::space);
        std::string un = userData[1];
        if (un == username) {
            return true;
        }
    }
    return false;
}

std::string getUniqueMatchValue(std::string filename, std::string keyword, int info_idx) {
    std::vector<std::string> matches = getLinesThatStartWithKeyWord(keyword, filename);
    if (matches.size() != 1) {
        server_failure("wrong number of ports specification in configuration file");
    }
    std::vector<std::string> matches_infos =
        Parsing::split_string(matches.front(), Parsing::space);
    if ((size_t) info_idx < matches_infos.size()) {
        return matches_infos[info_idx];
    } else {
        server_failure("out of bound argument requested from configuration file");
    }
}

std::string getConfBaseDir(std::string filename) {
    return getUniqueMatchValue(filename, "base", 1);
}

long getConfPort(std::string filename) {
    std::string port_as_str = getUniqueMatchValue(filename, "port", 1);
    return std::stol(port_as_str);

}

bool checkIfUserPasswordExists(std::string username, std::string pw, std::string filename) {
    std::vector<std::string> users = getLinesThatStartWithKeyWord("user", filename);
    for(std::string line: users) {
        std::vector<std::string> userData = Parsing::split_string(line, Parsing::space);
        std::string un = userData[1];
        if (un == username) {
            return pw == userData[2];
        }
    }
    return false;
}
