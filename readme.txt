  basic.c  (C) k theis 2020 <theis.kurt@gmail.com>
 
  This is a Tiny BASIC language interpreter written
  for both posix systems and the Arduino Due. It should
  work on anything with a command line and a c compiler.

  For posix systems (linux etc):
  compile with: cc -o basic basic.c -Wall
 
  For Arduino Due, just use the Arduino IDE to compile and
  upload this program. First rename basic.c to basic.ino

  This program is licensed under the GNU 3.0 license
 
  To run on a posix (linux) machine::
  basic [filename] where filename is an optional basic source file.
  After loading, the program will run until a STOP, END or EXIT statement.

  If a filename is not given on the command line, basic will start with 
  an Ok> prompt and place you in the editor mode with an empty file.
  
  On an Arduino Due: load the program using the IDE and plug in a usb 
  cable to the usb port closest to the power jack. Terminal defaults
  to 9600/8/N/1 and vt100/vt102. Putty, minicom, etc should work.

  -----------------------------------------------------------------

  NOTE: Although statements, expressions and functions appear in UPPER CASE,
  the basic lines entered are converted to lower case when typed. You can use 
  either UPPER or lower case in your statements. Characters between double 
  quotes ("") or parens () are NOT converted.

  -----------------------------------------------------------------

  These are the statements that this version of basic recognizes:
 
  LET [a-z/@(a-z/0-9)]=[expr] (see below for expr defines)
  INPUT ["",;][a-z]
  PRINT [expr][a-z][0-9]@(a-z/0-9)[; , ""]
  FILEREAD [a-z][,]
  FILEWRITE [a-z][,;""][@(a-z)]
  GOTO [0-9]
  GOSUB [0-9]
  RETURN
  REM
  IF [a-z][logical][a-z/0-9] [THEN/GOTO/GOSUB] [line number]
  IF [a-z][logical][a-z/0-9] [RETURN/STOP]
  STOP
  END
  EXIT
  SLEEP [0-9] (integer seconds) (NOTE:posix only)
  DELAY [0-9/a-z] (value is in msec) (NOTE:arduino only)
  FOR [a-z]=[a-z/0-9/expr] TO [a-z/0-9/expr] STEP [+-][0-9/a-z/expr]
  NEXT [a-z]
  CLEAR
  DIM (0-9/a-z/[expr])  NOTE: Array NOT cleared at start
  FILEOPEN [a-z/0-9][Rr/Ww]
  FILECLOSE

  --- Arduino Specific Statements ---
  PINSET [0-9/a-z]
  PINCLR [0-9/a-z]
  

  expr is a simple assignment (5, a+20, c-5, x) used for LET, PRINT
  and FOR. expr returns a numeric integer value.

  expr recognized the following operands:
  +  Addition
  -  Subtraction
  *  Multiplication
  /  Division
  %  Modulo
  &  Logical and
  |  Logical or
  ^  Logical xor
  ~  Compliment (bits inside a variable or value)
  E  Exponent (ie 2E3 = 2*2*2)

  For the Arduino, expr also includes:
  PINREAD(n) w/n = pin# [0-9/a-z] digital
  PINREAD(n) w/n = A0-A11  analog


  Logical comparisons (logical):
  The following symbols are used in the IF statement to compare 2 values:
  =  lvalue is equal to rvalue
  #	 lvalue is not equal to rvalue
  <  lvalue is less than rvalue
  >  lvalue is greater than rvalue
  &  lvalue logical and with rvalue
  |  lvalue logical or with rvalue
  ^  lvalue logical xor with rvalue

  Functions:
  ABS(x) w/x = [a-z]	absolute value of variable a-z
  RANDOM()				random number between 0 and 2^32/2 -1

  Functions return a numeric integer value 
  ------------------------------------------------------------------------

  Line numbers MUST be used, and be in the range of 1 thru 32767.
  Lines may be blank (newline terminated). However, the basic editor 
  does not save blank lines to memory. If you want them, create the file 
  with an external editor and load the program either from the command line
  or with the 'load' command.

  Variables are 32-bit integer (a-z). There is a single integer array 
  called @(). The dim nn statement sets up the array. nn is the decimal
  size of the array, maximum size is ARRAYMAX integers. On the Arduino, that's
  4*ARRAYMAX (see #define ARRAYMAX below). There are NO text variables.

  Spaces MUST be used between line numbers and keywords, and between multiple 
  keywords (ie. 10 for n=1 to 20 step 2). Variable assignments MUST NOT contain 
  any spaces (ie. 10 let a=5, b=a*20/3, c=a*b/2) but can use comma seperators
  and spaces between commas and the next assignment.

  -------------------------------------------------------------------------

  *** Commands ***
  run [linenumber]  	Start running the basic program. All variables are cleared.
  						If linenumber is given then no variables are cleared. The 
						basic program starts running from the given line.

  list					Display the basic program in memory.

  size/mem				Show free (unused) memory.

  *load [filename]		Load the file 'filename' into memory clearing out any 
  						prior code.
  
  *save [filename]		Save the program in memory to 'filename'.

  *dir					Show a directory of files in the current directory.

  *flist				List a file on the drive, no change to local memory.

  **slist				List a file to Serial Port #1 (printer etc).

  *delete               Delete a file
  
  new					Delete the program currently in memory.

  exit					Exit the basic interpreter.

  trace					Toggle program tracing ON/OFF.

  dump					Show a hex memory dump of the basic file.


    * only work if an SD card is connected to the SPI 
    port of the Arduino due. On a posix (linux) machine, 
    these commands access the current directory you are in.

    ** This only works on the Arduino due.


  TODO:
  nested for/next loops
  text variables
  virtual memory for buffer space and arrays
  larger gosub/return stack
  pwm output routines
  posix gpio routines
  msec posix delays 
  goto/gosub to a variable


