// Logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <string>

//TODO: Mudar padrões de escrita do logger para melhor entendimento
// Adicionar data e hora nas mensagens de log
// Adicionar níveis de log (INFO, WARNING, ERROR)
// Adicionar suporte a cores no terminal

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
