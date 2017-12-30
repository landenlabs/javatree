//-------------------------------------------------------------------------------------------------
//
// File: javaReader
// Author: Dennis Lang
// Read java source code files, separate comments from code
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
//-------------------------------------------------------------------------------------------------

#include "ll_stdhdr.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>

struct CommentItem
{
    size_t idx;
    enum ComType { LineComment, BlockCommentOpen, BlockCommentClose, Quote2, Quote1 };
    ComType comType;

    static bool compare(const CommentItem& lhs, const CommentItem& rhs) { return (lhs.idx < rhs.idx); }

    static std::vector<CommentItem>& findAll(std::vector<CommentItem>& found, lstring& have, const char* want, ComType comType)
    {
        size_t foundIdx = -1;
        while ((foundIdx = have.find(want, foundIdx + 1)) != -1)
        {
            CommentItem comItem;
            comItem.idx = foundIdx;
            comItem.comType = comType;
            found.push_back(comItem);
        }
        return found;
    }

    static void sort(std::vector<CommentItem>& list)
    {
        std::sort(list.begin(), list.end(), CommentItem::compare);
    }
};

class JavaReader
{
public:
    istream& getJavaline(istream& in, lstring& line);

    size_t getLineNum() const 
    { return my_lineNum; }

private:

    typedef std::vector<CommentItem> Comments;
    Comments my_comments;
    bool my_isCommentOpen = false;
    struct IntPair
    {
        int beg;
        int len;
    };
    typedef std::vector<IntPair> RemoveList;
    RemoveList my_removeList;
    unsigned int my_lineNum = 0;
};
