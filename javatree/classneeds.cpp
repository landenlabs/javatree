//-----------------------------------------------------------------------------
//  Classneeds.cpp      17-May-2015   Dennis Lang
//
//  Parse objects 
//-----------------------------------------------------------------------------
//  Requires:   classlist and class_rel
//-----------------------------------------------------------------------------
//
// Author: Dennis Lang - 2015
// http://home.comcast.net/~lang.dennis/
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


#ifndef CLASSLIST_H
// #include "classlist.h"
#endif

#include <stdio.h>
#include <fstream>
#include <ctype.h>

#ifndef PATTERN_H
#include "pattern.h"
#endif

#ifndef NUMSTR_H
#include "numstr.H"
#endif

#ifndef WWTIME_H
#include "wwtime.H"
#endif


Class_list nlist(400);      // Classes
Class_list flist(300);      // Files

const short MAX_LEVELS = 30;
WWString    levels[MAX_LEVELS];
              
// Runtime switches

enum {GRAPHICS_CHAR=0, TEXT_CHAR=1, SPACE_CHAR=2};
       
short show_possessions  = 0;               
short show_multiple_parents = 0;               
short list_n_classes    = 0;               
short list_f_classes    = 0;               
short show_n_tree       = 1;               
short show_f_tree       = 0;               
short print             = 0;
short tab               = 2;
short cset              = GRAPHICS_CHAR;

// Display stuff

char more[]         = {0xb3, '|', ' '};
char more_and_me[]  = {0xc3, '+', ' '};
char just_me[]      = {0xc0, '-', ' '};
char indent_text[81] = 
"                                       "
"                                       ";

// ---------------------------------------------------------------------------
// Add a class to list
// ---------------------------------------------------------------------------
Class_relations* add_class(Class_list& list, const WWString& class_name)
{
    Class_relations* crel_ptr = list.locate(class_name);
    
    if (crel_ptr == (Class_relations*)NULL_PTR)
    {
        crel_ptr = new Class_relations(class_name);
        list.insert(*crel_ptr);
    }
    
    return crel_ptr;
}

// ---------------------------------------------------------------------------
// Add a "provides" class to list
// ---------------------------------------------------------------------------
void add_provides(Class_relations* crel_ptr, const WWString& item_name)
{                                       
    Class_relations* drel_ptr = nlist.locate(item_name);

    if (drel_ptr == (Class_relations*)NULL_PTR)
    {
        drel_ptr = new Class_relations(item_name);
        nlist.insert(*drel_ptr);
        
        crel_ptr->add_parent(drel_ptr);
        drel_ptr->add_child(crel_ptr);
    }
    else
    {
        Class_linkage* link_ptr = drel_ptr->find_parent(crel_ptr);
        if ((void*)link_ptr != NULL)
        {
            link_ptr->relations = NULL;
        }
        
        if ((void*)drel_ptr->find_child(crel_ptr) == NULL)
        {
            crel_ptr->add_parent(drel_ptr);
            drel_ptr->add_child(crel_ptr);
        }
    }
    
}

// ---------------------------------------------------------------------------
// Add a "needs" class to list
// ---------------------------------------------------------------------------
void add_needs(Class_relations* crel_ptr, const WWString& item_name)
{                                       
    Class_relations* drel_ptr = nlist.locate(item_name);

    if (drel_ptr == (Class_relations*)NULL_PTR)
    {
        drel_ptr = new Class_relations(item_name);
        nlist.insert(*drel_ptr);
        crel_ptr->add_child(drel_ptr);
        drel_ptr->add_parent(crel_ptr);
    }
    else
    {
        if ((void*)drel_ptr->find_child(crel_ptr) == NULL &&
            (void*)drel_ptr->find_parent(crel_ptr) == NULL)
        {
            crel_ptr->add_child(drel_ptr);
            drel_ptr->add_parent(crel_ptr);
        }
    }
}

// ---------------------------------------------------------------------------
void display_other_parents(const Class_relations* parent_ptr, 
 const Class_linkage* first_parent)
{
    const Class_linkage* link_ptr = first_parent;
    Class_relations* crel_ptr;

    while ((void*)link_ptr != NULL)
    {
        crel_ptr = link_ptr->relations;
        if ((void*)crel_ptr != NULL && crel_ptr != parent_ptr)
        {   
            const char* name = crel_ptr->name();
            printf("  (%s)", name);
        }
                
        link_ptr = link_ptr->linkage;
    } 
}


