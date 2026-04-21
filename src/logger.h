#pragma once
#include <string>
#include <fstream>
#include <ctime>
#include <windows.h>

// Simple file logger for the service (services can't use printf/console)
class Logger {
public:
    static Logger& instance();
    void log(const std::wstring& msg);
    void log(const std::string& msg);

private:
    Logger();
    std::wofstream m_file;
    std::wstring getTimestamp();
    std::wstring getLogPath();
};

#define LOG(msg) Logger::instance().log(msg)
