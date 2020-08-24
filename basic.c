/* basic.c  (C) k theis 2020 <theis.kurt@gmail.com>
 
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

*/


/* !!!!!!!!!! NOTE NOTE NOTE NOTE NOTE !!!!!!!!!!! */
/* how are we coding this? (choose posix/arduino) */
#define posix
//#define arduino
/* !!!!!!!!!! NOTE NOTE NOTE NOTE NOTE !!!!!!!!!!! */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef posix
#include <unistd.h> 	// for posix sleep()
#endif

#ifdef arduino
#include <malloc.h> 	// for memory size determination
#include <SPI.h>
#include <SD.h>
#endif



/* general system defines */
#define MAXLINE 80			// max chars in a line
#define PROMPT "Ok> "

#ifdef posix
#define BUFSIZE 65536		// ram buffer memory for bigger computers
#define ARRAYMAX 65536      // max size of @() array (4 bytes/element)
// NOTE: on a posix machine, these are really unlimited due to VM
#endif

#ifdef arduino
#define BUFSIZE 16384		// ram buffer memory (arduino) for basic statements (appx 23 bytes/line)
#define ARRAYMAX 12032      // max size of @() array (4 bytes/element)
// NOTE: If you need more program size, adjust array size down so that you have 1024 bytes on top
// 16384 + (12032 * 4) + 1024 = 65536  (every byte of buffer = 4 bytes of array)
#define MAXRAND 2147483647	// 2^32/2-1
#endif

#define MAXLINENUMBER 32767
#define MAXRETURNSTACKPOS 10 // basic: max stack depth
#define HEADER "\r\nTiny+ Basic    (C) 2020 Kurt Theis"

/* define routine return values */
#define NORMAL_RETURN -1	// basic returned OK
#define ERROR_RETURN -2		// basic returned an error
#define END_RETURN -3		// basic encountered END
#define STOP_RETURN -4		// basic encountered STOP

/* define error messages */
#define ERR1    "syntax error\r\n"
#define ERR2    "syntax error in line "
#define ERR3    "bad char in line "
#define ERR4    "out of memory"
#define ERR5    "buffer is empty\n\r"
#define ERR6    "line number is out of range "
#define ERR7    "bad char in line number "
#define ERR8    "line not found in "
#define ERR9    "line not recognized in"
#define ERR10   "usage: load filename\r\n"
#define ERR11   "usage: save filename\r\n"
#define ERR12   "error creating file in "
#define ERR13   "error reading file in "
#define ERR14   "usage: flist filename\r\n"
#define ERR15   "usage: delete filename\r\n"
#define ERR16   "file not found in "
#define ERR17   "unexpected error in line "
#define ERR18   "end at line "
#define ERR19   "stop at line "
#define ERR20   "array re-dimension in line "
#define ERR21   "array size error in line "
#define ERR22   "dim: no action taken in line "
#define ERR23   "array too big in line "
#define ERR24   "out of memory in line "
#define ERR25   "stack full in line "
#define ERR26   "return without gosub in line "
#define ERR27   "bad format in line "
#define ERR28   "bad expression in line "
#define ERR29   "bad array in line "
#define ERR30   "array index too large in line "
#define ERR31   "unknown variable in line "
#define ERR32   "next without for in line "
#define ERR33   "unexpected next error in line "
#define ERR34   "usage: fileopen filename Rr/Ww in line "
#define ERR35   "file already open in line "
#define ERR36   "bad mode in fileopen in line "
#define ERR37   "file not open in line "
#define ERR38   "no file open for write in line "
#define ERR39   "unterminated string in line "
#define ERR40   "no file open for read in line "
#define ERR41   "unterminated quotes in line "
#define ERR42   "expression empty in line "
#define ERR43   "bad pin number in line "
#define ERR44   "missing closing ) in line "
#define ERR45   "array bounds error in line "
#define ERR46   "divide by zero error in line "
#define ERR47   "unknown operand error in line "
#define ERR48   "index to array must be a variable in line "
#define ERR49   "logical eval error in line "
#define ERR50   "directory error "



/* ******************** */
/* pre-define functions */
/* ******************** */

/* external (non-basic) routines */
extern void run80(void);


/* editor routines */
void list(char[]);
int getmaxlinenum(void);
int isline(int);
void fileload(char *);
void filesave(char *);
void flist(char *);
void dir(char*);
int run(char *);
void tokenize(char[]);
void linetolower(char *);
void filedelete(char *);
void showmem();


/* basic subroutines */
int parse(char *);
int setlinenumber(char[],int);
int parse_print(char[]);
int parse_input(char[]);
int eval(char *);
int parse_let(char[]);
int evallogic(char *);
int parse_if(char[]);
int parse_for(char[]);
int parse_next(char[]);
int isoperand(char);
int domath(int,char,int);
int dueanalog(int);
int fileopen(char[],char[]);
int fileclose(void);
int fileread(char[]);
int filewrite(char[]);



/* **************** */
/* global variables */
/* **************** */
#ifdef posix
FILE *diskfile;		// used for fileopen/close etc
#endif


#ifdef arduino
File root;          // used in dir
File sdFile;        // used in save, load
const int chipSelect = 53;  // SD card chip select

// used in showmem()
extern char _end;
char *ramstart=(char *)0x20070000;
char *ramend=(char *)0x20088000;
extern "C" char *sbrk(int i);
#endif

unsigned char *buffer;
unsigned int position;
unsigned int maxline;
int error=0;		// when a routine fails, error gets set
int DEBUG = 0;		// enables trace in parse()
char printmessage[MAXLINE+(MAXLINE/2)];		// universal print routine
int arraymax = 0;	// max size of array, assigned in DIM

unsigned int foraddr=0;		// loop address for/next 
unsigned char forvar;		// hold var name for/next
int tovar=0;		// final number for/next
int forstep=0;		// hold step size

/* define storage for return stack for gosubs */
int return_stack[MAXRETURNSTACKPOS]={-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};  // 10 deep
int return_stack_position = 0;

/* define storage for integer variables a-z */
int intvar[26]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/* define array for DIM and @(n) */
int* intarray = (int*)NULL;





/* showmem - routine to read arduino due registers and report memory usage */
/* saw this on stackoverflow.com, and many other places, and don't know who to attribute to */
void showmem() {
    #ifdef arduino
    /* due mem usage */
    char *heapend=sbrk(0);
    register char * stack_ptr asm("sp");
    struct mallinfo mi=mallinfo();
    Serial.print("\rDynamic RAM used ");
    Serial.println(mi.uordblks);
    Serial.print("\rProg Static Ram used ");
    Serial.println(&_end - ramstart);
    Serial.print("\rStack Ram used ");
    Serial.println(ramend - stack_ptr);
    Serial.print("\rFree Mem ");
    Serial.println(stack_ptr - heapend + mi.fordblks);
    #endif
    return;
}


#ifdef arduino
/* serial fgets : replacement for fgets */
void sgets(char *line) {
    unsigned int cnt=0;
    char ch;
    memset(line,0,MAXLINE);
    while (1) {
        if (Serial.available() > 0) {
            ch = Serial.read();
            if (ch == 0x08) {       // backspace
                if (cnt > 0) {   
                    Serial.print("\b");
                    Serial.print(" ");
                    Serial.print("\b");
                    line[cnt] = '\0';
                    cnt--;
                } else {
					continue;
                }
                continue;
            }
            
			if (ch == '\r') ch = '\n';
			if (cnt+1 >= MAXLINE-2) {		// MAXLINE reached
				Serial.write(0x07);		// ctrl-G bell
				continue;				// don't save anything
			}

            Serial.print(ch);			// save to buffer
			line[cnt++] = ch;

            if (ch == '\n') {
                Serial.print('\r');
                return;
            }
        }
    }
}
#endif

/* ***** */
/* SETUP */
/* ***** */
#ifdef arduino
void setup() {
    Serial.begin(9600);     // console I/O port 0
    Serial1.begin(9600);    // printer I/O port 1
    analogReadResolution(16);

    /* set up SD card */
    if (!SD.begin(chipSelect)) {        // 53 is SD chip select
        Serial.println("SD card init failed!");
        Serial.println("Power down, fix and restart");
        while (1);
    }


}
#endif




/* ***** */
/* PROUT */
/* ***** */
/* send string to stdout/serialout */
void prout(char message[MAXLINE+(MAXLINE/2)]) {
    #ifdef posix
	printf("%s",message);
    #endif

    #ifdef arduino 
	Serial.print(message);
    #endif

	return;
}



