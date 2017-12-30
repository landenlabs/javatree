### a LanDen Labs - JavaTree
Java Developer Tool


WebSite
[http://landenlabs.com](http://landenlabs.com)

***
Parse Java source files and produce class relationship as HTML, GraphViz or Text file
***

Javatree help banner
<blockquote>


Des: Generate Java class dependence tree<br>
Use: Javatree [-+ntgxshjz] header_files...

<pre>
Switches (*=default)(-=off, +=on):
  n  ; Show alphabetic class name list
* t  ; Show class dependency tree

<p>
Output format (single choice):
* g  ; Use graphics for tree connections
  x  ; Use (+|-) for tree connections
  s  ; Use spaces for tree connections
  h  ; Html tree connections
  j  ; Java tree connections
  z  ; GraphViz 

<p>
Modifiers:
  Z              ; Split GraphViz by tree, use with -O
  N=nodesPerFile ; Split by nodes per file, use with -O
  O=outpath      ; Save output in file 
  V=filePattern  ; Ignore files 
  A=allClasses   ; Defaults to public 

<p>
Examples
  javatree -t +n  src\*.java  ; *.java prevent recursion
  javatree -x  src\* > javaTree.txt
  javatree -h  src\* > javaTree.html
  javatree -j  src\* > javaTreeWithJs.html

<p>
  -V is case sensitive 
  javatree -z -Z -O=.\viz\ -V=*Test* -V=*Exception* src\* >foo.gv
  javatree -z -N=10 -O=.\viz\ -V=*Test* -V=*Exception* src\* >foo.g
 </pre>
 </blockquote>
  
  <hr>
  <p>