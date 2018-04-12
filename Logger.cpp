/*
 * Logger.cpp
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

#include "Logger.h"
#include "config.h"
#include "sys_io.h"
#include "EEPROM.h"

Logger::LogLevel Logger::logLevel = Logger::Info;
uint32_t Logger::lastLogTime = 0;
uint16_t Logger::fileBuffWritePtr = 0;
uint8_t Logger::filebuffer[BUF_SIZE]; //size of buffer for file output
uint32_t Logger::lastWriteTime = 0;
bool fileInitialized = false;

/*
 * Output a debug message with a variable amount of parameters.
 * printf() style, see Logger::log()
 *
 */
void Logger::debug(const char *message, ...)
{
    if (logLevel > Debug) {
        return;
    }

    va_list args;
    va_start(args, message);
    Logger::log(Debug, message, args);
    va_end(args);
}

/*
 * Output a info message with a variable amount of parameters
 * printf() style, see Logger::log()
 */
void Logger::info(const char *message, ...)
{
    if (logLevel > Info) {
        return;
    }

    va_list args;
    va_start(args, message);
    Logger::log(Info, message, args);
    va_end(args);
}

/*
 * Output a warning message with a variable amount of parameters
 * printf() style, see Logger::log()
 */
void Logger::warn(const char *message, ...)
{
    if (logLevel > Warn) {
        return;
    }

    va_list args;
    va_start(args, message);
    Logger::log(Warn, message, args);
    va_end(args);
}

/*
 * Output a error message with a variable amount of parameters
 * printf() style, see Logger::log()
 */
void Logger::error(const char *message, ...)
{
    if (logLevel > Error) {
        return;
    }

    va_list args;
    va_start(args, message);
    Logger::log(Error, message, args);
    va_end(args);
}

/*
 * Output a console message with a variable amount of parameters
 * printf() style, see Logger::logMessage()
 */
void Logger::console(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    Logger::logMessage(message, args);
    va_end(args);
}

void Logger::buffPutChar(char c)
{
    *(filebuffer + fileBuffWritePtr++) = c;
}

void Logger::buffPutString(const char *c)
{
    while (*c) *(filebuffer + fileBuffWritePtr++) = *c++;
}

void Logger::flushFileBuff()
{
    Logger::debug("Write to SD Card %i bytes", fileBuffWritePtr);
    lastWriteTime = millis();
/*
    if (!FS.Write((const char *)filebuffer, fileBuffWritePtr)) {
        Logger::error("Write to SDCard failed!");
        SysSettings.useSD = false;
        fileBuffWritePtr = 0;
        return;
    } */
    //FS.Flush(); //force write of the data to card

    SysSettings.logToggle = !SysSettings.logToggle;
    setLED(SysSettings.LED_LOGGING, SysSettings.logToggle);
    fileBuffWritePtr = 0;
}

boolean Logger::setupFile()
{/*
    if (!fileInitialized) {
        String filename;
        if (settings.appendFile == 1) {
            filename = String(settings.fileNameBase);
            filename.concat(".");
            filename.concat(settings.fileNameExt);
            if (FS.Open("0:", filename.c_str(), true)) {
                FS.GoToEnd();
                fileInitialized = true;
            }
        } else {
            filename = String(settings.fileNameBase);
            filename.concat(settings.fileNum++);
            filename.concat(".");
            filename.concat(settings.fileNameExt);
            EEPROM.write(EEPROM_ADDR, settings); //save settings to save updated filenum
            if (FS.CreateNew("0:", filename.c_str())) {
                fileInitialized = true;
            }
        }
        if (!fileInitialized) {
            Logger::error("open failed");
            return false;
        }
    }

    //Before we add the next frame see if the buffer is nearly full. if so flush it first.
    if (fileBuffWritePtr > BUF_SIZE - 40) {
        flushFileBuff();
    } 
    return true; */
    return false;
}

