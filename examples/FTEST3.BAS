10 rem test data collection
20 dim 10
30 let c=0
40 fileopen data.txt
50 let @(0)=pinread(A0)
60 let @(1)=pinread(A1)
70 let @(2)=pinread(A2)
80 let @(3)=pinread(A3)
90 print "Pass ";c
100 for i=0 to 3
110 filewrite @(i),;
120 next i
125 filewrite  
130 delay 500
135 let c=c+1
140 if c<20 then 50
150 fileclose
200 end
 