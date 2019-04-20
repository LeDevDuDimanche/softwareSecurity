#ifndef SYSTEMCMD_H
#define SYSTEMCMD_H
#include <iostream>
//Here the strings representing the 
//system calls we will have to make will be kept.
namespace CommandConstants
{
    const std::string ls = "ls -l ";
    const std::string mkdir = "mkdir ";
    const std::string touch = "touch ";
    const std::string rm = "rm -rf ";
    //This constant is for when you want a default buffer size
    //This number was chosen with heuristics
    const unsigned int buffer_size = 128;
} // CommandConstants

//These functions are meant for when you want to run a system command
//and retrieve the output from that command
namespace SystemCommands
{
    std::string ls(std::string cmd, std::string dirname);

    void mkdir(std::string cmd, std::string dirname);

    void rm(std::string cmd, std::string dirname);
} // SystemCommands

#endif