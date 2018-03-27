/*
 * Logger.h
 *
 Copyright (c) 2013 Collin Kidder, Michael Neuweiler, Charles Galpin

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <Arduino.h>
#include "config.h"


class Logger {
public:
    enum LogLevel {
        Debug = 0, Info = 1, Warn = 2, Error = 3, Off = 4
    };
    static void debug(const char *, ...);
    static void info(const char *, ...);
    static void warn(const char *, ...);
    static void error(const char *, ...);
    static void console(const char *, ...);
    static void file(const char *, ...);
    static void fileRaw(uint8_t*, int);
    static void setLoglevel(LogLevel);
    static LogLevel getLogLevel();
    static uint32_t getLastLogTime();
    static boolean isDebug();
    static void loop();
private:
    static LogLevel logLevel;
    static uint32_t lastLogTime;
    static uint8_t filebuffer[BUF_SIZE]; //size of buffer for file output
    static uint16_t fileBuffWritePtr;
    static uint32_t lastWriteTime;

    static void log(LogLevel, const char *format, va_list);
    static void logMessage(const char *format, va_list args);
    static void buffPutChar(char c);
    static void buffPutString(const char *c);
    static boolean setupFile();
    static void flushFileBuff();
};

#endif /* LOGGER_H_ */

