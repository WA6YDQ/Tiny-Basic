10 rem program to read from analog ports, 
20 rem write the integer values to sd card,
30 rem then read them back
40 rem note: fileopen [filename] w appends data
50 rem delete the file adval.dat if you don't want that
95 rem create a file to write data to
100 fileopen adval.dat w
105 rem get 20 values
110 for n=1 to 20
115 rem read analog values from a0
120 let a=pinread(A0)
122 print "A0=";a
125 rem write to the sd card
130 filewrite a
135 rem delay 1 second
140 delay 1000
145 rem continue until counter is exhasted
150 next n
155 rem done - close the file
160 fileclose
200 rem
210 rem now open the file and read back the values
220 fileopen adval.dat r
230 fileread d
240 if d=0 then 300
250 rem returned value is 0 when file is all read
260 print "data = ";d
270 goto 230
300 fileclose
310 print "done"
320 end
 