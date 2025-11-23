#ifndef MONDOT_UTIL_H
#define MONDOT_UTIL_H

#include <string>

void enable_terminal_colors();
const char* COL_RESET();
const char* COL_DARKGRAY();
const char* COL_YELLOW();
const char* COL_RED();

void colored_fprintf(FILE* out, const char* color, const std::string &msg);

extern bool DEBUG;
void dbg(const std::string &s);
void info(const std::string &s);
void errlog(const std::string &s);

#endif
