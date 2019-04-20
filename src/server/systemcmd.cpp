#include <server/systemcmd.hpp>
#include <server/pathvalidate.hpp>
#include <server/parsing.hpp>


namespace SystemCommands
{


    std::string ls(std::string cmd, std::string dirname) {
        const char *c_cmd = (cmd + dirname).c_str();
        unsigned int buf_size = CommandConstants::buffer_size;
        char buffer[buf_size];
        std::string result = "";
        FILE* pipe = popen(c_cmd, "r");
        if (!pipe) throw std::runtime_error("popen() failed!");
        try{
            while(fgets(buffer, sizeof buffer, pipe) != NULL) {
                result += buffer;
            }
        }
        catch(...) {
            pclose(pipe);
        }
        return result;
    }

    void mkdir(std::string cmd, std::string dirname) {
        bool exists = pathvalidate::exists(dirname);
        if (exists) {
            Parsing::BadPathException e{Parsing::entryExists};
            throw e;
        }
        std::vector<std::string> split_dir = Parsing::split_string(dirname, Parsing::slash);
        split_dir.pop_back();
        std::string parentDir = Parsing::join_vector(split_dir, Parsing::join_path);
        bool parentExists = pathvalidate::isDir(parentDir);
        if (!parentExists) {
            Parsing::BadPathException e{Parsing::entryDoesNotExist};
            throw e;
        }
        std::string command = cmd + " " + dirname;
        system(command.c_str());
    }

    void rm(std::string cmd, std::string dirname) {
        bool exists = pathvalidate::exists(dirname);
        if (!exists) {
            Parsing::BadPathException e{Parsing::entryDoesNotExist};
            throw e;
        }
        std::string command = cmd + " " + dirname;
        system(command.c_str());
    }


} // SystemCommands
