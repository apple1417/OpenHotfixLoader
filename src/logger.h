#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>

namespace ohl::logger {

const char* logPath = "ohl.log";

/**
 * @brief Logs a logMessage to the log file ohl::logger::logPath. \n will be
 * added at the end
 *
 * @param logMessage the message to be logged.
 */
void log( const char * logMessage );
void log( const wchar_t * logMessage );
void log( std::ostream& logMessage );
void log( std::wostream& logMessage );
/**
 * @brief Logs AND prints a logMessage to the log file ohl::logger::logPath. \n will be
 * added at the end
 *
 * @param logMessage the message to be logged.
 */
void logPrint( const char * logMessage );
void logPrint( const wchar_t * logMessage );
void logPrint( std::ostream& logMessage );
void logPrint( std::wostream& logMessage );
}  // namespace ohl
