#include <string>
#include <vector>
#include <server/pathvalidate.hpp>
#include <server/systemcmd.hpp>
#include <server/fileFetching.hpp>
#include <parsing.hpp>

namespace FileFetching
{
    std::vector<std::string> fetch_all_files_from_dir(std::string dir) {
        std::string ls = CommandConstants::simple_ls;
        std::string allEntries = SystemCommands::command_with_output(ls, Parsing::cleanDir(dir));
        std::vector<std::string> entriesVector = Parsing::split_string(allEntries, Parsing::new_line_char);
        std::vector<std::string> files;
        for (std::string entry: entriesVector) {
            std::string path = dir + "/" + entry;
            bool isDir = pathvalidate::isDir(path);
            if (isDir) {
                std::vector<std::string> tempFiles = fetch_all_files_from_dir(path);
                for (std::string f: tempFiles) {
                    files.push_back(f);
                }
                continue;
            }
            bool isFile = pathvalidate::isFile(path);
            if (isFile) {
                files.push_back(path);
            }
        }
        return files;
    }
} // FileFetching