/* **************** */
/*    main/loop     */
/* **************** */
#ifdef posix
int main(int argc, char **argv) {
#endif

#ifdef arduino
void loop() {
#endif

int pos=0;
int n;
char line[MAXLINE]={}, linenum[32]={};
char *p;

	/* basic program is stored in ram */
	buffer = (unsigned char *)malloc(BUFSIZE);
	if (buffer == NULL) {
		prout("out of memory");
        #ifdef posix
		return 1;
        #endif
        
        #ifdef arduino
        while(1);
        #endif
	}

    // clear the buffer before start
    memset(buffer,0,BUFSIZE);
	position = 0;   // set start position
	maxline = 0;    // highest line number

	
	sprintf(printmessage,"%s\r\n",HEADER);
	prout(printmessage);
	sprintf(printmessage,"%d Bytes Free\r\n",BUFSIZE-position);
	prout(printmessage);

    #ifdef posix
	/* test command line: if argv[1] = program name, load & run it */
	if (argc == 2) {
		char temp[strlen(argv[1])+5];
		strcpy(temp,"load ");
		strcat(temp,argv[1]);
		fileload(temp);		// format of the load command requires 'load' before filename
		/* run it */
		run("x");			// x is dummy, not used
	}   // at end jump to editor
    #endif


/***********************************/
/*       Main Command Loop         */
/***********************************/
	while (1) {

		/* show prompt, get a line or command */
		memset(line,0,MAXLINE);
		maxline = getmaxlinenum();
        prout(PROMPT);
        
        #ifdef posix
		fgets(line,MAXLINE,stdin);
        #endif

        #ifdef arduino
		sgets(line);
        #endif

		if (line[0] == '\n') continue;
		line[strlen(line)] = '\0';



        #ifdef posix
		/* exit - exit out of this program */
		if (strncmp(line,"exit",4)==0) {
			if (intarray != NULL)
				free(intarray);		// free up the array ram
			free(buffer);			// and program memory
			return 0;
		}
        #endif

		/* trace - trace program flow */
		if (strncmp(line,"trace",4)==0) {
			DEBUG = abs(DEBUG-1);
			if (DEBUG) {
				sprintf(printmessage,"Trace ON\r\n");
				prout(printmessage);
			}
			else {
				sprintf(printmessage,"Trace OFF\r\n");
				prout(printmessage);
			}
			continue;
		}
		
		/* list - display basic listing in buffer */
		if (strncmp(line,"list",4)==0) {
			list(line);
			continue;
		}


        /* slist - send listing to serial 1 (if arduino) */
        #ifdef arduino
        if (strncmp(line,"slist",5)==0) {
            list(line);
            continue;
        }
        #endif
        
		/* new - clear the buffers, reset pointers */
		if (strncmp(line,"new",3)==0) {
			position=0;
			memset(buffer,0,BUFSIZE);
            if (intarray != NULL)
                free(intarray);     // clear DIM memory
            for (int i=0; i<26; i++)
                intvar[i]=0;      // clear vars a-z
            intarray=(int*)NULL;
            arraymax=0;
			maxline=0;
			continue;
		}

		/* dump - hex dump of program listing */
		if (strncmp(line,"dump",4)==0) {
			int addr = 0;
			while (addr < position) {
				sprintf(printmessage,"%04X  ",addr);
				prout(printmessage);
				for (n=0; n<16; n++) {
					sprintf(printmessage,"%02X ",buffer[addr+n]);
					prout(printmessage);
				}
				prout("  ");
				for (n=0; n<16; n++) {
					sprintf(printmessage,"%c",isprint(buffer[addr+n])?buffer[addr+n]:'.');
					prout(printmessage);
				}
				prout("\r\n");
				addr += 16;
			}
			prout("\r\n");
			continue;
		}

		/* mem/size - show free memory */
		if ((strncmp(line,"mem",3)==0) || (strncmp(line,"size",4)==0)) {
			sprintf(printmessage,"Basic Program Storage: %d bytes free\r\n",BUFSIZE-position);
			prout(printmessage);
            showmem();
			continue;
		}

		/* load - load file to buffer */
		if (strncmp(line,"load",4)==0) {
			fileload(line);
			continue;
		}

		/* save - save buffer to file */
		if (strncmp(line,"save",4)==0) {
			filesave(line);
			continue;
		}

		/* flist - display file contents */
		if (strncmp(line,"flist",5)==0) {
			flist(line);
			continue;
		}

		/* dir - show directory */
		if (strncmp(line,"dir",3)==0) {
            prout("\r\n");
			dir(line);
			continue;
		}

        /* delete - delete a file from the file system */
        if (strncmp(line,"delete",6)==0) {
            filedelete(line);
            continue;
        }

		/* run - run the basic program */
		if (strncmp(line,"run",3)==0) {
			if (position==0) {
				prout(ERR5);    // empty buffer
				continue;
			}
			run(line);
            memset(line,0,MAXLINE);
            #ifdef arduino
            /* clear any chars in buffer caused by ctrl-c */
            if (Serial.available())
                while(Serial.available());
            #endif
			prout("\r\n");	// in case print terminated with ;
			continue;
		}

		/* if the 1st character of a line isn't a number, show an error */
		if (!(isdigit(line[0]))) {
			prout(ERR1);    // syntax
			continue;
		}

		/* ********************** */
		/* end of command testing */
		/* ********************** */
		
		/* first char IS a line number - save in memory */

		/* test line number count */
		sscanf(line,"%s ",linenum);
		if (atoi(linenum) < 1 || atoi(linenum) > MAXLINENUMBER) {
			prout(ERR6);    // line # out of range
			continue;
		}
        /* look for bad trailing chars in line number */
        if (!(isdigit(linenum[strlen(linenum)-1]))) {
                prout(ERR3);    // bad char in line #
                continue;
        }

		/* 
		 * Memory savings when converting to tokens is only 18%, 
		 * hardly worth the added code. With 16K ram you get about
		 * 744 lines of basic code. Quite a lot for a controller.
		 */
		//tokenize(line);
       
		linetolower(line);  // all but quoted and inside () lower case

		/* room to add the line? */
		if (strlen(line) + position > BUFSIZE-1) {
			prout(ERR4);    // out of memory
            continue;
        }



		/* *********************************************** */
		/* if line number > maxline, append line to buffer */
		/* *********************************************** */
		if (atoi(linenum) > maxline) {
						
			/* test if line is blank (don't insert blank lines) */
            int FLAG=0;
			for (int n=strlen(linenum); n<strlen(line); n++) 
                if (!(isspace(line[n]))) FLAG=1;
            if (!FLAG) continue;

			p=line;
			while (*p != '\0') {
				buffer[position++] = *p++;
			}
			if (position >= BUFSIZE-1) {
				prout(ERR4);    // out of memory
			}
			maxline = getmaxlinenum();
			continue;
		}
	


		/* ********************************************************* */
		/* if line number == existing line number, delete or replace */
		/* ********************************************************* */
		if ((pos = isline(atoi(linenum))) != -1) {
			int FLAG=0;
			/* pos points to start of line to replace */
			unsigned char *start, *end;
			end = start = buffer+pos;	// start of line
			while (*end++ != '\n'); 	// look for \n - end = end of line 
			
			/* delete line */
			while (end-buffer <= position) 
				*start++ = *end++;
			position -= (end-start); // line is deleted
			
			/* clear the old code above old position */
			for (n=position+1; n<BUFSIZE; n++) buffer[n]='\0';
			
			/* test if entered line is empty (delete) */
			if (strlen(line)-strlen(linenum) == 1) {
				maxline = getmaxlinenum();
				continue;		// line was empty - we're done
			}

			/* test if line is blank (don't insert blank lines) */
			for (n=strlen(linenum); n<strlen(line); n++) 
				if (!(isspace(line[n]))) FLAG=1;
			if (!FLAG) continue;

			/* else shift buffer up by strlen(line) */
			memmove(&buffer[pos+strlen(line)],&buffer[pos],position-pos);
			position += strlen(line);
			
			/* insert line at pos */
			for (int i=pos, n=0; i<=pos+(strlen(line)-1); i++)
				buffer[i] = line[n++];
			maxline = getmaxlinenum();
			continue;
		}

		/* ***************************************** */
        /* line# < maxline - insert line into buffer */
        /* ***************************************** */
        if (atoi(linenum) < maxline) {
            int n, i, start=0, end=1;
            char temp[MAXLINE]={};
            char LINEN[6];     // hold line number from temp

			/* test if line is blank (don't insert blank lines) */
            int FLAG=0;
            for (int n=strlen(linenum); n<strlen(line); n++)
                if (!(isspace(line[n]))) FLAG=1;
            if (!FLAG) continue;


            loop:   // find line w/line# higher than new line

            i = 0;
            for (n=start; n<position; n++) {
                if (n >= position) {
                    //sprintf(printmessage,"error - line not found\r\n");
					prout(ERR8);    // line not found
                    continue;
                }
                if (buffer[n] == '\n') break;
                temp[i++] = buffer[n];      // get line from buffer
            }
            end = n+1;
            sscanf(temp,"%s",LINEN);
            if (atoi(LINEN) > atoi(linenum)) goto loop2;
            start = end;
            goto loop;

            loop2:  // start points to start position of our new line

            /* shift up buffer by strlen(line) */
            memmove(&buffer[start+strlen(line)],&buffer[start],position-start);
            position += strlen(line);

            /* insert line at start */
            for (i=start, n=0; i<=start+(strlen(line)-1); i++)
                buffer[i] = line[n++];
            maxline = getmaxlinenum();
            continue;
        }


		/* **************************** */
		/* error - somehow we fell thru */
		/* **************************** */

		prout(ERR9);    // line not recognized
    	// should never see this
		continue;
	}
}









/* ************************************* */
/* return highest line # found in buffer */
/* ************************************* */
int getmaxlinenum(void) {
    char temp[MAXLINE]={};  // hold line
    int n,i, maxline = 0;
    char linenum[10] = {};
    int start = 0, end = 1;

loop:
    i = 0;
    for (n=start; n<position; n++) {
        if (buffer[n] == '\n') break;
        temp[i++] = buffer[n];
    }
    if (n >= position) {
		return maxline;
	}
    end = n+1;
    // got a line in temp
    sscanf(temp,"%s",linenum);
    if (atoi(linenum) > maxline)
        maxline = atoi(linenum);
    start = end;
    goto loop;
}


/* *************************** */
/* return true if line# exists */
/* *************************** */
int isline(int line) {
	unsigned char *p;
	char temp[MAXLINE]={}, linenum[32]={};
	int n=0;
	p = buffer;
loop:
	n=0;
	while (1) {
		temp[n++] = *p;
		if (*p++ == '\n') break;
	}
	sscanf(temp,"%s ",linenum);
	if (atoi(linenum)==line) return (p-buffer)-n;	// start position
	if (p-buffer >= position-1) return -1;
	goto loop;
}



/* ****************** */
/*     list/slist     */
/* ****************** */
void list(char line[]) {
char cmd[20]={};
	unsigned char *p;
	p=buffer;
	int cnt=0;
    sscanf(line,"%s ",cmd); // see what command we're running
  
	prout("\n\r");
	while (cnt++ < position) {
		// list
        if (strcmp(cmd,"list")==0) {
            if (*p == '\n') prout("\r");    // force a cr/lf
		    sprintf(printmessage,"%c",*p++);
		    prout(printmessage);
        }
        #ifdef arduino
        if (strcmp(cmd,"slist")==0) {
            if (*p == '\n') Serial1.print("\r");
            Serial1.print(*p++);
        }
        #endif
	}
	return;
}



/* ******************* */
/* load file to buffer */
/* ******************* */
void fileload(char *line) {

	//FILE *infile;
	char cmd[10]={}, filename[32]={}, ch;
    sscanf(line,"%s %s ",cmd,filename);
    if (strlen(filename)==0) {
		prout(ERR10);   // usage: load fname
        return;
    }
    #ifdef posix
    FILE *infile;
    infile = fopen(filename,"r");
    if (infile == NULL) {
		prout(ERR16);   // file not found
        return;
    }
	memset(buffer,0,BUFSIZE);
	position = 0;
	while (1) {
		ch = fgetc(infile);
		if (feof(infile)) break;
		if (ch != '\0')
			buffer[position++] = ch;
	}
	//position -= 1;	// otherwise we get run errors
	fclose(infile);
    #endif

    #ifdef arduino
    // sdFile defined in globals
    sdFile = SD.open(filename, FILE_READ);
    if (sdFile == NULL) {
        prout(ERR13);      // error reading file
        return;
    }
    memset(buffer,0,BUFSIZE);
    position = 0;
    while (sdFile.available()) {
        ch = sdFile.read();
        if (ch != '\0')
            buffer[position++] = ch;
    }
    sdFile.close();    
    #endif
    
	return;
}



/* ******************* */
/* save buffer to file */
/* ******************* */
void filesave(char *line) {

	char cmd[10]={}, filename[32]={};
	sscanf(line,"%s %s ",cmd,filename);
	if (strlen(filename)==0) {
		prout(ERR11);   // usage: save fname
		return;
	}
    #ifdef posix
    FILE *outfile;
	outfile = fopen(filename,"w");
	if (outfile == NULL) {
		prout(ERR12);   // error creating file
		return;
	}
	for (int n=0; n<=position; n++)
		fprintf(outfile,"%c",buffer[n]);
	fclose(outfile);
    #endif

    #ifdef arduino
    //sdFile defined in globals
    if (SD.exists(filename))
        SD.remove(filename);
    sdFile = SD.open(filename, FILE_WRITE);
    if (sdFile == NULL) {
        prout(ERR12);   // error creating file
        return;
    }
    for (int n=0; n<=position; n++)
        sdFile.write(buffer[n]);
    sdFile.close();
    #endif
    
	return;
}






/* ******************************** */
/* display listing of external file */
/* ******************************** */
void flist(char *line) {
char ch;

	char cmd[10]={}, filename[32]={};
	sscanf(line,"%s %s ",cmd,filename);
	if (strlen(filename)==0) {
		prout(ERR14);   // usage:flist filename
		return;
	}
   
    #ifdef posix
    FILE *infile;
	infile = fopen(filename,"r");
	if (infile == NULL) {
		prout(ERR13);   // error reading file
		return;
	}
	while (1) {
		ch = fgetc(infile);
		if (feof(infile)) break;
		printf("%c",ch);
	}
	fclose(infile);
    #endif
    
    #ifdef arduino
    sdFile = SD.open(filename, FILE_READ);
    if (sdFile == NULL) {
        prout(ERR13);   // error reading file
        return;
    }
    while (sdFile.available()) {
        ch = sdFile.read();
        if (ch == '\n') Serial.write('\r');
        Serial.write(ch);    
    }
    sdFile.close();    
    #endif

	return;
}




/* ************** */
/* show directory */
/* ************** */
void dir(char* line) {

    #ifdef posix
    #include <dirent.h>
	struct dirent **namelist;
	int n=0, i=0;

	n = scandir(".",&namelist,NULL,alphasort);
	if (n == -1) {
		prout(ERR50);   // directory error
		return;
	}

	while (n--) {
		printf("%s\r\n",namelist[i]->d_name);
		free(namelist[i]);
		i++;
	}
    #endif
    
    #ifdef arduino
    root = SD.open("/BASIC/");	// normally just SD.open("/");
    printDirectory(root,0);
    #endif
	
	return;
}

// I pulled this from the SD examples
#ifdef arduino
void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}
#endif


