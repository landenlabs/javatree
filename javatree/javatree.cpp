//-------------------------------------------------------------------------------------------------
//
//  JavaTree.cpp      5/8/2015        DALang
//
// Based off of:
//  Classtree.C       09/10/92        DALang
//
//  Parse Java files and generate class names and class dependence tree.
//
//-------------------------------------------------------------------------------------------------
//
// Author: Dennis Lang - 2015
// http://landenlabs.com/
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

// 4291 - No matching operator delete found
#pragma warning( disable : 4291 )

#include <stdio.h>
#include <ctype.h>
#include <fstream>
#include <iostream>
#include <iomanip>

#include "ll_stdhdr.h"
#include "class_rel.h"
#include "directory.h"
#include "javaReader.h"
#include "SwapStream.h"
#include "split.h"
#include "javaTree.h"

#include <vector>
#include <map>
#include <algorithm>
#include <regex>
using namespace std;

typedef std::map<lstring, ClassRelations*> ClassList;
typedef std::vector<lstring> StringList;
ClassList clist;

// Runtime switches

enum {GRAPHICS_CHAR=0, TEXT_CHAR=1, SPACE_CHAR=2, HTML_CHAR=3, JAVA_CHAR=4, VIZ_CHAR=5 };

bool show_names = false;            
bool show_tree  = true;
bool print      = false;
bool vizSplit   = false;
bool needHeader = true;
bool allClasses = false;
bool importPackage = false;
bool tabularList = false;
int cset        = GRAPHICS_CHAR;
int nodesPerFile = 0;

lstring outPath;
lstring codePath;
lstring title;
lstring graphName;
std::ofstream outStream;

// Display stuff
typedef std::vector<lstring> Indent;
typedef std::vector<std::regex> PatternList;

#ifdef HAVE_WIN
static char SLASH_CHR      = '\\';
#else
static char SLASH_CHR      = '/';
#endif
                                // graph     text    spaces   html
static lstring none[]         = {"    ",    "    ", "    ", "<img src='n.png'>", ""};
static lstring more[]         = {"   \xb3", "   |", "    ", "<img src='0.png'>", ""};
static lstring more_and_me[]  = {"   \xc3", "   +", "    ", "<img src='2.png'>", ""};
static lstring just_me[]      = {"   \xc0", "   -", "    ", "<img src='1.png'>", ""};

static lstring dc("\nClass Tree\n");
static lstring cc(": ");
static lstring doc_begin[]        = {""  , ""  , ""  , "<html>", ""};
static lstring doc_classes[]      = {dc  , dc  , dc  , "<table>", ""};
static lstring doc_classesChild[]  = {cc  , cc  , cc  , "<td>", ""};
static lstring doc_classesBLine[] = {""  , ""  , ""  , "<tr><td>", ""};
static lstring doc_classesELine[] = {"\n", "\n", "\n", "</tr>\n", ""};
static lstring doc_end[]          = {""  , ""  , ""  , "</table></html>", ""};

static int sNodeNum = 1;
static const char sDot[] = ".";
static const char sNL[] = "\\n";
static const lstring sPackageTag("package");
static const lstring sFileTag("file");

// ---------------------------------------------------------------------------
// Output GraphViz header.
void outVizHeader()
{
    needHeader = false;
    cout << "digraph " << graphName << " {\n"
        "bgcolor=transparent\n"
        "overlap=false;\n"
        "label=\"" << title << " Class Hierarchy - Dennis Lang & GraphViz\";\n"
        "fontsize=12;\n"
        "node [shape=box,style=filled,fillcolor=white];\n";
}

// Complete GraphViz output file.
void outVizTrailer()
{
    cout << "}\n";
}

// ---------------------------------------------------------------------------
// Add a class to list
ClassRelations* AddClass(
    const lstring& class_name, 
    const lstring& class_modifier, 
    const lstring& filename)
{
    ClassList::const_iterator iter = clist.find(class_name);
    ClassRelations* pCrel;

    if (iter == clist.end())
    {
        pCrel = new ClassRelations(class_name, class_modifier, filename);
        clist.insert(std::make_pair(class_name, pCrel));
    }
    else
    {
        pCrel = iter->second;
        pCrel->file(filename);
    }
    
    return pCrel;
}

// ---------------------------------------------------------------------------
// Add a parent class to list
void add_parent(
    ClassRelations* pChild, 
    const lstring& parents_name,
    const lstring& filename)
{
    static const lstring modifier;
    ClassRelations* pSuper = AddClass(parents_name, modifier, filename);

    pChild->add_parent(pSuper);
    pSuper->add_child(pChild);
}

// ---------------------------------------------------------------------------
void display_other_parents(
    const ClassRelations* parent_ptr, 
    const ClassLinkage* first_parent)
{
    const ClassLinkage* link_ptr = first_parent;
    ClassRelations* crel_ptr;

    while (link_ptr != NULL)
    {
        crel_ptr = link_ptr->relations;
        if (crel_ptr != NULL && crel_ptr != parent_ptr)
        {   
            lstring name = crel_ptr->name();
            printf("  (%s)", name.c_str());
        }
                
        link_ptr = link_ptr->linkage;
    } 
}

// ---------------------------------------------------------------------------
void  print_indent(const Indent& indent)
{
    for (size_t i=0; i < indent.size(); i++)
    {
        printf("%s", indent[i].c_str());
    }
}

// ---------------------------------------------------------------------------
void display_children(Indent& indent, size_t width, const ClassRelations* parent_ptr)
{
    const ClassLinkage* link_ptr = &parent_ptr->children();
    ClassRelations* crel_ptr;

    while (link_ptr != NULL)
    {
        crel_ptr = link_ptr->relations;
        link_ptr = link_ptr->linkage;
        
        if (crel_ptr != NULL)
        {   
            lstring name = crel_ptr->name();
            indent.push_back(
                (link_ptr != NULL)? more_and_me[cset] : just_me[cset]);
           
            fputs(doc_classesBLine[cset].c_str(), stdout);
            printf("%*.*s%s", 
                (int)width, (int)width,
                crel_ptr->file().c_str(),
                doc_classesChild[cset].c_str()
                ); 
            print_indent(indent);
            printf(" %s", name.c_str());
                
            display_other_parents(parent_ptr, &crel_ptr->parents());
            fputs(doc_classesELine[cset], stdout);

            indent.pop_back();
            indent.push_back(
                (link_ptr != NULL)? more[cset] : none[cset]);
            
            display_children(indent, width, crel_ptr); 
            indent.pop_back();
        }
    } 
}

