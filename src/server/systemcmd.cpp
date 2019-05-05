#include <server/systemcmd.hpp>
#include <server/pathvalidate.hpp>
#include <parsing.hpp>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>


namespace SystemCommands
{

    void clear(char *buffer) {
	    for (int i = 0; i < 256;i++) {
	    	buffer[i] = '\0';
	    }
    }
    std::string ping(std::string host) {
        auto sec = std::chrono::seconds(1);
        //Used for the timeout
        std::mutex m;
        std::condition_variable cv;
        std::string pingRetValue;
        std::thread t([&cv, &pingRetValue, &host]()
        {
            pingRetValue = command_with_output(CommandConstants::ping + host, "");
            cv.notify_one();
        });

        t.detach();
        {
            std::unique_lock<std::mutex> l(m);
            if(cv.wait_for(l, 5*sec) == std::cv_status::timeout)
                throw std::runtime_error("Host did not respond");
        }
        return pingRetValue;
    }

    std::string command_with_output(std::string cmd, std::string dirname) {
        char buffer[CommandConstants::buffer_size];
	    clear(buffer);
        std::vector<std::string> lines;
	    int index = 0;
        char newLine = '\n';
	    FILE *fpipe;
        char c = 0;

        if (0 == (fpipe = (FILE*)popen((cmd + " " + dirname).c_str(), "r")))
        {
            perror("popen() failed.");
            exit(1);
        }
        while (fread(&c, sizeof c, 1, fpipe))
        {
	    	buffer[index++] = c;
	    	if (c == newLine) {
	    		std::string temp{buffer};
                if (dirname.empty()) {
                    lines.push_back(temp);
                }
                else {
                    lines.push_back(Parsing::bufferToString(buffer));
                }
	    		clear(buffer);
	    		index = 0;

	    	}
        }
        pclose(fpipe);
        std::string retStr = Parsing::join_vector(lines, "");
        //This is for when there is no host
        //std::cout << "Something up: " << retStr << std::endl;
        return retStr;
    }
    void mkdir(std::string cmd, std::string dirname) {
        std::string cleanDir = Parsing::cleanDir(dirname);
        bool exists = pathvalidate::exists(cleanDir);
        if (exists) {
            Parsing::BadPathException e{Parsing::entryExists};
            throw e;
        }
        std::vector<std::string> split_dir = Parsing::split_string(cleanDir, Parsing::slash);
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
        std::string cleanDir = Parsing::cleanDir(dirname);
        bool exists = pathvalidate::exists(cleanDir);
        if (!exists) {
            Parsing::BadPathException e{Parsing::entryDoesNotExist};
            throw e;
        }
        std::string command = cmd + " " + dirname;
        system(command.c_str());
    }

    //grep returns true if there is any line that matches the given pattern
    //and returns false if no line matches the given pattern.
    bool grep(std::string filepath, std::string pattern) {
        std::string cmd = CommandConstants::grep;
        std::string cmd_concat = cmd + pattern + " " + filepath;
        int match = system(cmd_concat.c_str());
        if (match == 0) {
            return true;
        }
        return false;
    }


} // SystemCommands
