#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>

#include "logger.h"

namespace ohl::logger {

const char* logPath = "ohl.log";

/**
 * @brief Logs a logMessage to the log file ohl::logger::logPath. \n will be
 * added at the end
 *
 * @param logMessage the message to be logged.
 */
void log( const char * logMessage ) {
    // Open a log file for appending each time
    // at least it deals with the flushing issue.
    std::ofstream logFile( logPath, std::ios_base::app );
    logFile << logMessage << "\n";
    logFile.close();
}
/**
 * @brief Logs a logMessage to the log file ohl::logger::logPath. \n will be
 * added at the end
 *
 * @param logMessage the message to be logged.
 */
void log( const wchar_t * logMessage ) {
    // Open a log file for appending each time
    // at least it deals with the flushing issue.
    std::wofstream logFile( logPath, std::ios_base::app );
    logFile << logMessage << "\n";
    logFile.close();
}
void log( const std::ostream& logMessage ) {
    // Open a log file for appending each time
    // at least it deals with the flushing issue.
    std::ofstream logFile( logPath, std::ios_base::app );
    logFile << logMessage.rdbuf() << std::endl;
    logFile.close();
}
void log( const std::wostream& logMessage ) {
    // Open a log file for appending each time
    // at least it deals with the flushing issue.
    std::wofstream logFile( logPath, std::ios_base::app );
    logFile << logMessage.rdbuf() << "\n";
    logFile.close();
}
/**
 * @brief Logs AND prints a logMessage to the log file ohl::logger::logPath. \n will be
 * added at the end
 *
 * @param logMessage the message to be logged.
 */
void logPrint( const char * logMessage ) {
    std::cout << logMessage << "\n";
    log( logMessage );
}
/**
 * @brief Logs AND prints a logMessage to the log file ohl::logger::logPath. \n will be
 * added at the end
 *
 * @param logMessage the message to be logged.
 */
void logPrint( const wchar_t * logMessage ) {
    std::wcout << logMessage << "\n";
    log( logMessage );
}
void logPrint( const std::wostream& logMessage ) {
    std::wstringstream logStr;
    logStr << logMessage.rdbuf();
    std::wstring str = logStr.str();
    std::wcout << str << "\n";
    log( str.c_str() );
}
void logPrint( const std::ostream& logMessage ) {
    std::stringstream logStr;
    logStr << logMessage.rdbuf();
    std::string str = logStr.str();
    std::cout << str << "\n";
    log( str.c_str() );
}
}  // namespace ohl
