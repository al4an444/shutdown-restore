#include "logger.h"
#include <iomanip>
#include <sstream>
#include <filesystem>

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

std::wstring Logger::getLogPath() {
    // Log file sits next to the executable
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::filesystem::path p(path);
    p = p.parent_path() / L"ShutdownRestore.log";
    return p.wstring();
}

Logger::Logger() {
    m_file.open(getLogPath(), std::ios::app);
}

std::wstring Logger::getTimestamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm_buf;
    localtime_s(&tm_buf, &t);
    std::wostringstream wss;
    wss << std::put_time(&tm_buf, L"%Y-%m-%d %H:%M:%S");
    return wss.str();
}

void Logger::log(const std::wstring& msg) {
    if (m_file.is_open()) {
        m_file << L"[" << getTimestamp() << L"] " << msg << std::endl;
        m_file.flush();
    }
}

void Logger::log(const std::string& msg) {
    log(std::wstring(msg.begin(), msg.end()));
}