/* ************** */
/*  delete a file */
/* ************** */
void filedelete(char *line) {
char cmd[10]={}, filename[32]={};
    sscanf(line,"%s %s ",cmd,filename);
    if (strlen(filename)==0) {
        prout(ERR15);   // usage delete fname
        return;
    }
    #ifdef posix
    int err = unlink(filename);
    if (err == -1) perror(filename);
    #endif

    #ifdef arduino
    if (SD.exists(filename))
        SD.remove(filename);
    else
        Serial.println(ERR16);  // file not found
    #endif
    
    return;
    
}


/* ************************** */
/* convert line to lower case */
/* ************************** */
void linetolower(char *line) {
char temp[strlen(line)+5];
int FLAG=1;
	FLAG=1;
    for (int n=0; n<=strlen(line); n++) {
        if (line[n] == '"' || line[n] == '(' || line[n] == ')')  // ignore quoted text & parens
            FLAG=abs(FLAG-1);
        if (FLAG)
            temp[n] = tolower(line[n]);
        else
            temp[n] = line[n];
    }
	strcpy(line,temp);
	return;
}


/* ********************** */
/*    tokenize the line   */
/* ********************** */
void tokenize(char *line) {
    // unused - not worth the effort
	return;
}



/* ************************************ */
/* this is the actual basic interpreter */
/* ************************************ */
int run(char *line) {

char linenum[6]={};
char basicline[MAXLINE]={}, cmd[6]={};
int pos=0, n=0; 	// n-local, pos = local position
int res=0;			// result returned from parse()

    prout("\r\n");
    
	// test integrity of basic file
	for (n=0; n<position-1; n++) {
		if (buffer[n] == 0) {
			sprintf(printmessage,"ERROR in basic file at address %04x\r\n",n);
			prout(printmessage);
			sprintf(printmessage,"Basic file is corrupt.\r\n");
			prout(printmessage);
			return 1;
		}
	}

	// test how we got here: run <cr> starts at position 0 and clear all 
	// variables before starting.
	// run <line number> does not clear the vars and starts running
	// at <line number>.
	
	sscanf(line,"%s %s ",cmd,linenum);
	if (atoi(linenum) == 0) {


	// clear the gosub stack
	return_stack_position = 0;
	for (n=0; n<10; n++)
		return_stack[n] = -1;

	// clear all integer variables
	for (unsigned char ch='a'; ch <= 'z'; ch++)
            intvar[ch-'a']=0;

	// clear integer array
	if (intarray != NULL) {
		free(intarray);
		intarray = (int*)NULL;
		arraymax = 0;
	}

	// clear for/next variables
	forvar = '\0';
    forstep = 0;
    foraddr = 0;
    tovar = 0;

    #ifdef arduino
    if (sdFile) sdFile.close();     // close if open
    #endif
	#ifdef posix
	if (diskfile) { 
		fclose(diskfile);
		diskfile=0;		// null it
	}
	#endif

	pos = 0;		// set initial position in the buffer

	} else 
		pos = setlinenumber(linenum,0);		// get address of line number


	while (1) {

        #ifdef arduino
		/* stop on ^c */
        if (Serial.available() > 0) {
            char ch = Serial.read();
            if (ch == 0x03) {   // ^C
                Serial.print("\r\n^C Break.\r\n");
                while (Serial.available());
                return 0;
            }
        }
        #endif


		memset(basicline,0,MAXLINE);

		/* get a line */
		for (n=0; n<MAXLINE; n++) {
			basicline[n] = buffer[pos];			// pos points to current byte in the buffer
			if (buffer[pos] == '\n') break;		// and increments from 0 to the end of the buffer
			pos++; if (pos >= position) break;	// being changed only by goto/gosub/return/for/next
		}

		if (pos++ >= position) {
			//prout("no end statement\n\r");
			return 1;	// back to editor
		}

		if (basicline[n] == '\n') { // got line
			sscanf(basicline,"%s ",linenum);	 // line # = atoi(linenum) 
			res = parse(basicline);
			if (res == NORMAL_RETURN) continue;	 // normal exit, next basic line
			if (res == ERROR_RETURN) { 			 // error (err displayed in routine): exit to editor
				#ifdef posix
				printf("%s\n",linenum);
				#endif
				#ifdef arduino
				Serial.println(linenum);
				#endif
				return 1;
			}
			if (res == END_RETURN) return 0;	// got 'end' statement. done, return to editor
			if (res == STOP_RETURN) return 0;	// got 'stop' statement, done, return to editor
			if (res >= 0) {
				pos = res;		// result is address of a line to jump to
				continue;
			}
		}

		prout(ERR17);   // unexpected error
		return 1;
	}
}