// ---------------------------------------------------------------------------
void display_children(short level, const Class_relations* parent_ptr,
 char only_having)
{
    Class_linkage* link_ptr = (Class_linkage*)&parent_ptr->children();
    Class_relations* crel_ptr;
    short   indent = level * tab;
    short   scan_lvl;
    short   next_level = level;
    
    while ((void*)link_ptr != NULL)
    {
        crel_ptr = link_ptr->relations;
        if (only_having == '\0') link_ptr->relations = NULL;
//      link_ptr->relations = NULL;
        link_ptr = link_ptr->linkage;
   
        if ((void*)crel_ptr != NULL && crel_ptr != parent_ptr)
        {   
            const char* name = crel_ptr->name();
            if (strchr(name, only_having))
            {
                for (scan_lvl = 0; scan_lvl < level &&
                    levels[scan_lvl] != name; scan_lvl++);

                levels[level] = crel_ptr->name();
            
                indent_text[indent-1] = 
                    ((void*)link_ptr != NULL)? more_and_me[cset] : just_me[cset];
            
                printf("%.*s %s", indent, indent_text, name);
            
                if (show_multiple_parents == TRUE)
                    display_other_parents(parent_ptr, &crel_ptr->parents());
            
                if (scan_lvl < level)
                {
                    printf(" <Recursion>\n");
                    indent_text[indent-1] = ' ';
                    break;
                }
                
                if (scan_lvl == MAX_LEVELS-1)
                {
                    printf(" <Too deep>\n");
                    indent_text[indent-1] = ' ';
                    break;
                }
                putchar('\n');
            
                indent_text[indent-1] = 
                    ((void*)link_ptr != NULL)? more[cset] : ' ';
                    
                next_level = level + 1;                    
            }                
            
            display_children(next_level, crel_ptr, only_having);                
        }
    } 
    
//  parent_ptr->remove_children();
}

// ---------------------------------------------------------------------------
void display_dependences(Class_list& list, WWBoolean show_all, char only_having)
{
    Class_relations* crel_ptr;
    Class_relations* cprel_ptr;

    printf("\nTree of ");    
    if ((long)&list == (long)&flist)
        printf("File Needs\n");  
    else        
        printf("Class Needs\n");  
      
    for (short idx = 0; idx < list.count(); idx++)
    {
        crel_ptr = *(Class_relations**)list[idx];
        cprel_ptr= crel_ptr->parents().relations;
        
        if (show_all || (void*)cprel_ptr == NULL)
        {
            // Got top of the tree.
            
            printf("%s\n", (const char*)crel_ptr->name());
            
            levels[0] = crel_ptr->name();
            display_children(1, crel_ptr, only_having);
        }            
    }
}

#if 0
// ---------------------------------------------------------------------------
void add_file_needs(Class_relations* frel_ptr, Class_relations* nrel_ptr)
{
    const Class_linkage* link_ptr = &frel_ptr->children(); // needs
    Class_relations* crel_ptr;

    while ((void*)link_ptr != NULL)
    {
        crel_ptr = link_ptr->relations;
        
        if ((void*)crel_ptr != NULL && crel_ptr->name().length() > 0)
        {
//          crel_ptr = add_class(fdlist, crel_ptr->name());
        
            nrel_ptr->add_child(crel_ptr);
            crel_ptr->add_parent(nrel_ptr);
        }
    
        link_ptr = link_ptr->linkage;
    } 
}

// ---------------------------------------------------------------------------
void build_file_dependences(void)
{
    Class_relations* frel_ptr;
    Class_relations* nrel_ptr;

    for (short idx = 0; idx < flist.count(); idx++)
    {
        frel_ptr = *(Class_relations**)flist[idx];
        
        nrel_ptr = add_class(fdlist, frel_ptr->name());
        
        add_file_needs(frel_ptr, nrel_ptr);
    }
}
#endif


// ---------------------------------------------------------------------------
void display_classes(Class_list& list)
{
    Class_relations* crel_ptr;
    short idx;

    if ((long)&list == (long)&flist)
        printf("\nFiles\n");  
    else        
        printf("\nClasses\n");  
      
    for (idx = 0; idx < list.count(); idx++)
    {                                  
        crel_ptr = *(Class_relations**)list[idx];
        printf("%5u: %s\n", idx, (const char*)crel_ptr->name());
    }
}

// ---------------------------------------------------------------------------
void release_links(Class_linkage* first_link)
{
    Class_linkage*  last_link;
    Class_linkage*  prev_link;
    
    do
    {
        last_link = first_link;
        prev_link = first_link;
        
        while (last_link->linkage != NULL)
        {
            prev_link = last_link;
            last_link = last_link->linkage;
        }
        
        if (last_link != first_link)
        {
            delete last_link;
            prev_link->linkage = NULL;            
        }
        
    } while(last_link != first_link);
    
}

// ---------------------------------------------------------------------------
void release_clist(Class_list& clist)
{
    Class_relations* crel_ptr;
    
    for (short idx = 0; idx < clist.count(); idx++)
    {
        crel_ptr = *(Class_relations**)clist[idx];
        release_links((Class_linkage*)&crel_ptr->children());
        release_links((Class_linkage*)&crel_ptr->parents());
        delete crel_ptr;
    }
}

