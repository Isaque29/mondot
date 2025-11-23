#include "fileutil.h"
#include <fstream>
#include <stdexcept>
#include <iterator>

using namespace std;

std::string slurp_file(const std::string &path)
{
    std::ifstream ifs(path, std::ios::binary);
    if(!ifs) throw std::runtime_error("cannot open " + path);

    // read whole file into string
    std::string s((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    return s;
}
