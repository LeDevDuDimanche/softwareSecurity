#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <server/commandParsing.hpp>
namespace Parsing
{

    int commandNameToArgumentsFunc(std::string commandName) {
        if (commandName == "cd") {
            return 1;
        }
        if (commandName == "rm") {
            return 1;
        }
        if (commandName == "ls") {
            return 0;
        }
        if (commandName == "mkdir") {
            return 1;
        }
        if (commandName == "get") {
            return 1;
        }
        if (commandName == "put") {
            return 2;
        }
        if (commandName == "w") {
            return 0;
        }
        if (commandName == "whoami") {
            return 0;
        }
        if (commandName == "date") {
            return 0;
        }
        if (commandName == "ping") {
            return 1;
        }
        if (commandName == "login") {
            return 1;
        }
        if (commandName == "pass") {
            return 1;
        }
        if (commandName == "exit") {
            return 0;
        }
        if (commandName == "logout") {
            return 0;
        }
        if (commandName == "grep") {
            return 1;
        }
        return -1;
    }

    std::string CommandNotFoundException::getDesc() {
        char placeholder[600];
        sprintf(placeholder, "Couldn't find the command %s", commandName.c_str());
        std::string ret{placeholder};
        return ret;
    }
    CommandNotFoundException::CommandNotFoundException(std::string command)
    {
        this-> commandName = command;
    }

    CommandNotFoundException::~CommandNotFoundException()
    {
    }



    CommandArgumentsException::CommandArgumentsException(std::string name, size_t givenArguments) {
        this->commandName = name;
        this->argumentTotal = givenArguments;
    }
    std::string CommandArgumentsException::getDesc() {
        unsigned int expectedArgumentTotal = commandNameToArgumentsFunc(this->commandName);
        std::stringstream ss1;
        std::stringstream ss2;
        ss1 << expectedArgumentTotal;
        ss2 << this->argumentTotal;
        return "The command " + this->commandName + " takes in " + ss1.str() + " arguments, not " + ss2.str();
    }

    bool hasCommand(std::string command) {
        int argumentTotal = commandNameToArgumentsFunc(command);
        return argumentTotal != -1;
    }
    int getArgumentTotalForCommand(std::string command) {
        bool exists = hasCommand(command);
        if (!exists) {
            CommandNotFoundException e{command};
            throw e;
        }
        return commandNameToArgumentsFunc(command);
    }
    bool hasRightNumberOfArguments(std::vector<std::string> commandVector) {
        std::string commandName = commandVector[0];
        size_t argumentTotal = getArgumentTotalForCommand(commandName);
        if (argumentTotal != commandVector.size() - 1) {
            CommandArgumentsException e{commandName, commandVector.size() - 1};
            throw e;
        }
        return true;
    }

    CommandArgumentsException::~CommandArgumentsException()
    {
    }
} // Parsing
