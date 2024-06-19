// log.h
#ifndef LOG_H
#define LOG_H

#include <string>
#include <fstream>
#include <iostream>

extern std::ofstream logFile;

void log(const std::string& message);

#endif // LOG_H