// ---------------------------------------------------------------------------
size_t display_interfaces(int parentNum, size_t width, const ClassRelations* parent_ptr)
{
    size_t nodeCnt = 0;
    if (parent_ptr != NULL)
    {
        // [color=red,penwidth=3.0]
        const ClassLinkage* interfaces_ptr = &parent_ptr->interfaces();
        const char* sep = "";
        while (interfaces_ptr != NULL &&  interfaces_ptr->relations != NULL)
        {
            ClassRelations* nextInterface_ptr = interfaces_ptr->relations;
            interfaces_ptr = interfaces_ptr->linkage;
            if (nextInterface_ptr->modifier() == sFileTag)
                continue;
            lstring name = nextInterface_ptr->name();
            if (cset == VIZ_CHAR)
            {
                cout << "\"" << name << "\"  [style=filled, fillcolor=yellow] \n";
                cout << "\"" << name << "\" -> \"" << parent_ptr->name() << "\" [color=red,penwidth=3.0] \n";
            }
            else
            {
                cout << sep << name;
                sep = ", ";
            }
            nodeCnt++;
        }
    }

    return nodeCnt;
}

// ---------------------------------------------------------------------------
size_t display_children(
    int parentNum, 
    size_t width, 
    const ClassRelations* parent_ptr,
    const ClassRelations* pparent_ptr)
{
    size_t nodeCnt = 0;
    const ClassLinkage* nextChild_ptr = &parent_ptr->children();
    ClassRelations* child_ptr;
    const char* parendModStr = "";
    const char* childModStr = "";

    if (cset == VIZ_CHAR)
    {
        nodeCnt += display_interfaces(parentNum, width, parent_ptr);

        if (pparent_ptr == NULL)
            parendModStr = " [fillcolor=cyan1]";
        else
            parendModStr = " [fillcolor=cyan4]";
    }
 
    // Check if any children.
    if (nextChild_ptr != NULL && nextChild_ptr->relations != NULL)
    {
        // Parent node with children - iterate over children
        while (nextChild_ptr != NULL)
        {
            child_ptr = nextChild_ptr->relations;
            nextChild_ptr = nextChild_ptr->linkage;

            if (child_ptr != NULL)
            {
                lstring chilNname = child_ptr->name();
                if (cset == VIZ_CHAR)
                {
                    ReplaceAll(chilNname, sDot, sNL);
                    lstring parentName = parent_ptr->name();
                    ReplaceAll(parentName, sDot, sNL);

                    if (parent_ptr->modifier().find("abstract") != -1)
                        parendModStr = (pparent_ptr == NULL) ? " [color=green] " : " [fillcolor=chartreuse] ";
                    if (child_ptr->modifier().find("abstract") != -1)
                        childModStr = (parent_ptr == NULL) ? " [color=green] " : " [fillcolor=chartreuse] ";
                    if (child_ptr->modifier().find("public") == -1)
                        childModStr = " [color=red] ";

                    if (*parendModStr != 0)
                        cout << "\"" << parentName << "\" " << parendModStr << std::endl;
                    if (*childModStr != 0)
                        cout << "\"" << chilNname << "\" " << childModStr << std::endl;
                    cout << "\"" << parentName << "\" -> \"" << chilNname << "\"\n";
                    parendModStr = "";
                }
                else
                {
                    cout << "d.add(" << sNodeNum;
                    cout << "," << parentNum << ",'" << ReplaceAll(chilNname, "<", "&lt;")
                        << "','" << child_ptr->file() << "');\n";
                }
                nodeCnt++;
                nodeCnt += display_children(sNodeNum++, width, child_ptr, parent_ptr);
            }
        }
    }
    else
    {
        // Single node - no children
        lstring name = parent_ptr->name();
        if (cset == VIZ_CHAR)
        {
            if (importPackage)
            {
                if (parent_ptr->parents().relations == NULL)
                    cerr << "Isolated package " << name << std::endl;
            }
            else
            {
                ReplaceAll(name, sDot, sNL);
                if (parent_ptr->modifier().find("abstract") != -1)
                    childModStr = (parent_ptr == NULL) ? " [color=green] " : " [fillcolor=chartreuse] ";
                if (parent_ptr->modifier().find("public") == -1)
                    childModStr = " [color=red] ";

                if (*childModStr != 0)
                    cout << "\"" << name << "\" " << childModStr << std::endl;
                cout << "\"" << name << "\"\n";

                nodeCnt++;
            }
        }
        else
        {
        }
    }
    return nodeCnt;
}


// ---------------------------------------------------------------------------
size_t count_interfaces(const ClassRelations* parent_ptr)
{
    size_t nodeCnt = 0;
    if (parent_ptr != NULL)
    {
        const ClassLinkage* interfaces_ptr = &parent_ptr->interfaces();

        while (interfaces_ptr != NULL &&  interfaces_ptr->relations != NULL)
        {
            ClassRelations* nextInterface_ptr = interfaces_ptr->relations;
            interfaces_ptr = interfaces_ptr->linkage;
            if (nextInterface_ptr->modifier() == sFileTag)
                continue;

            nodeCnt++;
        }
    }

    return nodeCnt;
}

// ---------------------------------------------------------------------------
size_t count_children(
    const ClassRelations* parent_ptr,
    const ClassRelations* pparent_ptr)
{
    size_t nodeCnt = 0;
    const ClassLinkage* link_ptr = &parent_ptr->children();

    if (cset == VIZ_CHAR)
        nodeCnt += count_interfaces(parent_ptr);

    while (link_ptr != NULL)
    {
        ClassRelations* crel_ptr = link_ptr->relations;
        link_ptr = link_ptr->linkage;

        if (crel_ptr != NULL)
            nodeCnt += 1 + count_children(crel_ptr, parent_ptr);
    }

    return nodeCnt;
}


