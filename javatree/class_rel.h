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

#pragma once
#include "ll_stdhdr.h"

// ---------------------------------------------------------------------------
class ClassRelations;

class ClassLinkage
{
  public:
    ClassLinkage() : linkage(NULL), relations(NULL) {}
    ClassLinkage*      linkage;
    ClassRelations*    relations;
};

class ClassRelations
{
  public:
    ClassRelations() {}
    ClassRelations(const lstring& name, const lstring& modifier, const lstring& file) 
        { my_name = name; my_modifier = modifier; my_file = file; }
    
    const lstring& name() const { return my_name; }
    const lstring& modifier() const { return my_modifier; }
    const lstring& file() const { return my_file; }
    void  file(const lstring& file) { my_file = file; }
    
    const ClassLinkage& children() const { return my_children; }
    const ClassLinkage& parents() const { return my_parents; }
    const ClassLinkage& interfaces() const { return my_interfaces; }

    void  remove_interfaces(void)   { remove_interfaces(this); }
    void  remove_parents(void)      { remove_parents(this); }
    void  remove_children(void)     { remove_children(this); }
    
    void  add_interface(ClassRelations* crel) 
        { add_linkage_to(my_interfaces, crel); }
    void  add_parent(ClassRelations* crel) 
        { add_linkage_to(my_parents, crel); }
    void  add_child(ClassRelations* crel)  
        { add_linkage_to(my_children, crel); }
        
    ClassLinkage* find_interface(const ClassRelations* crel)
        { return find_relation(my_interfaces, crel); }
    ClassLinkage* find_parent(const ClassRelations* crel)
        { return find_relation(my_parents, crel); }
    ClassLinkage* find_child(const ClassRelations* crel)
        { return find_relation(my_children, crel); }
    
  protected:    
    void  add_linkage_to(ClassLinkage&, ClassRelations*);
    void  remove_interfaces(ClassRelations*);
    void  remove_parents(ClassRelations*);
    void  remove_children(ClassRelations*);
    ClassLinkage* find_relation(ClassLinkage& link, const ClassRelations*); 
    
    lstring        my_name;
    lstring        my_modifier;
    lstring        my_file;
    ClassLinkage    my_children;
    ClassLinkage    my_parents;
    ClassLinkage    my_interfaces;
};