/* ********************** */
/*** Instruction Parser ***/
/* ********************** */
int parse (char line[]) {	// parse the line, run the contents 
	char linenum[6]={}, keyword[20]={}, option[60]={}, value[20]={};

	sscanf(line,"%s %s %s %s ",linenum,keyword,option,value);
	if (strlen(line) == 1) return NORMAL_RETURN;	// ignore blank lines

	if (DEBUG) {
		sprintf(printmessage,"TRACE: line [%s]  \r\n",line);
		prout(printmessage);
	}

	if (atoi(linenum)==0) {
		prout(ERR6);    // line number error
		return ERROR_RETURN;
	}

	error = 0;		// initialize before each line
	/* test keyword */
	if (strcmp(keyword,"end")==0) {		// END
		prout(ERR18);   // end of line
        #ifdef posix
        printf("%s\r\n",linenum);
        #endif
        #ifdef arduino
        Serial.println(linenum);
        #endif
		return END_RETURN;
	}

    #ifdef posix
	if (strcmp(keyword,"exit")==0) {	// EXIT
		prout("\n");
		exit(0);
	}
    #endif

	if (strcmp(keyword,"stop")==0) {	// STOP
		prout(ERR19);   // stop at line
        #ifdef posix
        printf("%s\r\n",linenum);
        #endif
        #ifdef arduino
        Serial.println(linenum);
        #endif
		return STOP_RETURN;
	}

	if (strcmp(keyword,"rem")==0) {		// REM
		return NORMAL_RETURN;	// ignore rest of line 
	}

	if (strcmp(keyword,"dim")==0) {		// DIM
		if (arraymax > 0) {	// we already did this
			prout(ERR20);   // array re-dim
			return ERROR_RETURN;
		}
		error = 0;
		int res = eval(option);		// get size of array
		if (error) {
			prout(ERR21);   // array size error
			return ERROR_RETURN;
		}
		if (res < 1) {
			prout(ERR22);   // dim - no action taken
			return ERROR_RETURN;
		}
		if (res > ARRAYMAX) {
			prout(ERR21);   // array size 
			return ERROR_RETURN;
		}
		intarray = (int*) malloc(res * sizeof(int));
		if (intarray == NULL) {
			prout(ERR24);   // out of memory
			return ERROR_RETURN;
		}
		arraymax = res;
		return NORMAL_RETURN;
	}

	if (strcmp(keyword,"goto")==0) {	// GOTO
		int result = setlinenumber(option,0);					// return the address of the line to goto
		if (result == ERROR_RETURN) return ERROR_RETURN;		// line # not found
		return result;	// return address of line
	}

	if (strcmp(keyword,"gosub")==0) {	// GOSUB
		int res = setlinenumber(linenum,1);		// get addr of line after linenum
		if (res == ERROR_RETURN) return ERROR_RETURN;
		if (return_stack_position + 1 > MAXRETURNSTACKPOS) {
			prout(ERR25);   // stack full
			return ERROR_RETURN;
		}
		return_stack[return_stack_position++] = res;	// save return address on stack
		res = setlinenumber(option,0);  				// get address of linenumber following gosub
        if (res == ERROR_RETURN) return ERROR_RETURN;
        return res; // gosub new address
	}

	if (strcmp(keyword,"return")==0) {	// RETURN
		if (return_stack_position < 1) {
			prout(ERR26);   // return w/o gosub
			return ERROR_RETURN;
		}
		int res = return_stack[--return_stack_position];		// pop the return address
		if (res == ERROR_RETURN) return ERROR_RETURN;
		return res;
	}

    #ifdef posix
	if (strcmp(keyword,"sleep")==0) {	// SLEEP
		if (atoi(option) > 0)
			sleep(atoi(option));		// in integer seconds
		return NORMAL_RETURN;
	}
    #endif


	if (strcmp(keyword,"clear")==0) {	// CLEAR
		for (unsigned char ch='a'; ch <= 'z'; ch++)
			intvar[ch-'a']=0;			// clear all integer variables
		free(intarray);
		arraymax=0;
		return NORMAL_RETURN;
	}

	if (strcmp(keyword,"let")==0) {		// LET
		int res = parse_let(line);
		return res;
		return parse_let(line);
	}

	if (strcmp(keyword,"print")==0) {	// PRINT
		return parse_print(line);
	}

	if (strcmp(keyword,"input")==0) {	// INPUT
		return parse_input(line);
	}

	if (strcmp(keyword,"if")==0) {		// IF
		int res = parse_if(line);
		return res;
		return parse_if(line);
	}

	if (strcmp(keyword,"for")==0) {		// FOR
		return parse_for(line);
	}

	if (strcmp(keyword,"next")==0) {	// NEXT
		return parse_next(line);
	}


    if (strcmp(keyword,"fileopen")==0) {    // FILEOPEN
        return fileopen(option,value);
    }

    if (strcmp(keyword,"fileclose")==0) {    // FILECLOSE
        return fileclose();
    }


    if (strcmp(keyword,"filewrite")==0) {   // FILEWRITE
        return filewrite(line);
    }

    if (strcmp(keyword,"fileread")==0) {    // FILEREAD
        return fileread(line);
    }


	/* arduino specific statements */
    #ifdef arduino

	/* pinset pin# - set digital pin to 1 */
	if (strcmp(keyword,"pinset")==0) {		// PINSET
		int res=0;
		if (option[0] >= 'a' && option[0] <= 'z') 
			res = intvar[option[0] - 'a'];
		else
			res = atoi(option);
		pinMode(res,OUTPUT);
		digitalWrite(res,1);
		return NORMAL_RETURN;
	}

	/* pinclr pin# - set digital pin to 0 */
	if (strcmp(keyword,"pinclr")==0) {		// PINCLR
        int res=0;    
		if (option[0] >= 'a' && option[0] <= 'z') 
            res = intvar[option[0] - 'a'];
        else
            res = atoi(option);
		pinMode(res,OUTPUT);
		digitalWrite(res,0);
		return NORMAL_RETURN;
	}

    if (strcmp(keyword,"delay")==0) {       // DELAY
        int res = 0;
        if (option[0] >= 'a' && option[0] <= 'z')
            res = intvar[option[0] - 'a'];
        else
            res = atoi(option);
        delay(res);     // in msec
        return NORMAL_RETURN;
    }
    
    #endif
	/* end of arduino specific statements */

	prout(ERR2);    // syntax in line
	return ERROR_RETURN;
}