// ---------------------------------------------------------------------------
// Return true when time to make a new output file.
//   * no output file
//   * -Z split graphviz by subtree
//   * -N split by nodes per file.
bool NextFile(std::ostream& out, size_t& nodeCnt, size_t nextNodeCnt)
{
    bool next = false;
    if (outPath.empty())
        return false;
    if (!out.good() || nodeCnt == -1)
        next = true;
    if (vizSplit)
        next = true;
    if (nodesPerFile != 0 && nodeCnt * 2 > nodesPerFile && nodeCnt + nextNodeCnt/2 > nodesPerFile)
        next = true;

    if (next)
        nodeCnt = 0;

    return next;
}

// ---------------------------------------------------------------------------
void display_dependences(void)
{
    ClassRelations* crel_ptr;
    ClassRelations* cprel_ptr;

    if (cset != VIZ_CHAR)
        fputs(doc_classes[cset], stdout);

    ClassList::const_iterator iter;

    size_t fileWidth = 14;
    size_t nameWidth = 14;

    // typedef std::map<size_t, ClassRelations*> SortNodeList;
    // SortNodeList sortedList;

    for (iter = clist.begin(); iter != clist.end(); iter++)
    {
        crel_ptr = iter->second;
        cprel_ptr= crel_ptr->parents().relations;
       
        fileWidth = max(fileWidth, crel_ptr->file().length());
        nameWidth = max(nameWidth, crel_ptr->name().length());
        // sortedList.insert(std::make_pair(count_children(crel_ptr, NULL), crel_ptr));
    }

    SwapStream swapStream(cout);

    size_t nodeCnt = -1;
    for (iter = clist.begin(); iter != clist.end(); iter++)
    {
        crel_ptr = iter->second;
        cprel_ptr= crel_ptr->parents().relations;
        
        if (cprel_ptr == NULL)  // Find Super class (no parent)
        {
            // Have super class - now display subclasses.
     
            if (cset == VIZ_CHAR)
            {
                if (NextFile(outStream, nodeCnt, count_children(crel_ptr, NULL)))
                {
                    if (outStream.good())
                    {
                        outVizTrailer();
                        swapStream.restore();
                        outStream.close();
                    }
                    needHeader = true;
                    lstring outFile = outPath + crel_ptr->name() + ".gv";
                    regex dosSpecial("[<,>?]");
                    outFile = std::regex_replace(outFile, dosSpecial, "_");
                    outStream.open(outFile);
                    if (outStream.good())
                        swapStream.swap(outStream);
                    else
                        std::cerr << "Failed to open " << outFile << std::endl;
                }
               
                if (needHeader)
                    outVizHeader();

                // cout << crel_ptr->name() << " -> " <<   crel_ptr->file() << std::endl;
                nodeCnt += display_children(sNodeNum , fileWidth, crel_ptr, NULL);
                // display_interfaces(sNodeNum , fileWidth, crel_ptr);
                sNodeNum++;
            }
            else if (cset == JAVA_CHAR)
            {
                cout << "d.add(" << sNodeNum;
                lstring name = crel_ptr->name();
                cout << "," << 0 << ",'" <<  ReplaceAll(name, "<", "&lt;") 
                    << "','" << crel_ptr->file() << "');\n";

                display_children(sNodeNum++, fileWidth, crel_ptr, NULL);
            }
            else
            {
                fputs(doc_classesBLine[cset], stdout);
                printf("%*.*s%s %s",       
                    (int)fileWidth, (int)fileWidth,
                    crel_ptr->file().c_str(),
                    doc_classesChild[cset].c_str(),
                    crel_ptr->name().c_str()
                    );
                fputs(doc_classesELine[cset], stdout);
                Indent indent;
                display_children(indent, fileWidth, crel_ptr);
            }
        }            
    }

    if (cset == VIZ_CHAR)
    {
        if (outStream.good())
        {
            outVizTrailer();
            swapStream.restore();
            outStream.close();
        }
    }
}


// ---------------------------------------------------------------------------
void display_names(void)
{
    ClassRelations* crel_ptr;

    std::cout << "ClassName\tModifiers\tFirstParent\tFirstChild\tInterfaces\tFile\n";
    ClassList::const_iterator iter;
    for (iter = clist.begin(); iter != clist.end(); iter++)
    {
        crel_ptr = iter->second;
        std::cout << crel_ptr->name() << "\t" << crel_ptr->modifier();
        std::cout << "\t";
        if (crel_ptr->parents().relations != NULL)
            std::cout << crel_ptr->parents().relations->name(); // TODO - display parent count
        else
            std::cout << "_NoParent_";
        std::cout << "\t";
        if (crel_ptr->children().relations != NULL)
            std::cout << crel_ptr->children().relations->name(); // TODO - display childre count
        else
            std::cout << "_NoChildren_";
        std::cout << "\t";
        display_interfaces(0, 0, crel_ptr);
        std::cout << "\t" << crel_ptr->file();
        std::cout << std::endl;
    }
}


