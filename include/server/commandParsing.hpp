#ifndef COMMANDPARSING_HPP
#define COMMANDPARSING_HPP
#include <map>
#include <string>
#include <sstream>
#include <vector>
namespace Parsing
{

    class CommandNotFoundException
    {
    private:
        std::string commandName;
    public:
        CommandNotFoundException(std::string command);
        ~CommandNotFoundException();
        std::string getDesc();
    };

    std::string CommandNotFoundException::getDesc() {
        return "The command " + this->commandName + " was not found\n";
    }
    CommandNotFoundException::CommandNotFoundException(std::string command)
    {
        this-> commandName = command;
    }
    
    CommandNotFoundException::~CommandNotFoundException()
    {
    }

    class CommandArgumentsException
    {
    private:
        std::string commandName;
        unsigned int argumentTotal;
    public:
        CommandArgumentsException(std::string name, unsigned int givenArguments);
        ~CommandArgumentsException();
        std::string getDesc();
    };
    
    CommandArgumentsException::CommandArgumentsException(std::string name, unsigned int givenArguments) {
        this->commandName = name;
        this->argumentTotal = givenArguments;
    }

    std::string CommandArgumentsException::getDesc() {
        unsigned int expectedArgumentTotal = commandNameToArguments[this->commandName];
        std::stringstream ss1;
        std::stringstream ss2;
        ss1 << expectedArgumentTotal;
        ss2 << this->argumentTotal;
        return "The command " + this->commandName + " takes in " + ss1.str() + "arguments, not " + ss2.str() + "\n";
    }
    
    CommandArgumentsException::~CommandArgumentsException()
    {
    }
    
    

    std::map<std::string, unsigned int> commandNameToArguments = {
        {"cd", 1}, 
        {"ls", 0},
        {"mkdir", 1},
        {"rm", 1}, 
        {"whoami", 0}, 
        {"w", 0}, 
        {"grep", 1}, 
        {"date", 0}, 
        {"login", 1},
        {"pass", 1},
        {"logout", 0},
        {"exit", 0},
        {"ping", 0},
        {"get", 1},
        {"put", 2}
    };

    bool hasCommand(std::string command) {
        return commandNameToArguments.find(command) != commandNameToArguments.end();
    }

    unsigned int getArgumentTotalForCommand(std::string command) {
        bool exists = hasCommand(command);
        if (!exists) {
            CommandNotFoundException e{command};
            throw e;
        }
        return commandNameToArguments[command];
    }

    bool hasRightNumberOfArguments(std::vector<std::string> commandVector) {
        std::string commandName = commandVector[0];
        unsigned int argumentTotal = getArgumentTotalForCommand(commandName);
        if (argumentTotal != commandVector.size() - 1) {
            CommandArgumentsException e{commandName, commandVector.size() - 1};
            throw e;
        }
        return true;
    }
} // Parsing

#endif