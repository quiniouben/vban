#include <stdio.h>
#include <stdarg.h>
#include "logger.h"

enum LogLevel LoggerOutputLevel = LOG_ERROR;
 
void logger_set_output_level(enum LogLevel level)
{
    LoggerOutputLevel = level;
}

void logger_log(enum LogLevel msgLevel, const char* format, ... )
{
	va_list args;
	FILE* const output = (msgLevel <= LOG_WARNING) ? stderr : stdout;

	if (msgLevel > LoggerOutputLevel) return;
	
	switch (msgLevel)
	{
        case LOG_FATAL:
            fprintf(output, "Fatal: ");
            break;
        case LOG_ERROR:
            fprintf(output, "Error: ");
            break;
        case LOG_WARNING:
            fprintf(output, "Warning: ");
            break;
        case LOG_INFO:
            fprintf(output, "Info: ");
            break;
        case LOG_DEBUG:
            fprintf(output, "Debug: ");
            break;
        default:
            return;
    }

	va_start(args, format);
	vfprintf(output, format, args);
	va_end(args);
	fprintf(output, "\n" );
}