void Logger::loop()
{
    if (fileBuffWritePtr > 0) {
        if (millis() > (lastWriteTime + 200)) { //flush out data if enough time has gone by
            flushFileBuff();
        }
    }
}

void Logger::file(const char *message, ...)
{
    if (!SysSettings.SDCardInserted) {
        Logger::debug("Trying to write data to file without SDCard inserted");
        return; // not possible to log without card
    }

    char buff[20];

    va_list args;
    va_start(args, message);

    if (!setupFile()) return;

    for (; *message != 0; ++message) {
        if (*message == '%') {
            ++message;

            if (*message == '\0') {
                break;
            }

            if (*message == '%') {
                buffPutChar(*message);
                continue;
            }

            if (*message == 's') {
                register char *s = (char *)va_arg(args, int);
                buffPutString(s);
                continue;
            }

            if (*message == 'd' || *message == 'i') {
                sprintf(buff, "%i", va_arg(args, int));
                buffPutString(buff);
                continue;
            }

            if (*message == 'f') {
                sprintf(buff, "%f0.2", va_arg(args, double));
                buffPutString(buff);
                continue;
            }

            if (*message == 'x') {
                sprintf(buff, "%x", va_arg(args, int));
                buffPutString(buff);
                continue;
            }

            if (*message == 'X') {
                buffPutString("0x");
                sprintf(buff, "%x", va_arg(args, int));
                buffPutString(buff);
                continue;
            }

            if (*message == 'l') {
                sprintf(buff, "%l", va_arg(args, long));
                buffPutString(buff);
                continue;
            }

            if (*message == 'c') {
                buffPutChar(va_arg(args, int));
                continue;
            }

            if (*message == 't') {
                if (va_arg(args, int) == 1) {
                    buffPutString("T");
                } else {
                    buffPutString("F");
                }

                continue;
            }

            if (*message == 'T') {
                if (va_arg(args, int) == 1) {
                    buffPutString("TRUE");
                } else {
                    buffPutString("FALSE");
                }
                continue;
            }

        }

        buffPutChar(*message);
    }

    buffPutString("\r\n");

    va_end(args);

}

void Logger::fileRaw(uint8_t* buff, int sz)
{
    if (!SysSettings.SDCardInserted) {
        Logger::debug("File write without SD card inserted!");
        return; // not possible to log without card
    }

    if (!setupFile()) {
        Logger::debug("Attempt to write without file being open");
        return;
    }

    for (int i; i < sz; i++) {
        buffPutChar(*buff++);
    }
}

/*
 * Set the log level. Any output below the specified log level will be omitted.
 */
void Logger::setLoglevel(LogLevel level)
{
    logLevel = level;
}

/*
 * Retrieve the current log level.
 */
Logger::LogLevel Logger::getLogLevel()
{
    return logLevel;
}

/*
 * Return a timestamp when the last log entry was made.
 */
uint32_t Logger::getLastLogTime()
{
    return lastLogTime;
}

/*
 * Returns if debug log level is enabled. This can be used in time critical
 * situations to prevent unnecessary string concatenation (if the message won't
 * be logged in the end).
 *
 * Example:
 * if (Logger::isDebug()) {
 *    Logger::debug("current time: %d", millis());
 * }
 */
boolean Logger::isDebug()
{
    return logLevel == Debug;
}

/*
 * Output a log message (called by debug(), info(), warn(), error(), console())
 *
 * Supports printf() like syntax:
 *
 * %% - outputs a '%' character
 * %s - prints the next parameter as string
 * %d - prints the next parameter as decimal
 * %f - prints the next parameter as double float
 * %x - prints the next parameter as hex value
 * %X - prints the next parameter as hex value with '0x' added before
 * %b - prints the next parameter as binary value
 * %B - prints the next parameter as binary value with '0b' added before
 * %l - prints the next parameter as long
 * %c - prints the next parameter as a character
 * %t - prints the next parameter as boolean ('T' or 'F')
 * %T - prints the next parameter as boolean ('true' or 'false')
 */
