5 rem read analog values, save and print
10 dim 12
20 for i=0 to 11
30 let @(i)=0
40 print "d/a values"
50 let @(0)=pinread(A0)
60 let @(1)=pinread(A1)
70 let @(2)=pinread(A2)
80 let @(3)=pinread(A3)
90 let @(4)=pinread(A4)
100 let @(5)=pinread(A5)
110 let @(6)=pinread(A6)
120 let @(7)=pinread(A7)
130 let @(8)=pinread(A8)
140 let @(9)=pinread(A9)
150 let @(10)=pinread(A10)
160 let @(11)=pinread(A11)
200 for i=0 to 11
210 print @(i),;
220 next i
225 delay 1000
226 print
230 goto 50
 