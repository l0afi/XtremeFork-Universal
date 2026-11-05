#include "Logger.h"

std::unique_ptr<std::ofstream> Logger::s_File = nullptr;

void Logger::Init(const std::string& filename) {
    s_File = std::make_unique<std::ofstream>(filename, std::ios::out | std::ios::trunc);
}

void Logger::Log(const std::string& message) {
    if (s_File && s_File->is_open()) {
        (*s_File) << message << std::endl;
    }
}
