#pragma once

class Utility
{
public:
    static unsigned int parseHexCharacter(char chr)
    {
        unsigned int result = 0;
        if (chr >= '0' && chr <= '9') result = chr - '0';
        else if (chr >= 'A' && chr <= 'F') result = 10 + chr - 'A';
        else if (chr >= 'a' && chr <= 'f') result = 10 + chr - 'a';

        return result;
    }

    static unsigned int parseHexString(char *str, int length)
    {
        unsigned int result = 0;
        for (int i = 0; i < length; i++) result += parseHexCharacter(str[i]) << (4 * (length - i - 1));
        return result;
    }
};