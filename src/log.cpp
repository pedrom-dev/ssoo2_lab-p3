// log.cpp
#include "log.h"

std::ofstream logFile("log.txt", std::ios_base::app);

void log(const std::string& message) {
    std::cout << message << std::endl;
    logFile << message << std::endl;
}