void Logger::log(LogLevel level, const char *format, va_list args)
{
    lastLogTime = millis();
    Serial.print(lastLogTime);
    Serial.print(" - ");

    switch (level) {
    case Debug:
        Serial.print("DEBUG");
        break;

    case Info:
        Serial.print("INFO");
        break;

    case Warn:
        Serial.print("WARNING");
        break;

    case Error:
        Serial.print("ERROR");
        break;
    }

    Serial.print(": ");

    logMessage(format, args);
}

/*
 * Output a log message (called by log(), console())
 *
 * Supports printf() like syntax:
 *
 * %% - outputs a '%' character
 * %s - prints the next parameter as string
 * %d - prints the next parameter as decimal
 * %f - prints the next parameter as double float
 * %x - prints the next parameter as hex value
 * %X - prints the next parameter as hex value with '0x' added before
 * %l - prints the next parameter as long
 * %c - prints the next parameter as a character
 * %t - prints the next parameter as boolean ('T' or 'F')
 * %T - prints the next parameter as boolean ('true' or 'false')
 */
void Logger::logMessage(const char *format, va_list args)
{
    uint8_t buffer[200];
    uint8_t buffLen = 0;
    uint8_t writeLen;
    for (; *format != 0; ++format) {
        if (*format == '%') {
            ++format;

            if (*format == '\0') {
                break;
            }

            if (*format == '%') {
                buffer[buffLen++] = *format;
                continue;
            }

            if (*format == 's') {
                register char *s = (char *) va_arg(args, int);
                writeLen = sprintf((char*)&buffer[buffLen], "%s", s);
                buffLen += writeLen;
                continue;
            }

            if (*format == 'd' || *format == 'i') {
                writeLen = sprintf((char*)&buffer[buffLen], "%i", va_arg(args, int));
                buffLen += writeLen;
                continue;
            }

            if (*format == 'f') {
                writeLen = sprintf((char*)&buffer[buffLen], "%.2f", va_arg(args, double));
                buffLen += writeLen;
                continue;
            }

            if (*format == 'x') {
                writeLen = sprintf((char*)&buffer[buffLen], "%X", va_arg(args, int));
                buffLen += writeLen;
                continue;
            }

            if (*format == 'X') {
                writeLen = sprintf((char*)&buffer[buffLen], "0x%X", va_arg(args, int));
                buffLen += writeLen;
                continue;
            }

            if (*format == 'l') {
                writeLen = sprintf((char*)&buffer[buffLen], "%l", va_arg(args, long));
                buffLen += writeLen;
                continue;
            }

            if (*format == 'c') {
                writeLen = sprintf((char*)&buffer[buffLen], "%c", va_arg(args, int));
                buffLen += writeLen;
                continue;
            }

            if (*format == 't') {
                if (va_arg(args, int) == 1) {
                    buffer[buffLen++] = 'T';
                } else {
                    buffer[buffLen++] = 'F';
                }
                continue;
            }

            if (*format == 'T') {
                if (va_arg(args, int) == 1) {
                    writeLen = sprintf((char*)&buffer[buffLen], "TRUE");
                    buffLen += writeLen;
                } else {
                    writeLen = sprintf((char*)&buffer[buffLen], "FALSE");
                    buffLen += writeLen;
                }
                continue;
            }
        }
        else buffer[buffLen++] = *format;
    }
    buffer[buffLen++] = '\r';
    buffer[buffLen++] = '\n';
    Serial.write(buffer, buffLen);
    //If wifi has connected nodes then send to them too.
    for(int i = 0; i < MAX_CLIENTS; i++){
        if (SysSettings.clientNodes[i] && SysSettings.clientNodes[i].connected()){
            SysSettings.clientNodes[i].write(buffer, buffLen);
        }
    }
}


