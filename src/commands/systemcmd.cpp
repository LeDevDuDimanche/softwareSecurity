#include <systemcmd.hpp>
#include <pathvalidate.hpp>
#include <parsing.hpp>


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
        std::string command = cmd + " " + dirname;
        system(command.c_str());
    }


} // SystemCommands
