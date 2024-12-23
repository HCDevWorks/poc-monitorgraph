// Logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <string>

class Logger {
public:
    static void Log(const std::string& message) {
        std::ofstream logFile("MonitorGraph.log", std::ios::app);
        if (logFile.is_open()) {
            logFile << message << std::endl;
            logFile.close();
        }
    }
};

#endif
