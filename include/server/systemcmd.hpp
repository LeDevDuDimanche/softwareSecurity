#ifndef SYSTEMCMD_H
#define SYSTEMCMD_H
#include <iostream>
//Here the strings representing the 
//system calls we will have to make will be kept.
namespace CommandConstants
{
    const std::string ls = "ls -l ";
    const std::string simple_ls = "ls";
    const std::string mkdir = "mkdir ";
    const std::string touch = "touch ";
    const std::string rm = "rm -rf ";
    const std::string date = "date ";
    const std::string ping = "ping -c 1 ";
    //We just need to check if the pattern matches any line of the file
    const std::string grep = "grep -q ";
    //This constant is for when you want a default buffer size
    //This number was chosen with heuristics
    const unsigned int buffer_size = 256;
} // CommandConstants

//These functions are meant for when you want to run a system command
//and retrieve the output from that command
namespace SystemCommands
{

    std::string ping(std::string host);

    std::string command_with_output(std::string cmd, std::string dirname);

    void mkdir(std::string cmd, std::string dirname);

    void rm(std::string cmd, std::string dirname);

    bool grep(std::string filepath, std::string pattern);
} // SystemCommands

#endif