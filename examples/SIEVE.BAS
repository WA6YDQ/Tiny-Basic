1 rem this is the byte sieve of eratosthenes benchmark
2 rem written on tiny basic for the arduino due
3 rem this runs 1 iteration in appx 16 seconds
5 let s=8190
10 dim 8200
20 print "1 iteration"
50 let c=0
60 for i=0 to s
70 let @(i)=1
80 next i
90 for i=0 to s
100 if @(i)=0 then 180
110 let p=i+i+3
115 rem print p
120 let k=i+p
130 if k>s then 170
140 let @(k)=0
150 let k=k+p
160 goto 130
170 let c=c+1
180 next i
190 print c,"primes"
 