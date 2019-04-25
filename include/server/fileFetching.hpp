
#ifndef FILEFETCHING_HPP
#define FILEFETCHING_HPP

#include <string>
#include <vector>
#include <server/pathvalidate.hpp>
#include <server/systemcmd.hpp>

namespace FileFetching
{
    std::vector<std::string> fetch_all_files_from_dir(std::string dir);

} // FileFetching

#endif
