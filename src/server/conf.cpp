#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <grass.hpp>
#include <server/parsing.hpp>


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
 
long getConfPort(std::string filename) {
    std::vector<std::string> ports = getLinesThatStartWithKeyWord("port", filename);
    if (ports.size() != 1) {
        server_failure("wrong number of ports specification in configuration file");
    }
    std::string port_line = ports.front();
    std::vector<std::string> port_info = Parsing::split_string(port_line, Parsing::space);
    return std::stol(port_info[1]);

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