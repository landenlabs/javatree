//-------------------------------------------------------------------------------------------------
//
// File: lstring.h
// Author: Dennis Lang
//  Desc: std::string wrapper
//
//-------------------------------------------------------------------------------------------------
//
// Author: Dennis Lang - 2015
// http://landenlabs.com
//
// This file is part of JavaTree project.
//
// ----- License ----
//
// Copyright (c) 2015 Dennis Lang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once


#include <string>

class lstring : public std::string
{
public:
    lstring() : std::string()
    { }

    lstring(const char* rhs) : std::string(rhs)
    { }

    lstring(const lstring& rhs) : std::string(rhs)
    { }
    lstring(const lstring&& rhs) : std::string(rhs)
    { }

    lstring(const std::string& rhs) : std::string(rhs)
    { }
    lstring(const std::string&& rhs) : std::string(rhs)
    { }

    std::string& toString()
    { return *this;  }

    const std::string& toConstString() const
    {  return *this;  }

    operator const char*() const
    {
        return c_str();
    }

    char back() const
    {
        return std::string::back();
    }

    lstring substr(size_t pos = 0, size_t len = npos) const
    {
        return lstring(std::string::substr(pos, len));
    }

    lstring& trim()
    {
        erase(0, find_first_not_of(' '));       // leading spaces
        erase(find_last_not_of(' ') + 1);       // trailing spaces
        return *this;
    }
    
    lstring& operator=(const lstring& rhs) {
        this->assign(rhs);
        return *this;
    }
};


inline lstring operator+ (const lstring& lhs, const lstring& rhs)
{
    return lhs.toConstString() + rhs.toConstString();
}
inline lstring operator+ (const lstring& lhs, const char*   rhs)
{
    return lhs.toConstString() + rhs;
}

// ---------------------------------------------------------------------------
// Replace all occurances of 'search' with 'replace'
inline const lstring& ReplaceAll(lstring& subject,
    const lstring& search,
    const lstring& replace)
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != lstring::npos)
    {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return subject;
}