// ---------------------------------------------------------------------------
void process_section(const WWString& section_name, ifstream& in)
{
    Pattern     reference_p(
        "[A-Za-z0-9_]*__`[0-9][0-9]*`[A-Z][A-Za-z0-9_]*` *@|[ 0-9]*@|extern@|`");
    Pattern     possession_p("?*@|.(text|data)");
    Class_relations* crel_ptr = (Class_relations*)NULL;    
    WWString    item;
    Int_string  numstr;
    Match       match, match2;
    char        line[256];
    short       poss_need;
    static char* poss_need_str[] = {" provides ", " needs "};

    
    while (!in.eof())
    {  
        in.getline(line, sizeof(line));
        if (line[0] == 0 || line[0] == ' ') return;
        
        if (match.match(reference_p, line) == TRUE)                
        {
            if ((void*)crel_ptr == NULL)
            {
                crel_ptr = add_class(flist, section_name);
            }                
            
            match.extract_into(numstr, 0, 1);
            match.extract_into(item, 1, 2);
            item.truncate((int)(long)numstr);
            
            short end_idx = match.location(3);
            
            if (possession_p.compare(line, end_idx) == TRUE)   
            {    
                poss_need = 0;
                add_provides(crel_ptr, item);
            }
            else                    
            {
                poss_need = 1;
                add_needs(crel_ptr, item);
            }    
            
            if (show_possessions)
            cout << section_name << poss_need_str[poss_need] << item << endl;
        }                    
    }                    
}

// ---------------------------------------------------------------------------
void find_file_references(const char* filename)
{               
    Pattern     class_p("`[A-Za-z0-9_.]*` *@| *@| file ");
    Match       match;     
    WWString    section_name;
    ifstream    in;
    char        line[256];
    
    in.open(filename);
    if (in.good())
    {
        while (!in.eof())
        {  
            in.getline(line, sizeof(line));
            if (match.match(class_p, line) == TRUE)                
            {
                match.extract_into(section_name, 0, 1);
                process_section(section_name, in);
            }                    
        }                    
        in.close();
    }
    else
    {
        cerr << "Classtree: Unable to open " << filename << endl;
    }
}


// ---------------------------------------------------------------------------
void main(int argc, char* argv[])
{       
    WWTime  time;
    time.load_current_time();
     
    if (argc == 1)
    {
        cerr << "\n" << argv[0] << ":  " << time.time_string() << "\n"
             << "\nDes: Generate class dependence tree" 
                "\nUse: Classtree [-+ntpgxs] header_files...\n"
                "\nSwitches (*=default)(-=off, +=on):"
                "\n  m  = Show multiple (needs)"
                "\n  ln = List classes alphabetically"
                "\n  lf = List files alphabetically"
                "\n* t  = Show class dependency tree"
                "\n  o  = Show possessions"
                "\n"   
                "\n  p  = Add HP prefix for printing"
                "\n* g  = Use graphics for tree connections"
                "\n  x  = Use (+|-) for tree connections"
                "\n  s  = Use spaces for tree connections"
             << endl;
    }
    else
    {
        for (short argn = 1; argn < argc; argn++)
        {
            if (*argv[argn] == '-' || *argv[argn] == '+')
            {
                short polarity = (*argv[argn] == '+');
                
                switch (tolower(argv[argn][1]))
                {
                  case 'm': show_multiple_parents = polarity; break;
                  case 'o': show_possessions = polarity; break;
                  case 'p': print     = polarity;       break;
                  case 'g': cset      = GRAPHICS_CHAR;  break;
                  case 'x': cset      = TEXT_CHAR;      break;
                  case 's': cset      = SPACE_CHAR;     break;
                  case 'l': 
                    switch (tolower(argv[argn][2]))
                    {
                      case 'f':    list_f_classes = polarity;       break;
                      case 'n':    
                      default:
                                   list_n_classes = polarity;      break;
                    }                                    
                  case 't': 
                    switch (tolower(argv[argn][2]))
                    {
                      case 'f':    show_f_tree = polarity;      break;
                      case 'n':    
                      default:
                                   show_n_tree = polarity;      break;
                    }                                    
                }
            }
            else
            {
                find_file_references(argv[argn]);
            }
        }            
        
        if (print) printf("\x1b(10U\x1b(s0p10.00h12.0v0s0b3T");
        
        printf("%s: %s\n\n"
            "Multiple inheritance is designated by following the class name\n"
            "with all additional parents inclosed in parenthesis\n\n", 
            argv[0], (const char*)time.time_string());
        
                    
        if (list_n_classes) display_classes(nlist);
        if (show_n_tree) display_dependences(nlist, FALSE, '\0');
        
        if (list_f_classes) display_classes(flist);
        if (show_f_tree) display_dependences(flist, TRUE, '.');
        
#if 0        
        build_file_dependences();
        if (list_fd_classes) display_classes(fdlist);
        if (show_fd_tree) display_dependences(fdlist);
#endif        
        // release_clist();
    }
}
