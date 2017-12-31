#!/bin/csh -f

#
#  Generate test reports
# 

javatree test > reports/windows.txt
javatree -h test    >  reports/html.html
javatree -h -T test >  reports/tabular.html
javatree -x test    >  reports/text.txt
javatree -t +n test >  reports/list.txt

javatree -j test > reports/tree.html
javatree -z test > reports/directgraph.dot
