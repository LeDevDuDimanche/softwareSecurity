#ifndef CONF_HPP
#define CONF_HPP
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <parsing.hpp>

namespace AuthenticationMessages
{
    const std::string userDoesNotExist = "This username is not registered";
    const std::string incorrectCommandSequence = "Pass can only be issued right after a successful login command";
    const int authenticatingStatus = 0;
    const int notLoggedIn = -1;
    const int loggedIn = 1;
    const std::string incorrectPassword = "Incorrect password";
    const std::string logutMessage = "Successful logout";
    const std::string mustBeLoggedIn = "access denied.";
} // AuthenticationMessages

std::vector<std::string> getLinesThatStartWithKeyWord(std::string keyword, std::string filename);

bool checkIfUserExists(std::string username, std::string filename);

std::string getConfBaseDir(std::string filename);

long getConfPort(std::string filename);

bool checkIfUserPasswordExists(std::string username, std::string pw, std::string filename);
#endif
