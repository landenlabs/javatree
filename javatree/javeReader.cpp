//-------------------------------------------------------------------------------------------------
//
//  JavaReader.cpp      5/8/2015        DALang
//
// Based off of:
//  Classtree.C       09/10/92        DALang
//
//  Read java source code files, separate comments from code
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


#include "javaReader.h"

#include <regex>
using namespace std;

// ---------------------------------------------------------------------------
istream& JavaReader::getJavaline(istream& in, lstring& line)
{
    int blockStart = 0;
    int quote1Start = 0;
    int quote2Start = 0;

    if (getline(in, line).good())
    {
        //  "//"
        //  "'//'"
        // @SuppressWarnings({"EmptyCatchBlock", "PointlessBooleanExpression"})

        my_lineNum++;
        my_comments.clear();
        my_removeList.clear();
        CommentItem::findAll(my_comments, line, "//", CommentItem::LineComment);
        CommentItem::findAll(my_comments, line, "/*", CommentItem::BlockCommentOpen);
        CommentItem::findAll(my_comments, line, "*/", CommentItem::BlockCommentClose);
        CommentItem::findAll(my_comments, line, "\"", CommentItem::Quote2);
        CommentItem::findAll(my_comments, line, "'", CommentItem::Quote1);
        CommentItem::sort(my_comments);

        bool quote1Open = false;
        bool quote2Open = false;

        for (Comments::iterator iter = my_comments.begin(); iter != my_comments.end(); iter++)
        {
            if (my_isCommentOpen)
            {
                if (iter->comType == CommentItem::BlockCommentClose)
                {
                    IntPair intPair;
                    intPair.beg = blockStart;
                    intPair.len = (int) iter->idx - blockStart + 2;  // /* ... */        
                    my_removeList.push_back(intPair);
                    blockStart = -1;
                    my_isCommentOpen = false;
                }
            }
            else
            {
                if (quote2Open)
                {
                    // ToDo - handle escaped quotes.
                    if (iter->comType == CommentItem::Quote2)
                    {
                        IntPair intPair;
                        intPair.beg = quote2Start;
                        intPair.len = (int) iter->idx - quote2Start + 1;  // "..."
                        my_removeList.push_back(intPair);
                        quote2Start = -1;
                        quote2Open = false;
                    }
                }
                else if (quote1Open)
                {
                    // ToDo - handle escaped quotes.
                    if (iter->comType == CommentItem::Quote1)
                    {
                        IntPair intPair;
                        intPair.beg = quote1Start;
                        intPair.len = (int) iter->idx - quote1Start + 1;  // '...'   '\0x10'
                        my_removeList.push_back(intPair);
                        quote1Start = -1;
                        quote1Open = false;
                    }
                }
                else
                {
                    switch (iter->comType)
                    {
                    case CommentItem::LineComment:
                        line.erase(iter->idx);
                        return in;
                    case CommentItem::BlockCommentOpen:
                        my_isCommentOpen = true;
                        blockStart = (int) iter->idx;
                        break;
                    case CommentItem::BlockCommentClose:
                        break;
                    case CommentItem::Quote2:
                        quote2Start = (int) iter->idx;
                        quote2Open = true;
                        break;
                    case CommentItem::Quote1:
                        quote1Start = (int) iter->idx;
                        quote1Open = true;
                        break;
                    }
                }
            }
        }
    }

    if (my_isCommentOpen)
    {
        line.erase(blockStart);
    }

    for (RemoveList::reverse_iterator riter = my_removeList.rbegin(); riter != my_removeList.rend(); riter++)
    {
        line.erase(riter->beg, riter->len);  // end comments are 2 char wide.
    }

    return in;
}
