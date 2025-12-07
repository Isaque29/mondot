#ifndef MONDOT_FILEUTIL_H
#define MONDOT_FILEUTIL_H

#include <string>
#include <filesystem>

std::string slurp_file(const std::string &path);

struct ScriptFile
{
    std::string path;
    std::filesystem::file_time_type last_write;
};

#endif