// ---------------------------------------------------------------------------
void release_links(ClassLinkage* first_link)
{
    ClassLinkage*  last_link;
    ClassLinkage*  prev_link;
    
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
// Release class list children and parents.
void Release_clist()
{
    ClassRelations* crel_ptr;
    
    ClassList::const_iterator iter;
    for (iter = clist.begin(); iter != clist.end(); iter++)
    {
        crel_ptr = iter->second;
        release_links((ClassLinkage*)&crel_ptr->children());
        release_links((ClassLinkage*)&crel_ptr->parents());
        release_links((ClassLinkage*)&crel_ptr->interfaces());
        delete crel_ptr;
    }
}


// ---------------------------------------------------------------------------
// Convert DOS filename to Browser url file locator. 
// Ex: \dir\subdir\file.ext => file:///dir/subdir/file.ext
lstring FilePathToURL(lstring filepath)
{
    static lstring  diskDrive("file:///");

    for (unsigned i = 0; i < filepath.length(); i++)
        if (filepath[i] == '\\')
            filepath[i] = '/';

    return diskDrive + filepath;
}

// ---------------------------------------------------------------------------
// Function used with Split object to perform custom split to handle nested
// Java Generic (template) typing.   ex:  foo<bar<car>>>
static size_t FindSplit(const lstring& str, const char* delimList, size_t begIdx)
{
    int depth = 0;
    for (size_t idx = begIdx; idx < str.length(); idx++)
    {
        const char c = str[idx];
        if (c == '<')
            depth++;
        if (c == '>')
            depth--;
        if (depth == 0) 
        {
            for (const char* cptr = delimList; *cptr != '\0'; cptr++)
            {
                if (c == *cptr)
                    return idx;
            }
        }
    }

    return lstring::npos;
}

// ---------------------------------------------------------------------------
// Return Full class name by prepending base class names. Ex: parentName.childName
lstring&  MakeFullClassName(lstring& outFullName, const StringList& inNames, const lstring& name)
{
    if (inNames.empty())
        outFullName = name;
    else
    {
        outFullName.clear();
        const lstring dot = sDot;
        for (unsigned idx = 0; idx != inNames.size(); idx++)
        {
            if (inNames[idx].length() == 0)
                cerr << "Bad class " << name << std::endl;
            else
                outFullName += inNames[idx] + dot;
        }
        outFullName += name;
    }

    return outFullName;
}

struct TableItem
{
    lstring package;
    lstring className;
    lstring fullClassName;
    lstring modifier;
    lstring filename;
};
typedef std::vector<TableItem> TableList;

TableList tableList;

// ---------------------------------------------------------------------------
void outputHtmlTableList()
{
    time_t nowTime;
    time(&nowTime);

    std::cout << 
        "<table id='gradient-style' summary='display' ellspacing='0' width='100%'  >\n"
        "<thead>\n"
        "<tr>\n"
        "<th scope='col'>Pakcage</th> \n"
        "<th scope='col'>FullClassName</th> \n"
        "<th scope='col'>ClassName</th> \n"
        "<th scope='col'>Modifiers</th> \n"
        "<th scope='col'>Filename</th> \n"
        "</tr>\n"
        "</thead>\n"
        "<tfoot>\n"
        "<tr>\n"
        "<td colspan='5'>Class List by Dennis Lang " << ctime(&nowTime) << "</td> \n"
        "</tr> \n"
        "</tfoot> \n"
        "<tbody>\n";
        
    for (unsigned idx = 0; idx != tableList.size(); idx++)
    {
        TableItem& item = tableList[idx];
        std::cout << "<tr>"
            << " <td>" << item.package
            << " <td>" << item.fullClassName
            << " <td>" << item.className
            << " <td>" << item.modifier
            << " <td>" << item.filename
            << std::endl;
    }

    std::cout  << 
        "</tbody>\n"
        "</table>\n";

}

// ---------------------------------------------------------------------------
bool TabularListOfInFile(const char* filepath)
{
    // private static final class LoaderReference extends WeakReference<ClassLoader>
    // private static final class CacheKey implements Cloneable {
    // final class EntryIterator extends PrivateEntryIterator<Map.Entry<K,V>> {
    // public final class Formatter implements Closeable, Flushable {
    // static final class Entry<K,V> implements Map.Entry<K,V>
    // public abstract class Calendar implements Serializable, Cloneable, Comparable<Calendar> {
    // public abstract class AbstractSet<E> extends AbstractCollection<E> implements Set<E> {
    // static class MutablePair<F, S> {
    // public abstract class UnselectableArrayAdapter<E> extends ArrayAdapter<E> {
    // public class Garage<X extends Vehicle> { 
    // class BST<X extends Comparable<X>> {
    // int totalFuel(List<? extends Vehicle> list) { 

    
    static std::regex  pubClass_p("[ \t]*(((public|abstract|final|static)[ \t]+)*)class[ \t]+([A-Za-z].*)");
    static std::regex  allClass_p("[ \t]*(((public|protected|private|abstract|final|static)[ \t]+)*)class[ \t]+([A-Za-z].*)");
    static std::regex  extentds_p("extends[ \t]+([A-Za-z][A-Za-z0-9_]+)");
    static std::regex  implements_p("implements[ \t]+([A-Za-z0-9_., <>]+)");
    static std::regex package_p("package[ \t]+([A-Za-z0-9_.]+);");

    std::smatch     matchs;
    lstring     full_class_name;
    lstring     class_name;
    lstring     class_modifier;
    lstring     filename;
    lstring     packageName;
    ifstream    in;
    lstring     line;
    lstring     line2;
    JavaReader  reader;

    std::regex& class_p = allClasses ? allClass_p : pubClass_p;

    if (strstr(filepath, ".java") == NULL)
        return false;   // Ignore non-java files.

    try {
        in.open(filepath);
        if (in.good())
        {
            if (cset == JAVA_CHAR)
            {
                filename = FilePathToURL(filepath);
            }
            else
            {
                const char* slashPos = strrchr(filename, SLASH_CHR);
                if (slashPos != NULL)
                    filename = slashPos + 1;
                else
                    filename = filepath;
            }

            int depth = 0;
            StringList classNames;
            const char* baseName = strrchr(filepath, SLASH_CHR);
            class_name = (baseName != NULL) ? baseName + 1 : filepath;
            class_name.resize(class_name.find_last_of('.'));

            // std::cout << "<html>\n<body>\n<table>\n";

            // Read input file lines.
            while (reader.getJavaline(in, line).good())
            {
                size_t rLen = line.length();
                if (rLen >= 4096 || rLen <= 0)
                    continue;   // probably not a valid line.

                // Locate class 
                if (std::regex_match(line, matchs, package_p, std::regex_constants::match_default))
                {
                    packageName = std::string(matchs[1]);
                }
                else if (std::regex_match(line, matchs, class_p, std::regex_constants::match_default))
                {
                    size_t endIdx = line.find_first_of(";{");
                    char endChar = ' ';
                    if (endIdx != std::string::npos)
                        endChar = line[endIdx];
                    else
                        endIdx = line.length();

                    if (endChar == ';')
                        continue;   // Ignore empty classes

                    class_modifier = std::string(matchs[1]);
                    lstring rightClass = std::string(matchs[4]);
                    rightClass.resize(endIdx - matchs.position(4));

                    //   className < G1 , G2 >   =>  className<G1,G2>
                    ReplaceAll(rightClass, "\t", " ");
                    std::regex spaceSpecialRe(" *([<>,])");
                    std::regex specialSpaceRe("([<,]) *");
                    // std::regex_constants::match_flag_type flags = std::regex_constants::match_default;
                    rightClass = std::regex_replace(rightClass, spaceSpecialRe, "$1");
                    rightClass = std::regex_replace(rightClass, specialSpaceRe, "$1");

                    Split split(rightClass, " ", FindSplit);
                    class_name = split[0];

                    if (allClasses || class_modifier.find("public") != string::npos)
                    {
                        MakeFullClassName(full_class_name, classNames, class_name);
                        // crel_ptr = AddClass(full_class_name, class_modifier, filename);
                        TableItem item;
                        item.className = class_name;
                        item.fullClassName = full_class_name;
                        item.package = packageName;
                        item.modifier = class_modifier;
                        item.filename = filename;

                        tableList.push_back(item);
#if 0
                        std::cout
                            << "<tr>"
                            << " <td>" << packageName
                            << " <td>" << class_name
                            << " <td>" << full_class_name
                            << " <td>" << class_modifier
                            << " <td>" << filename
                            << std::endl;
#endif
                           
                        if (endChar != '{')
                        {
                            lstring moreText;
                            while (reader.getJavaline(in, moreText).good())
                            {
                                line += moreText;
                                if (moreText.find_first_of(";{") != std::string::npos)
                                    break;
                            }
                        }

                        {
                            lstring parents = rightClass.substr(class_name.length());
                            parents = std::regex_replace(parents, spaceSpecialRe, "$1");
                            parents = std::regex_replace(parents, specialSpaceRe, "$1");
                            Split split(parents, ", ", FindSplit);

                            int modType = 0;    // 0=none, 1=extends, 2=implements
                            for (size_t splitIdx = 0; splitIdx != split.size(); splitIdx++)
                            {
                                const lstring& token = split[splitIdx];
                                if (token == "extends")
                                {
                                    modType = 1;
                                    continue;
                                }
                                else if (token == "implements")
                                {
                                    modType = 2;
                                    continue;
                                }
                                else if (modType != 0)
                                {
                                    switch (modType)
                                    {
                                    case 1: // extends
                                        // add_parent(crel_ptr, token, "_no_file_");
                                        break;
                                    case 2: // implements
                                        // crel_ptr->add_interface(new ClassRelations(token, "interface", "_no_file_"));
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                // else if (line.find("class") != -1)
                //      cerr << filepath << ":" << line << std::endl;   // Debug - show missing lines.

                for (size_t idx = 0; idx != line.length(); idx++)
                {
                    char c = line[idx];
                    if (c == '{')
                    {
                        classNames.resize(depth);
                        classNames.push_back(class_name);
                        depth++;
                    }
                    else if (c == '}')
                    {
                        depth--;
                        if (depth < 0)
                            std::cerr << " Brace parsing error on " << filepath << std::endl;
                        else
                            classNames.resize(depth);
                    }
                }
            }
            in.close();

            // std::cout << "\n</table>\n</body>\n</html>\n";
            return true;
        }
        else
        {
            cerr << "Classtree: Unable to open " << filepath << endl;
        }
    }
    catch (exception ex)
    {
        cerr << "Parsing error in file:" << filepath << endl;
        cerr << ex.what() << std::endl;
    }
    return false;
}

// ---------------------------------------------------------------------------
bool hasExtension(const lstring& filepath, const char* extn)
{
    char* found = strstr(filepath, extn);
    if (found != NULL)
    {
        return (found[strlen(extn)] == '\0');
    }
    return false;
}

// ---------------------------------------------------------------------------
bool FindClassDefsInFile(const char* filepath)
{
    // private static final class LoaderReference extends WeakReference<ClassLoader>
    // private static final class CacheKey implements Cloneable {
    // final class EntryIterator extends PrivateEntryIterator<Map.Entry<K,V>> {
    // public final class Formatter implements Closeable, Flushable {
    // static final class Entry<K,V> implements Map.Entry<K,V>
    // public abstract class Calendar implements Serializable, Cloneable, Comparable<Calendar> {
    // public abstract class AbstractSet<E> extends AbstractCollection<E> implements Set<E> {
    // static class MutablePair<F, S> {
    // public abstract class UnselectableArrayAdapter<E> extends ArrayAdapter<E> {
    // public class Garage<X extends Vehicle> { 
    // class BST<X extends Comparable<X>> {
    // int totalFuel(List<? extends Vehicle> list) { 

    static std::regex  pubClass_p("[ \t]*(((public|abstract|final|static)[ \t]+)*)class[ \t]+([A-Za-z].*)");
    static std::regex  allClass_p("[ \t]*(((public|protected|private|abstract|final|static)[ \t]+)*)class[ \t]+([A-Za-z].*)");
    static std::regex  extentds_p("extends[ \t]+([A-Za-z][A-Za-z0-9_]+)");
    static std::regex  implements_p("implements[ \t]+([A-Za-z0-9_., <>]+)");
    static std::regex package_p("package[ \t]+([A-Za-z0-9_.]+);");

    std::smatch     matchs;
    lstring     full_class_name;
    lstring     class_name;
    lstring     class_modifier;
    lstring     filename;
    lstring     packageName;
    ifstream    in;
    lstring     line;
    lstring     line2;
    ClassRelations* crel_ptr = NULL;
    JavaReader  reader;

    std::regex& class_p = allClasses ? allClass_p : pubClass_p;
 
    if (!hasExtension(filepath, ".java"))
        return false;   // Ignore non-java files.

    try {
        in.open(filepath);
        if (in.good())
        {
            if (cset == JAVA_CHAR)
            {
                filename = FilePathToURL(filepath);
            }
            else
            {
                const char* slashPos = strrchr(filename, SLASH_CHR);
                if (slashPos != NULL)
                    filename = slashPos + 1;
                else
                    filename = filepath;
            }

            int depth = 0;
            StringList classNames;
            const char* baseName = strrchr(filepath, SLASH_CHR);
            class_name = (baseName != NULL) ? baseName + 1 : filepath;
            class_name.resize(class_name.find_last_of('.'));

            // Read input file lines.
            while (reader.getJavaline(in, line).good())
            {
                size_t rLen = line.length();
                if (rLen >= 4096 || rLen <= 0)
                    continue;   // probably not a valid line.

                // Locate class 
#if 0
                if (std::regex_match(line, matchs, package_p, std::regex_constants::match_default))
                {
                    packageName = std::string(matchs[1]);
                }
                else 
#endif
                   
                    if (std::regex_match(line, matchs, class_p, std::regex_constants::match_default))
                {
                    size_t endIdx = line.find_first_of(";{");
                    char endChar = ' ';
                    if (endIdx != std::string::npos)
                        endChar = line[endIdx];
                    else
                        endIdx = line.length();

                    if (endChar == ';')
                        continue;   // Ignore empty classes

                    class_modifier = std::string(matchs[1]);
                    lstring rightClass = std::string(matchs[4]);
                    rightClass.resize(endIdx - matchs.position(4));

                    //   className < G1 , G2 >   =>  className<G1,G2>
                    ReplaceAll(rightClass, "\t", " ");
                    std::regex spaceSpecialRe(" *([<>,])");
                    std::regex specialSpaceRe("([<,]) *");
                    // std::regex_constants::match_flag_type flags = std::regex_constants::match_default;
                    rightClass = std::regex_replace(rightClass, spaceSpecialRe, "$1");
                    rightClass = std::regex_replace(rightClass, specialSpaceRe, "$1");

                    Split split(rightClass, " ", FindSplit);
                    class_name = split[0];

                    if (allClasses || class_modifier.find("public") != string::npos)
                    {
                        MakeFullClassName(full_class_name, classNames, class_name);
                        crel_ptr = AddClass(full_class_name, class_modifier, filename);

                        if (endChar != '{')
                        {
                            lstring moreText;
                            while (reader.getJavaline(in, moreText).good())
                            {
                                line += moreText;
                                if (moreText.find_first_of(";{") != std::string::npos)
                                    break;
                            }
                        }

                        {
                            lstring parents = rightClass.substr(class_name.length());
                            parents = std::regex_replace(parents, spaceSpecialRe, "$1");
                            parents = std::regex_replace(parents, specialSpaceRe, "$1");
                            Split split(parents, ", ", FindSplit);

                            int modType = 0;    // 0=none, 1=extends, 2=implements
                            for (size_t splitIdx = 0; splitIdx != split.size(); splitIdx++)
                            {
                                const lstring& token = split[splitIdx];
                                if (token == "extends")
                                {
                                    modType = 1;
                                    continue;
                                }
                                else if (token == "implements")
                                {
                                    modType = 2;
                                    continue;
                                }
                                else if (modType != 0)
                                {
                                    switch (modType)
                                    {
                                    case 1: // extends
                                        add_parent(crel_ptr, token, "_no_file_");
                                        break;
                                    case 2: // implements
                                        crel_ptr->add_interface(new ClassRelations(token, "interface", "_no_file_"));
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                // else if (line.find("class") != -1)
                //      cerr << filepath << ":" << line << std::endl;   // Debug - show missing lines.

                for (size_t idx = 0; idx != line.length(); idx++)
                {
                    char c = line[idx];
                    if (c == '{')
                    {
                        classNames.resize(depth);
                        classNames.push_back(class_name);
                        depth++;
                    }
                    else if (c == '}')
                    {
                        depth--;
                        if (depth < 0)
                            std::cerr << " Brace parsing error on " << filepath << std::endl;
                        else
                            classNames.resize(depth);
                    }
                }
            }
            in.close();
            return true;
        }
        else
        {
            cerr << "Classtree: Unable to open " << filepath << endl;
        }
    }
    catch (exception ex)
    {
        cerr << "Parsing error in file:" << filepath << endl;
        cerr << ex.what() << std::endl;
    }
    return false;
}

// ---------------------------------------------------------------------------
bool FindImportPackageInFile(const char* filepath)
{

    /*
    package com.wsi.android.framework.map;

    import java.util.Map;
    import java.util.Set;

    import org.xml.sax.Attributes;

    import android.content.Context;
    import android.content.SharedPreferences;

    import com.google.android.gms.maps.model.LatLng;
    import com.wsi.android.framework.R;
    */
    static std::regex import_p("(package|import) ([A-Za-z0-9_.]+);");

    std::smatch     matchs;
    lstring         filename;
    ifstream        in;
    lstring         line;
    ClassRelations* child_ptr = NULL;
    ClassRelations* file_ptr = NULL;
    JavaReader      reader;
    lstring         packageName;
    lstring         empty;


    if (strstr(filepath, ".java") == NULL)
        return false;   // Ignore non-java files.

    try {
        in.open(filepath);
        if (in.good())
        {
            if (cset == JAVA_CHAR)
            {
                filename = FilePathToURL(filepath);
            }
            else
            {
                const char* slashPos = strrchr(filename, SLASH_CHR);
                if (slashPos != NULL)
                    filename = slashPos + 1;
                else
                    filename = filepath;
            }

            // Read input file lines.
            while (reader.getJavaline(in, line).good())
            {
                size_t rLen = line.length();
                if (rLen >= 4096 || rLen <= 0)
                    continue;   // probably not a valid line.

                // Locate class 
                if (std::regex_match(line, matchs, import_p, std::regex_constants::match_default))
                {
                    bool isPackage = matchs[1] == "package";
                    if (isPackage)
                    {
                        packageName = std::string(matchs[2]);
                        child_ptr = AddClass(packageName, sPackageTag, empty);

                        file_ptr = AddClass(filename, sFileTag, packageName);
                        child_ptr->add_interface(file_ptr);
                    }
                    else
                    {
                        lstring importName = std::string(matchs[2]);
                        if (importName.find(packageName) == 0)
                            continue;
                        if (importName.find("com.wsi.") != 0)
                            continue;
                       

#if 0
                        child_ptr = AddClass(packageName, empty, filename);
                        add_parent(child_ptr, importName, filename);
#else
                        child_ptr = AddClass(importName, packageName, filename);
                        add_parent(child_ptr, packageName, filename);
                        file_ptr->add_child(child_ptr);
#endif
                        // crel_ptr->add_interface(new ClassRelations(token, "interface", "_no_file_"));
                    }
                }
            }

            in.close();
            return true;
        }
        else
        {
            cerr << "Classtree: Unable to open " << filepath << endl;
        }
    }
    catch (exception ex)
    {
        cerr << "Parsing error in file:" << filepath << endl;
        cerr << ex.what() << std::endl;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Return true if inPath (filename part) matches pattern in patternList
bool FileMatches(const lstring& inPath, const PatternList& patternList)
{
    size_t nameStart = inPath.rfind(SLASH_CHR) + 1;
    if (patternList.empty() || nameStart == 0 || nameStart == inPath.length())
        return false;

    for (size_t idx = 0; idx != patternList.size(); idx++)
        if (std::regex_match(inPath.begin() + nameStart, inPath.end(), patternList[idx]))
            return true;

    return false;
}

// ---------------------------------------------------------------------------
static size_t FindClassDefinitions(const lstring& dirname, PatternList ignorePatterns)
{
    Directory_files directory(dirname);
    lstring fullname;

    size_t fileCount = 0;
    while (directory.more())
    {
        directory.fullName(fullname);
        if (directory.is_directory())
        {
            fileCount += FindClassDefinitions(fullname, ignorePatterns);
        }
        else if (fullname.length() > 0 && !FileMatches(fullname, ignorePatterns))
        {
            if (importPackage)
            {
                if (FindImportPackageInFile(fullname))
                    fileCount++;
            }
            else if (tabularList)
            {
                if (TabularListOfInFile(fullname))
                    fileCount++;
            }
            else
            {
                if (FindClassDefsInFile(fullname))
                    fileCount++;
            }
        }
    }
    return fileCount;
}

// ---------------------------------------------------------------------------
// Make title from code path, converting special characters to '_'
void MakeTitle(const lstring& codePath)
{
    //   d:\dir1\dir2\file.java  =>  _dir1_dir2_file

    size_t extn = codePath.find_last_of('.');
    title = codePath;
    if (extn != string::npos)
        title.resize(extn);
    size_t dirPos = title.find_last_of(SLASH_CHR);
    if (dirPos != string::npos)
        title.resize(dirPos);
    size_t colonPos = title.find_first_of(':');
    if (colonPos != string::npos)
        title.erase(0, colonPos+1);

    std::regex specialCharRe("[*?-]+");
    std::regex_constants::match_flag_type flags = std::regex_constants::match_default;
    title = std::regex_replace(title, specialCharRe, "_", flags);
    std::replace(title.begin(), title.end(), SLASH_CHR, '_');

    graphName = title;
}

// ---------------------------------------------------------------------------
void outputHtmlPrefix1()
{
    cout <<
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head> \n";
}

// ---------------------------------------------------------------------------
void outputHtmlMetaHeader2()
{
    cout <<
        "<meta name=\"keywords\" content=\"Android,class,hierarchy,tree,diagram\"> \n"
        "<meta name=\"description\" content=\"Android class hierachy\">\n"
        "<meta name=\"author\" content=\"Dennis Lang\">\n"
        "\n"
        "<!-- Mobile -->\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, maximum-scale=1\">\n"
        "\n"
        "<link rel=\"icon\" type=\"/~lang.dennis/image/ico\" href=\"favicon.ico\" > \n"
        "<link rel=\"shortcut icon\" href=\"/~lang.dennis/favicon.ico\" >\n"
        "\n"
        "<meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\" >\n"
        "<META http-equiv=\"Content-Style-Type\" content=\"text/css\">\n"
        "<meta name=\"google-site-verification\" content=\"sTC1_x30FvQ9Xk-23yK-7jWMaoWFc7Dtw0waX6FFrAk\" >\n"
        "\n"
        "<!-- Google analytics tracking -->\n"
        "<script type=\"text/javascript\" src=\"/~lang.dennis/scripts.js\"></script>\n"
        "\n"
        "<style> \n"
        "    body { \n"
        "    background: url(\"bg.jpg\");\n"
        "    }\n"
        "</style>\n";
}

// ---------------------------------------------------------------------------
void outputHtmlTableStyle()
{
    cout << 
        "<style type = 'text/css'>\n"
        "#gradient-style\n"
        "{\n"
        "    font-family: 'Lucida Sans Unicode', 'Lucida Grande', Sans-Serif;\n"
        "    font-size: 12px;\n"
        "    margin: 45px;\n"
        "    width: 480px;\n"
        "    text-align: left;\n"
        "    border-collapse: collapse;\n"
        "}\n"
        "#gradient-style th\n"
        "{\n"
        "    font-size: 13px;\n"
        "    font-weight: normal;\n"
        "    padding: 8px;\n"
        "    background: #b9c9fe url('img/gradhead.png') repeat-x;\n"
        "    border-top: 2px solid #d3ddff;\n"
        "    border-bottom: 1px solid #fff;\n"
        "    color: #039;\n"
        "}\n"
        "#gradient-style td\n"
        "{\n"
        "    padding: 8px;\n"
        "    border-bottom: 1px solid #fff;\n"
        "    color: #669;\n"
        "    border-top: 1px solid #fff;\n"
        "    background: #e8edff url('img/gradback.png') repeat-x;\n"
        "}\n"
        "#gradient-style tfoot tr td\n"
        "{\n"
        "    background: #e8edff;\n"
        "    font-size: 12px;\n"
        "    color: #99c;\n"
        "}\n"
        "#gradient-style tbody tr:hover td\n"
        "{\n"
        "   background: #d0dafd url('img/gradhover.png') repeat-x;\n"
        "   color: #339;\n"
        "}\n"
        "</style>\n";

    cout <<
        "<link rel='stylesheet' type='text/css' href='jquery.dataTables.min.css'> \n"
        "<script type='text/javascript' language='javascript' src='jquery-1.11.3.min.js'></script> \n"
        "<script type='text/javascript' language='javascript' src='jquery.dataTables.min.js'></script> \n"
        "\n"
        "<script type='text/javascript' class='init'> \n"
        "\n"
        "$(document).ready(function() { \n"
        "  $('#gradient-style').DataTable({ \n"
        "    'scrollY':        '600px', \n"
        "    'scrollCollapse' : false, \n"
        "    'paging' : false \n"
        "  }); \n"
        "}); \n"
        "</script> \n";
}

// ---------------------------------------------------------------------------
void outputHtmlTitle3(const char* title)
{
    cout << "	<title>" << title << " - Dennis Lang</title> \n";
}

// ---------------------------------------------------------------------------
int main(int argc, char* argv[])
{  
    if (argc == 1)
    {
        cerr << "\n" << argv[0] << "\n"
            << "\nDes: Generate Java class dependence tree"
            "\nUse: Javatree [-+ntgxshjz] header_files...\n"
            "\nSwitches (*=default)(-=off, +=on):"
            "\n  n  ; Show alphabetic class name list"
            "\n* t  ; Show class dependency tree"
            "\nOutput format (single choice):\n"
            "\n* g  ; Use graphics for tree connections"
            "\n  x  ; Use (+|-) for tree connections"
            "\n  s  ; Use spaces for tree connections"
            "\n  h  ; Html tree connections"
            "\n  j  ; Java tree connections"
            "\n  z  ; GraphViz "
            "\nModifiers:\n"
            "\n  Z              ; Split GraphViz by tree, use with -O"
            "\n  N=nodesPerFile ; Split by nodes per file, use with -O"
            "\n  O=outpath      ; Save output in file \n"
            "\n  V=filePattern  ; Ignore files \n"
            "\n  A=allClasses   ; Defaults to public \n"
            "\n"
            "\nExamples\n"
            "\n  javatree -t +n  src\\*.java  ; *.java prevent recursion"
                "\n  javatree -x  src\\* > javaTree.txt"
                "\n  javatree -h  src\\* > javaTree.html"
                "\n  javatree -j  src\\* > javaTreeWithJs.html"
                "\n"
                "\n  -V is case sensitive "
                "\n  javatree -z -Z -O=.\\viz\\ -V=*Test* -V=*Exception* src\\* >foo.gv"
                "\n  javatree -z -N=10 -O=.\\viz\\ -V=*Test* -V=*Exception* src\\* >foo.gv"
                "\n";
    }
    else
    {
        PatternList ignorePatterns;
        for (int argn = 1; argn < argc; argn++)
        {
            if (*argv[argn] == '-' || *argv[argn] == '+')
            {
                bool polarity = (*argv[argn] == '+');
                
                switch (argv[argn][1])
                {
                    // base options
                    case 'n': show_names= polarity;       break;
                    case 't': show_tree = polarity;       break;

                    // Output format
                    case 'g': cset      = GRAPHICS_CHAR;  break;
                    case 'x': cset      = TEXT_CHAR;      break;
                    case 's': cset      = SPACE_CHAR;     break;
                    case 'h': cset      = HTML_CHAR;      break;
                    case 'j': cset      = JAVA_CHAR;      break;
                    case 'z': cset      = VIZ_CHAR;       break;

                    // Modifiers
                    case 'A': allClasses = true;          break;
                    case 'I': importPackage = true;       break;
                    case 'T': tabularList = true;         break;
                    case 'Z': vizSplit = true;            break;
                    case 'N':
                        nodesPerFile = (int)strtol(argv[argn] + 3, 0, 10);
                        break;
                    case 'O':   // -O=<outPath>
                        outPath = argv[argn]+3;
                        break;
                    case 'V':  // -V=<pattern>
                        lstring str = argv[argn]+3;
                        ReplaceAll(str, "*", ".*");
                        ignorePatterns.push_back(std::regex(str));
                        break;
                }
            }
            else
            {
                codePath = argv[argn];
                MakeTitle(codePath);
                size_t fileCnt = FindClassDefinitions(argv[argn], ignorePatterns);
                std::cerr << fileCnt << " Files parsed, " << clist.size() << " classes found\n";
            }
        }            
    
        if (cset == VIZ_CHAR)
        {
            display_dependences();
            Release_clist();
            // outVizTrailer();
        }
        else if (tabularList)
        {
            outputHtmlPrefix1();
            outputHtmlMetaHeader2();
            outputHtmlTableStyle();
            outputHtmlTitle3(graphName);
            cout <<
                "</head>\n"
                "<h2>Tabular List of " << graphName << "</h2>"
                "<body>\n";
            outputHtmlTableList();
            cout <<
                "</body> \n"
                "</html> \n"
                "\n";
        }
        else if (cset == JAVA_CHAR)
        {
            outputHtmlPrefix1();
            outputHtmlMetaHeader2();
            outputHtmlTitle3(graphName);
#if 1
            cout <<
                "	<link rel=StyleSheet href=dtree.css type=text/css /> \n"
                "	<script type=text/javascript src=dtree.js></script>  \n";
#else
            cout << "<style>\n" << dtree_css << "\n</style>\n";
            cout << "<script type=\"text/javascript\">\n" << dtree_js << "\n</script>\n";
#endif
             cout <<
                "</head>           \n"
                "<body>            \n"
                "<h2>Example</h2>  \n"
                "<div class=dtree> \n"
                "	<p><a href=javascript:d.openAll();>open all</a> | <a href=javascript:d.closeAll();>close all</a></p>  \n"
                "	<script type=text/javascript> \n"
                "		<!--                \n"
                "		d = new dTree('d'); \n"
                        "       d.add(0, -1, '" << graphName << "');\n";

            display_dependences();
            Release_clist();

            cout << 
                "		document.write(d); \n"
                "       d.openAll();\n"
                "		//--> \n"
                "	</script> \n"
                "</div>  \n"
                "</body> \n"
                "</html> \n"
                "\n";
        }
        else
        {
            fputs(doc_begin[cset], stdout);
                        
            if (show_names) 
                display_names();
            if (show_tree)  
                display_dependences();
            
            Release_clist();

            fputs(doc_end[cset], stdout);
        }

        std::cerr << std::endl;
    }

    return 0;
}