/* ********* */
/*    LET    */
/* ********* */
int parse_let(char line[]) {
char *p;
char linenum[6];
char temp[20]={};
int cnt=0;

	sscanf(line,"%s ",linenum);
	if (atoi(linenum)==0) {
		prout(ERR6);    // bad linenumber
		return ERROR_RETURN;
	}

	p = strstr(line,"let");
	if (p == NULL) {
		prout(ERR27);    // bad format
		return ERROR_RETURN;
	}

	while (*p++ != ' ');	// point to 1st non-blank char after 'let'
	cnt = 0;

	while (1) {
		if (*p == '\n' || *p == '\0') return NORMAL_RETURN;

		if (*p == ',') {
            p++;
            continue;
        }
		
		if (*p == ' ') {
            p++;
            continue;
        }

		// assign variable
		if (*p >= 'a' && *p <= 'z') {
			intvar[*p -'a'] = eval(p+2);
			if (error) {
				prout(ERR28);   // bad expression
				return ERROR_RETURN;
			}
            if (*(p+1) != '=') {
                prout(ERR2);    // syntax error
                return ERROR_RETURN;
            }
			while (1) {		// point past value
				p++;
				if (*p=='\n' || *p=='\0' || *p==',' || *p== ' ') break;
			}
			continue;
		}

		// set array value
		if (*p == '@' ) {
			p++;
			if (*p != '(') {
				prout(ERR29);   // bad array
				return ERROR_RETURN;
			}
			p++;	// point to start of expr
			error = 0;
			cnt=0;
			memset(temp,0,20);
			while (*p != ')') {
				if (cnt > 15) {		// beware overflow
					prout(ERR23);
					error = 1;
					return ERROR_RETURN;
				}
				temp[cnt++]=*p++;
			}
			temp[cnt]='\n';		// temp holds value (num/var a-z) between()
			int index=eval(temp);	// index is array index
			if (error) {
				prout(ERR2);
				return ERROR_RETURN;
			}
			if (index > arraymax-1) {
				prout(ERR23);   // array too large
				return ERROR_RETURN;
			}
			if (*p != ')') prout("missing )");
			p++;
			if (*p != '=') prout("missing =");
			p++;
			int res = eval(p);
			if (error) {
				prout(ERR2);
				return ERROR_RETURN;
			}
			intarray[index] = res;
			while (1) {	// step p until *p=\n or ,
				if (*p == '\n' || *p == ',') break;
				p++;
			}
			continue;
		}



	}

	prout(ERR2);    // syntax in line
	return ERROR_RETURN;
}


/* ********** */
/*     IF     */
/* ********** */
int parse_if(char line[]) {
	char linenum[6]={}, keyword[20]={}, expression[20]={}, wordthen[20]={}, newline[20]={};
	sscanf(line,"%s %s %s %s %s ",linenum,keyword,expression,wordthen,newline);
	int res = 0;

	/* 
	 * format: 
	 * if [expression] [then/goto/gosub] [line#] 
	 * if [expression] [return/stop]
	 *
	 */ 	

	res = evallogic(expression);
	if (error) {
		return ERROR_RETURN;
	}

	if (!res) // returned FALSE
		return NORMAL_RETURN;
	else {	// goto line number 'newline'
		if ((strcmp(wordthen,"then")==0) || (strcmp(wordthen,"goto")==0)) {
			int result = setlinenumber(newline,0);                  // return the address of the line
        	if (result == ERROR_RETURN) return ERROR_RETURN;        // line # not found
        	return result;  										// return address of line
		}
		if (strcmp(wordthen,"gosub")==0) {
			int res = setlinenumber(linenum,1);						// get address of next line
			if (res == ERROR_RETURN) return ERROR_RETURN;			// non-existant #
			if (return_stack_position + 1 > MAXRETURNSTACKPOS) {	// test stack
				prout(ERR25);   // stack full
				return ERROR_RETURN;
			}
			return_stack[return_stack_position++] = res;			// push return addr on stack
			res = setlinenumber(newline,0);							// get address of dest line
			if (res == ERROR_RETURN) return ERROR_RETURN;			// bad line number
			return res;												// jump to new line
		}
		if (strcmp(wordthen,"return")==0) {
			if (return_stack_position < 1) {
				prout(ERR26);   // return w/o gosub
				return ERROR_RETURN;
			}
			int res = return_stack[--return_stack_position];		// get return address
			if (res == ERROR_RETURN) return ERROR_RETURN;
			return res;												// jump back
		}
		if (strcmp(wordthen,"stop")==0) {
			prout(ERR19);   // stopped at line
			return STOP_RETURN;
		}


		prout(ERR2);    // syntax in line
		return ERROR_RETURN;
	}
	prout(ERR17);   // unknown error in line
	return ERROR_RETURN;
}


/* *********** */
/*    FOR      */
/* *********** */
int parse_for(char line[]) {
char linenum[6]={}, keyword[6]={}, expr[12]={}, key2[6]={}, final[12]={}, key3[6]={}, stepsize[12]={};
char *p;
int res=0;
	/* ex: 10 for n=1 to 10 step 1 */
	/* the for part of a for/next loop just sets up the variables  */
	/* global vars: foraddr (ret address) (int)intvar[n] (ch)forvar (int)tovar  (int)forstep */
	sscanf(line,"%s %s %s %s %s %s %s ",linenum,keyword,expr,key2,final,key3,stepsize);
	
	/* get forvar */
	expr[strlen(expr)]='\n';
	/* test for 1st variable */
	if (!(*expr >= 'a' && *expr <= 'z')) {
		prout(ERR2);
		return ERROR_RETURN;
	}

	p=strchr(expr,'=');		// point to =
	p++;
	error = 0;
	res = eval(p);			// return value of var to start with
	if (error) {
		prout(ERR28);   // bad expression
		return ERROR_RETURN;
	}
	if (expr[0] >= 'a' && expr[0] <= 'z') {
		forvar = expr[0];
		intvar[(unsigned char)forvar-'a'] = res;
	}
	
	/* get final var */
	final[strlen(final)]='\n';
	res = eval(final);
	if (error) {
		prout(ERR28);   // bad expression
		return ERROR_RETURN;
	}
	tovar = res;
	
	/* get step size */
	if (atoi(stepsize)==0) 
		res = 1;
	else {
		stepsize[strlen(stepsize)] = '\n';
		res = eval(stepsize);
	}
	if (error) {
		prout(ERR28);   // bad expression
		return ERROR_RETURN;
	}
	if (res == 0) 
		forstep = 1;
	else
		forstep = res;

	/* set return address */
	res = setlinenumber(linenum,1);
	if (res == ERROR_RETURN) return ERROR_RETURN;
	foraddr = res;

	return NORMAL_RETURN;
}


