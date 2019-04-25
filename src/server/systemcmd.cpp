#include <server/systemcmd.hpp>
#include <server/pathvalidate.hpp>
#include <server/parsing.hpp>


namespace SystemCommands
{

    void clear(char *buffer) {
	    for (int i = 0; i < 256;i++) {
	    	buffer[i] = '\0'; 
	    }
    }

    std::string ls(std::string cmd, std::string dirname) {
        char newLine = '\n';
	    FILE *fpipe;
        char c = 0;

        if (0 == (fpipe = (FILE*)popen((cmd + " " + dirname).c_str(), "r")))
        {
            perror("popen() failed.");
            exit(1);
        }
	    std::vector<std::string> lines;
	    char buffer[256];
	    int index = 0;
        while (fread(&c, sizeof c, 1, fpipe))
        {
            //printf("%c", c);
	    	buffer[index++] = c;
	    	if (c == newLine) {
	    		std::string temp{buffer};
	    		lines.push_back(temp);
	    		clear(buffer);
	    		index = 0;

	    	}
        }
	    for(std::string line: lines) {
	    	std::cout << line;
	    }
        pclose(fpipe);
        std::string retStr = Parsing::join_vector(lines, "");
        return retStr;
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
