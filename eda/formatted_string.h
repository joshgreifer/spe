//
// Created by josh on 10/11/2020.
//

#ifndef EDA_FORMATTED_STRING_H
#define EDA_FORMATTED_STRING_H
#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
class formatted_string
{
    char *str;
public:
    formatted_string(const char *fmt, ...) : str(NULL)
    {
        va_list args;
                va_start(args, fmt);
        const size_t len = _vscprintf(fmt, args)+1;
                va_start(args, fmt);
        str =  new char[len];
        vsprintf_s(str, len, fmt, args);
    }

    virtual ~formatted_string()
    {
        delete[] str;
    }

    formatted_string(const formatted_string& other)
    {
        const size_t len = strlen(other.str)+1;
        str =  new char[len];
        strcpy_s(str, len, other.str);
    }

    formatted_string& operator=(const formatted_string& other)
    {
        if (this != &other)
            return *this = other.str;
    }

    formatted_string(formatted_string&& other)  : str(NULL)
    {
        str = other.str;
        other.str = NULL;
    }

    formatted_string& operator=(formatted_string&& other)
    {
        if (this != &other) {
            delete[] str;
            str = other.str;
            other.str = NULL;
        }
        return *this;
    }

    operator const char *() const { return str; }

    formatted_string& operator= (const char *s)
    {
        delete[] str;
        const size_t len = strlen(s)+1;
        str =  new char[len];
        strcpy_s(str, len, s);
        return *this;
    }
};
#endif //EDA_FORMATTED_STRING_H