/* ************* */
/*     NEXT      */
/* ************* */
int parse_next(char line[]) {
char linenum[6]={}, keyword[8]={}, var[4]={};
int res=0;
char varname;


	sscanf(line,"%s %s %s ",linenum,keyword,var);

	/* get var to test */
	varname = var[0];
	if (!(varname >= 'a' && varname <= 'z')) {
		prout(ERR31);   // bad variable
		return ERROR_RETURN;
	}
	if (varname != forvar) {
		prout(ERR32);   // next w/o for
		return ERROR_RETURN;
	}

	/* get the next var, add (subtract) it and save it */
	res = intvar[(unsigned char)varname-'a'];
	res += forstep;
	intvar[(unsigned char)varname-'a'] = res;
	
	/* if counting up  */
	if (forstep > 0) {
		if (res > tovar) {
			forvar = '\0';	// clear for vars
			forstep = 0;
			foraddr = 0;
			tovar = 0;
			return setlinenumber(linenum,1);
		}
		else
			return foraddr;
	}
	
	/* if counting down */
	if (forstep < 0) {
		if (res < tovar) {
			forvar = '\0';	// clear for vars
			forstep = 0;
			foraddr = 0;
			tovar = 0;
			return setlinenumber(linenum,1);
		}
		else
			return foraddr;
	}
	prout(ERR33);   // unexpected next error
	return ERROR_RETURN;

}



/* *********** */
/*    INPUT    */
/* *********** */
int parse_input(char line[]) {
	char *p;
	char lineno[6]={};
	sscanf(line,"%s ",lineno);	// get line number for error messages
	char temp[12]={};

	p = strstr(line,"input");
	if (p == NULL) {
		prout(ERR27);   // bad format in line
		return ERROR_RETURN;
	}

	while (*p++ != ' ');		// point to 1st non-blank after 'input'

	while (1) {		// input loop
		if (*p == '\n') return NORMAL_RETURN;

		if (*p == ',') {
			prout("   ");
			p++;
			continue;
		}

		if (*p == ';') {		// do-nothing
			p++;
			continue;
		}

		if (*p == ' ') {		// ignore spaces
			p++;
			continue;
		}

		if (*p == '"') {        // double quotes - print everything until next double quotes
            p++;
            while (1) {
                if (*p == '"') break;
                sprintf(printmessage,"%c",*p);
				prout(printmessage);
                p++;
            }
            p++;        // increment past "
            continue;
        }

		// load an integer variable
		if (*p >= 'a' && *p <= 'z') {
			memset(temp,0,12);

			#ifdef posix
			fgets(temp,11,stdin); 
			#endif

			#ifdef arduino 
			sgets(temp); 
			#endif

			intvar[(unsigned char)*p-'a'] = atoi(temp);
			p++;
			continue;
		}

		prout(ERR2);    // syntax error in line
		return ERROR_RETURN;
	}
	prout(ERR17);   // unexpected error in line
	return ERROR_RETURN;
}


/* ******** */
/* FILEOPEN */
/* ******** */
int fileopen(char fname[],char mode[]) {
// open a file for fileread, filewrite. Error if already open.
    if (strlen(fname)==0) {
        prout(ERR34);       // usage:
        return ERROR_RETURN;
    }
    
    // open a file for read/append
#ifdef posix
	if (diskfile != NULL) {
		prout(ERR35);	// file already open
		return ERROR_RETURN;
	}
	if (mode[0] == 'w' || mode[0] == 'W')
		diskfile = fopen(fname,"w");
	else if (mode[0] == 'r' || mode[0] == 'R')
		diskfile = fopen(fname,"r");
	else {
		prout(ERR36);	// bad mode in fileopen
		return ERROR_RETURN;
	}
	if (diskfile == NULL) {
		prout(ERR16);	// file not found
		perror("");
		return ERROR_RETURN;
	}
	return NORMAL_RETURN;
#endif

#ifdef arduino
    if (sdFile != NULL) {
        prout(ERR35);   // file already open
        return ERROR_RETURN;
    }
    if (mode[0] == 'w' || mode[0] == 'W')
        sdFile = SD.open(fname,FILE_WRITE);
    else if (mode[0] == 'r' || mode[0] == 'R')
        sdFile = SD.open(fname);
    else {
        prout(ERR36);   // bad mode in fileopen
        return ERROR_RETURN;
    }
    if (sdFile == NULL) {
        prout(ERR16);   // file not found
        return ERROR_RETURN;    
    }
    return NORMAL_RETURN;
#endif 
}



/* ********* */
/* FILECLOSE */
/* ********* */
int fileclose(void) {
// close an already open file. Error if already closed.    
#ifdef posix
	if (diskfile == NULL) {	// file not open
		prout(ERR37);
		return ERROR_RETURN;
	}
	fclose(diskfile);
	diskfile=0;	// null it
	return NORMAL_RETURN;
#endif
#ifdef arduino
    if (sdFile == NULL) {
        prout(ERR37);       // file not open
        return ERROR_RETURN;
    }
    sdFile.close();		// automagically nulls pointer
    //sdFile = NULL;
    return NORMAL_RETURN;
#endif

    return 0;
}


/* ********* */
/* FILEWRITE */
/* ********* */
int filewrite(char line[]) {
// write values to open file
char *p; char temp[20]={};
int res=0, cnt=0;

#ifdef arduino
    if (sdFile == NULL) {
        prout(ERR38);   // no file open for write
        return ERROR_RETURN;
    }
#endif

#ifdef posix
	if (diskfile == NULL) {
		prout(ERR38);	// no file open for write 
		return ERROR_RETURN;
	}
#endif
	
    p = line;
    p = strstr(line,"filewrite");    // point to begin of statement
    while (islower(*p++));             // point to space after
    
    if (*p == '\n') {   // write empty line
        #ifdef arduino
        sdFile.write('\n');
        #endif
		#ifdef posix
		fprintf(diskfile,"\n");
		#endif
        return NORMAL_RETURN;
    }
    
    while (1) {    
        if (p-line > strlen(line)) {
            prout(ERR39);       // unterminated line
            return ERROR_RETURN;
        }
        if (*p == '\0') return NORMAL_RETURN;    
    
        if (*p == ' ') {            // skip blanks
            p++;
            continue;
        }
        if (*p >= 'a' && *p <= 'z') {   // write variable contents
            res = intvar[*p - 'a'];
            #ifdef arduino
            sdFile.print(res);
            #endif
			#ifdef posix
			fprintf(diskfile,"%d",res);
			#endif
            p++;
            continue;            
        }
        if (*p == '"') {    // write everything inside double quotes
            p++;    // skip past "
            while (*p != '"') {
                #ifdef arduino
                sdFile.write(*p++);
                #endif
				#ifdef posix
				fprintf(diskfile,"%c",*p++);
				#endif
            }
            if (*p == '"') p++;        // skip past term quote
            continue;
        }
        if (*p == ',') {    // write 3 spaces
            #ifdef arduino
            sdFile.write("   ");
            #endif
			#ifdef posix
			fprintf(diskfile,"   ");
			#endif
            p++;
            continue;
        }
        if (*p == ';') {    // do nothing 
            p++;
            continue;
        }
        if (*p == '@') {    // write array var
            memset(temp,0,20);
            cnt=0;
            while (*p != ')') 
                temp[cnt++]=*p++;
            temp[cnt++]=*p++;       // write )
            //temp[cnt]='\n';         // and terminating newline
            error = 0;
            res = eval(temp);
            if (error) return ERROR_RETURN; // eval failed
            #ifdef arduino
            sdFile.print(res);
            #endif
			#ifdef posix
			fprintf(diskfile,"%d",res);
			#endif
            continue;
        }
        if (*p == '\n' && *(p-1) == ';') {
            return NORMAL_RETURN;
        }
        if (*p == '\n' && *(p-1) != ';') {
            #ifdef arduino
            sdFile.write('\n');
            #endif
			#ifdef posix
			fprintf(diskfile,"\n");
			#endif
            return NORMAL_RETURN;
        }
   
        prout(ERR7);    // bad char in line
        return ERROR_RETURN;
    }

    return NORMAL_RETURN;   // probably won't get here
}

