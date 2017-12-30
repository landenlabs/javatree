//-------------------------------------------------------------------------------------------------
//
// File: Class_rel
// Author: Dennis Lang
// Desc: Class relations
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

#include "class_rel.h"

//-------------------------------------------------------------------------------------------------
void ClassRelations::add_linkage_to(ClassLinkage& start_linkage, ClassRelations* crel_ptr)
{
    ClassLinkage* linkage_ptr = &start_linkage;
    
    while (linkage_ptr->relations != NULL)
    {
        if (linkage_ptr->relations == crel_ptr)
            return;
        
        if (linkage_ptr->linkage == NULL)
            linkage_ptr->linkage = new ClassLinkage;
        
        linkage_ptr = linkage_ptr->linkage;
    }
    
    linkage_ptr->relations = crel_ptr;
}


//-------------------------------------------------------------------------------------------------
void ClassRelations::remove_interfaces(ClassRelations* crel_ptr)
{                   
    if (crel_ptr != NULL)
    {
        ClassLinkage* linkage_ptr = (ClassLinkage*)&crel_ptr->interfaces();
        
        do 
        {
            remove_interfaces(linkage_ptr->relations);
            linkage_ptr->relations = NULL;
            
            ClassLinkage* linknext_ptr = linkage_ptr->linkage;
            delete linkage_ptr;
            linkage_ptr = linknext_ptr;
            
        } while (linkage_ptr != NULL);
    }        
}

//-------------------------------------------------------------------------------------------------
void ClassRelations::remove_parents(ClassRelations* crel_ptr)
{                   
    if (crel_ptr != NULL)
    {
        ClassLinkage* linkage_ptr = (ClassLinkage*)&crel_ptr->parents();
        
        do 
        {
            remove_parents(linkage_ptr->relations);
            linkage_ptr->relations = NULL;
            
            ClassLinkage* linknext_ptr = linkage_ptr->linkage;
            delete linkage_ptr;
            linkage_ptr = linknext_ptr;
            
        } while (linkage_ptr != NULL);
    }        
}


//-------------------------------------------------------------------------------------------------
void ClassRelations::remove_children(ClassRelations* crel_ptr)
{                   
    if (crel_ptr != NULL)
    {
        ClassLinkage* linkage_ptr = (ClassLinkage*)&crel_ptr->children();
        
        do 
        {
            remove_children(linkage_ptr->relations);
            linkage_ptr->relations = NULL;
            
            ClassLinkage* linknext_ptr = linkage_ptr->linkage;
            delete linkage_ptr;
            linkage_ptr = linknext_ptr;
            
        } while (linkage_ptr != NULL);
    }        
}


//-------------------------------------------------------------------------------------------------
ClassLinkage* ClassRelations::find_relation(ClassLinkage& 
  start_linkage, const ClassRelations* crel_ptr)
{
    ClassLinkage* linkage_ptr = &start_linkage;
    
    while (linkage_ptr != NULL && linkage_ptr->relations != crel_ptr)
    {
        linkage_ptr = linkage_ptr->linkage;
    }
    
    return linkage_ptr;
}


