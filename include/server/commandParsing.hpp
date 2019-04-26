#ifndef COMMANDPARSING_HPP
#define COMMANDPARSING_HPP
#include <map>
#include <string>
#include <sstream>
#include <vector>
namespace Parsing
{
    //The map was not being cooperative so this is a bad work around
    int commandNameToArgumentsFunc(std::string commandName);

    class CommandNotFoundException
    {
    private:
        std::string commandName;
    public:
        CommandNotFoundException(std::string command);
        ~CommandNotFoundException();
        std::string getDesc();
    };

    class CommandArgumentsException
    {
    private:
        std::string commandName;
        unsigned int argumentTotal;
    public:
        CommandArgumentsException(std::string name, size_t givenArguments);
        ~CommandArgumentsException();
        std::string getDesc();
    };

    int getArgumentTotalForCommand(std::string command);
    bool hasRightNumberOfArguments(std::vector<std::string> commandVector);
} // Parsing

#endif