/* ******** */
/* FILEREAD */
/* ******** */
int fileread(char line[]) {
char *p; char temp[MAXLINE]={}, ch;
int res=0, cnt=0;

#ifdef arduino
    if (sdFile == NULL) {
        prout(ERR40);   // no file open for read
        return ERROR_RETURN;
    }
#endif
#ifdef posix
	if (diskfile == NULL) {
		prout(ERR40);	// no file open for read
		return ERROR_RETURN;
	}
#endif


    p = line;
    p = strstr(line,"fileread");    // point to begin of statement
    while (islower(*p++));             // point to space after

    while (1) {     // read chars until vars run out or -1 returned
        if (*p == '\n' || *p == '\0') return NORMAL_RETURN;

        if (*p == ',' || *p == ' ') {   // ignore space, commas
            p++;
            continue;
        }
        
        if (*p >= 'a' && *p <= 'z' ) {  // read data into a variable
            memset(temp,0,MAXLINE);
            cnt=0;
            while (1) {
				#ifdef arduino
                ch = sdFile.read();
				#endif
				#ifdef posix
				ch = fgetc(diskfile);
				#endif
                if (ch == -1) break; // EOF
                if ((!(isdigit(ch))) || ch==',') break;
                temp[cnt++] = ch;    
            }
            if (ch == -1) 
                res=ch;
            else
                res = atoi(temp);
            intvar[*p - 'a'] = res;
            // if no more data, -1 is returned
            if (res == -1) return NORMAL_RETURN;
            p++;
            continue;
        }
        
        prout(ERR7);    // bad char in line
        return ERROR_RETURN;      

    }


}



/* *********** */
/*    PRINT    */
/* *********** */
int parse_print(char line[]) {	// the entire line is passed
	char *p;
	char lineno[6]={}, temp[MAXLINE]={};		// create an expression to send to eval()
	int cnt=0, result=0;
	int linelen = strlen(line);
    
	sscanf(line,"%s ",lineno);
	
	p = strstr(line,"print");	// point to print statement
	if (p == NULL) {
		prout(ERR27);   // bad format
		return ERROR_RETURN;
	}
	
	error = 0;
	
	while (*p++ != 't');		// point to 1st non-blank after 'print'
	if (*p == '\n') {           // print a blank line
        prout("\r\n");
        return NORMAL_RETURN;
	}
   
	while (1) {		// loop thru 
		if (p-line > linelen) {
				prout(ERR39);   // unterminated line
				return ERROR_RETURN;
		}
	
		// print the value of array @(x)
		if (*p == '@' && *(p+1) == '(') {
			error = 0;
			p+=2;
			char temp[8]={}; int cnt=0;
			while (*p != ')')
				temp[cnt++]=*p++;
			temp[cnt]='\n';
			int res = eval(temp);
			sprintf(printmessage,"%d",intarray[res]);
			prout(printmessage);
			p++;
			continue;
		}


		if (*p == '\n' && *(p-1) == ';') 
			return NORMAL_RETURN;
	
		if (*p == '\n' && *(p-1) != ';') {
			prout("\r\n");
			return NORMAL_RETURN;
		}

		if (*p == ',') {		// comma (3 spaces)
			prout("   ");
			p++;
			continue;
		}
		
		if (*p == ';') {		// semi-colon - do nothing
			p++;
			continue;
		}

		if (*p == ' ') {		// ignore spaces
			p++;
			continue;
		}

		if (*p == '"') {		// double quotes - print everything until next double quotes
			p++;
			while (1) {
				if (*p == '"') break;
				sprintf(printmessage,"%c",*p);
				prout(printmessage);
				p++;
				if (p-line > linelen) {
					prout(ERR39);   // unterminated line
					return ERROR_RETURN;
				}
			}
			p++;		// increment past "
			continue;
		}
		
		if ((*p >= 'a' && *p <= 'z') && 
			(*(p+1)==',' || *(p+1)==';' || *(p+1)=='\n')) {	// print value of integer variable
				sprintf(printmessage,"%d",intvar[(unsigned char)*p-'a']);		// but only if followed by , or ;  or \n
				prout(printmessage);
				p++;							// (otherwise it messes up eval below)
				continue;
		}

		// evaluate an expression
		memset(temp,0,MAXLINE); result=0; cnt=0;
		while (1) {
			temp[cnt++]=*p++;
			if (*p == '\n' || *p == ',' || *p == ';') break;
		}
		temp[cnt]='\n';		// keep eval() happy
		result = eval(temp);
		if (error) {
			prout(ERR28);   // bad expression
			return ERROR_RETURN;
		}
		sprintf(printmessage,"%d",result);	
		prout(printmessage);
		continue;

		
	}
	// should never get here
	prout(ERR17);   // unknown error in line
	return ERROR_RETURN;
}




/* Used in GOTO/GOSUB to find address vs line number */
/* if curnext == 0, return start address for line in opt */
/* if curnext == 1, return start address for line # +1 in opt */
int setlinenumber (char opt[20],int curnext) {
	int n, startpos=0, pos=0;
	char basicline[MAXLINE]={};
	char linenum[6]={};

	/* get a line */
loop:
		for (n=0; n<MAXLINE; n++) {
            basicline[n] = buffer[pos];
            if (buffer[pos] == '\n') break;
            pos++; if (pos >= position) break;
        }

        if (pos++ >= position) {
            prout(ERR8);    // line not found
			return ERROR_RETURN; 
        }

		sscanf(basicline,"%s ",linenum);	// get linenum
		if (atoi(linenum) == atoi(opt)) {
			if (!curnext) return startpos;		// return address of linenum
			if (curnext) return pos;			// return address of linenum+1
		}
		memset(basicline,0,MAXLINE);
		startpos = pos;
		goto loop;
}

#ifdef arduino
/* *********** */
/* analog read */
/* *********** */
int dueanalog(int val) {
    // the compiler decodes the A0-A11 in analogRead - can't
    // be done at runtime. So we do this ugly piece of code    

    switch (val) {
        case 0:
        return analogRead(A0);
        break;
        case 1:
        return analogRead(A1);
        break;
        case 2:
        return analogRead(A2);
        break;
        case 3:
        return analogRead(A3);
        break;
        case 4:
        return analogRead(A4);
        break;
        case 5:
        return analogRead(A5);
        break;
        case 6:
        return analogRead(A6);
        break;
        case 7:
        return analogRead(A7);
        break;
        case 8:
        return analogRead(A8);
        break;
        case 9:
        return analogRead(A9);
        break;
        case 10:
        return analogRead(A10);
        break;
        case 11:
        return analogRead(A11);
        break;
        default:
        return -1;
    }
}
#endif


/* *************** */
/*    isoperand    */
/* *************** */
// return 1 if supplied char is an operand
int isoperand(char ch) {
	if (ch == '+' ||	// plus
		ch == '-' ||	// subtract
		ch == '*' ||	// multiply
		ch == '/' ||	// divide
		ch == '%' ||	// modulo
		ch == '&' ||	// and
		ch == '|' ||	// or
		ch == '^' ||	// xor
		ch == 'E' ||	// exponent ie 2E3 = 2*2*2
		ch == '~' ) 	// compliment
		return 1;
	else
		return 0;
}


