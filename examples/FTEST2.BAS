10 rem test data collection
20 dim 10
40 fileopen data.txt w
50 let @(0)=pinread(A0)
60 let @(1)=pinread(A1)
70 let @(2)=pinread(A2)
80 let @(3)=pinread(A3)
100 for i=0 to 3
110 filewrite @(i)
120 next i
125 filewrite  
130 print "Writing done"
140 fileclose
150 end
 