### a LanDen Labs - JavaTree
Android Developer Tool


WebSite
[http://landenlabs.com](http://landenlabs.com)

***
Description
***

javatree

Des: Generate Java class dependence tree
Use: Javatree [-+ntgxshjz] header_files...

Switches (*=default)(-=off, +=on):
  n  ; Show alphabetic class name list
* t  ; Show class dependency tree
Output format (single choice):

* g  ; Use graphics for tree connections
  x  ; Use (+|-) for tree connections
  s  ; Use spaces for tree connections
  h  ; Html tree connections
  j  ; Java tree connections
  z  ; GraphViz 
Modifiers:

  Z              ; Split GraphViz by tree, use with -O
  N=nodesPerFile ; Split by nodes per file, use with -O
  O=outpath      ; Save output in file 

  V=filePattern  ; Ignore files 

  A=allClasses   ; Defaults to public 


Examples

  javatree -t +n  src\*.java  ; *.java prevent recursion
  javatree -x  src\* > javaTree.txt
  javatree -h  src\* > javaTree.html
  javatree -j  src\* > javaTreeWithJs.html

  -V is case sensitive 
  javatree -z -Z -O=.\viz\ -V=*Test* -V=*Exception* src\* >foo.gv
  javatree -z -N=10 -O=.\viz\ -V=*Test* -V=*Exception* src\* >foo.gv