/* ****************************** */
/* evaluate arithmetic expression */
/* ****************************** */
int eval (char *expr) {

int lvalue=0, rvalue=0, cnt=0, index=0, res=0;
char operand='\0';
char value[30]={};
int MINUSFLAG=0;
char *p;

	// all other routines return an address or return value. This returns an integer value.
	
	if (DEBUG) {
		sprintf(printmessage,"in eval: [%s]\r\n",expr);
		prout(printmessage);
	}

	if (*expr == '\n' || *expr == '\0') {
		error = 1;
		return ERROR_RETURN;			// initial error check for empty expression
	}

#ifdef arduino
	/* arduino specific commands */
	error = 0;
	p = strstr(expr,"pinread(");	// test pinread
	if (p != NULL) {
		while (*p++ != '(');	// point to variable
        //Serial.print("*p="); Serial.println(*p);
		if (*p >= 'a' && *p <= 'z') {	// use var as pin #
			res = intvar[*p - 'a'];		// get pin #
			//Serial.print("pin number "); Serial.println(res,DEC);
			pinMode(res,INPUT_PULLUP);
			if (*(p+1) != ')') error = 1;	// trailing )
			return digitalRead(res);
		}
		// must be a number
		if (isdigit(*p)) {
            memset(value,0,30);
		    cnt=0;
			while (isdigit(*p)) 
				value[cnt++] = *p++;
			res = atoi(value);
            //Serial.print("pin number "); Serial.println(res,DEC);
			pinMode(res,INPUT_PULLUP);
			if (*p != ')') error = 1;	// trailing )
			return digitalRead(res);
		}
        
		// maybe analog (A0-A11)
		if (*p == 'A') {
			memset(value,0,30);
			cnt = 0;
			p++;		// point to digit after A
			while(isdigit(*p)) 
					value[cnt++] = *p++;
			if (atoi(value) < 0 || atoi(value) > 11) {	// bad number
				prout(ERR43);   // bad pin number
				error = 1;
				return ERROR_RETURN;
			}
			res = dueanalog(atoi(value));
            if (res == -1) error = 1;
			return res;
		}
		prout(ERR43);   // bad pin number
		error = 1;
		return ERROR_RETURN;
	}
#endif

	// test for functions
	
	p = strstr(expr,"abs(");		// abs(x) w/x is a-z only
	if (p != NULL) {
		while (*p++ != '(');	// pointing to variable
		if (*p >= 'a' && *p <= 'z') {	// it's a var
			if (intvar[*p - 'a'] < 0)
				res = intvar[*p - 'a'] * -1;
			else
				res = intvar[*p - 'a'];
			if (*(p+1) != ')') error = 1;
			return res;
		}
	}

	p = strstr(expr,"random()");	// return random number 
	if (p != NULL) {
		#ifdef posix
		res = random();
		#endif
		#ifdef arduino
		res = random(0,MAXRAND);	// set in defines at top
		#endif
		return res;
	}



evalloop:
	memset(value,0,30); cnt=0; 
	MINUSFLAG=0;
	if (*expr == '-') {
		MINUSFLAG=1;
		expr++;
	}

	if (*expr == '\n' || *expr == '\0' || *expr == ',' || *expr == ' ')  {
		if (operand != '\0')
			lvalue = domath(lvalue,operand,rvalue);
		printf("top: lvalue=%d\n",lvalue);
		return lvalue;
	}

	// test numbers
	if (isdigit(*expr)){
		while (1) {
			if (!(isdigit(*expr))) break;
			value[cnt++] = *expr++;
		}
		rvalue = atoi(value);
		if (MINUSFLAG) rvalue *= -1;
		if (*expr == '\n' || *expr == '\0' || *expr == ',' || *expr == ' ')  {
			if (operand == '\0') 
				return rvalue;
			else {
				lvalue = domath(lvalue,operand,rvalue);
				return lvalue;
			}
		}
		if (operand != '\0') {	// middle of an expresion
			lvalue = domath(lvalue,operand,rvalue);
			operand = '\0';
			rvalue = 0;
			if (isoperand(*expr)) 
				operand = *expr++;
			goto evalloop;
		}
		if (isoperand(*expr)) {		// 1st pass thru
			operand = *expr;
			expr++;
			lvalue = rvalue; rvalue = 0;
			goto evalloop;
		}
		goto evalloop;
		
	}

	// test letters
	if (*expr >= 'a' && *expr <= 'z') {
		rvalue = intvar[*expr - 'a'];
		if (MINUSFLAG) rvalue *= -1;
		expr++;
		if (*expr == '\n' || *expr == '\0' || *expr == ',' || *expr == ' ') {
			if (operand == '\0')
				return rvalue;
			else {
				lvalue = domath(lvalue,operand,rvalue);
				return lvalue;
			}
		}
		if (operand != '\0') {  // middle of an expresion
            lvalue = domath(lvalue,operand,rvalue);
            operand = '\0';
            rvalue = 0;
            if (isoperand(*expr))
                operand = *expr++;
            goto evalloop;
        }
		if (isoperand(*expr)) {
            operand = *expr;
			expr++;
			lvalue = rvalue; rvalue = 0;
            goto evalloop;
        }
		goto evalloop;
	}

	// test array
	if (*expr == '@' && *(expr+1) == '(') {
		expr += 2;
		if (isdigit(*expr)) {	// digits in array index
			while (1) {
				value[cnt++] = *expr++;
				if (isdigit(*expr)) continue;
				break;
			}
			index = atoi(value);
		}
		if (*expr >= 'a' && *expr <= 'z') {	// var in array index
			index = intvar[*expr - 'a'];
            expr++;
		}
		if (*expr != ')') {		
			prout(ERR44);   // missing closing )
			error = 1;
			return ERROR_RETURN;
		}
		expr++;	// point past ')'
       
		if (index >= arraymax) {
			prout(ERR45);   // array bounds error
			error = 1;
			return ERROR_RETURN;
		}
		rvalue = intarray[index];
		if (MINUSFLAG) rvalue *= -1;
		if (*expr == '\n' || *expr == '\0' || *expr == ',' || *expr == ' ') {
			if (operand == '\0')
				return rvalue;
			else {
				lvalue = domath(lvalue,operand,rvalue);
				return lvalue;
			}
		}
		if (operand != '\0') {	// mid expr
			lvalue = domath(lvalue,operand,rvalue);
			operand = '\0';
			rvalue = 0;
			if (isoperand(*expr))
				operand = *expr++;
			goto evalloop;
		}
		if (isoperand(*expr)) {
			operand = *expr;
			expr++;
			lvalue = rvalue; rvalue = 0;
			goto evalloop;
		}
		goto evalloop;
	}

	error = 1;
	return ERROR_RETURN;
}


int domath(int lvalue, char operand, int rvalue) {
int res = 0;

	switch (operand) {
		case '\0':
			return 0;
			break;
		case '+':	// add
			return lvalue + rvalue;
			break;
		case '-':	// subtract
			return lvalue - rvalue;
			break;
		case '*':	// multiply
			return lvalue * rvalue;
			break;
		case '/':	// divide
			if (rvalue == 0) {
				prout(ERR46);   // divide by zero
				error = 1;
				return ERROR_RETURN;
			}
			return lvalue / rvalue;
			break;
		case '%':	// modulo (remainder)
			if (rvalue == 0) {
				prout(ERR46);   // divide by zero
				error = 1;
				return ERROR_RETURN;
			}
			return lvalue % rvalue;
			break;
		case '&':	// and
			return lvalue & rvalue;
			break;
		case '|':	// or
			return lvalue | rvalue;
			break;
		case '^':	// xor
			return lvalue ^ rvalue;
			break;
		case '~':	// compliment lvalue: NOTE rvalue unused
			return ~lvalue;
			break;
		case 'E':	// exponent ie 2E3 = 2*2*2
			res = lvalue;
			for (int n=1; n < rvalue; n++)
				res = res * lvalue;
			return res;
			break;

		default:	// really shouldn't get here
			prout(ERR47);   // unknown operand
			error = 1;
			return ERROR_RETURN;
		}
}



/* ************************************************ */
/*    Evaluate Logical Expression (used in if/then) */
/* ************************************************ */
int evallogic(char *expr) {		// logical evaluation, return 1 if true, 0 if false
char operand = '\0';
char value[20]={};
int lvalue=0, rvalue=0;
int cnt=0;

	// 1st char MUST be a variable
	if (*expr >= 'a' && *expr <= 'z') {
		lvalue = intvar[(unsigned char)*expr-'a'];	// get value of variable
		expr++;		// point to '=' after variable
	}

	if (*expr == '@' && *(expr+1) == '(') {
		expr+=2;
		if (*expr >= 'a' && *expr <= 'z') {
			int index = intvar[(unsigned char)*expr - 'a'];
			lvalue = intarray[index];
			expr++; // point to ');
			expr++;	// point to '='
		}
		else {
			prout("index to array must be a variable (a-z)\r\n");
			error = 1;
			return ERROR_RETURN;
		}
	}

		//expr++;		
		operand = *expr;		// get comparison operator

		expr++;					// point to test 
		// test digits 0-9
		if (isdigit(*expr)) {	// get digits until \0
			while (*expr != '\0')
				value[cnt++] = *expr++;
			 rvalue = atoi(value);
			 goto logictest;
		}
		// test variables
		if (*expr >= 'a' && *expr <= 'z') {
			rvalue = intvar[(unsigned char)*expr-'a'];
			goto logictest;
		}
		// neither number or var - show error and return
		prout("logical eval error");
		error=1;
		return ERROR_RETURN;
	

logictest:	// test variable against rvalue
		switch (operand) {
			case '=':			// equal
				return (lvalue == rvalue);
				break;
			case '#':			// not equal
				return (lvalue != rvalue);
				break;
			case '<':			// less than
				return (lvalue < rvalue);
				break;
			case '>':			// greater than
				return (lvalue > rvalue);
				break;
			case '&':			// logical and
				return (lvalue & rvalue);
				break;
			case '|':			// logical or
				return (lvalue | rvalue);
				break;
			case '^':			// logical xor
				return (lvalue ^ rvalue);
				break;
			default:
				sprintf(printmessage,"unknown operand [%c]",operand);
				prout(printmessage);
				error=1;
				return ERROR_RETURN;
		}

		// end of tests
	
	prout(ERR17);   // unknown error
	error = 1;
	return ERROR_RETURN;
}
