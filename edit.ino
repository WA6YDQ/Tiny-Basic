/* edit.ino - simple line editor for due */

void ledit() {
    /* pre-define functions */
    void buffersave(char[],unsigned char*);
    int bufferload(char[],unsigned char*);
    int bufferdelete(char[],unsigned char*);
    int bufferinsert(char[],unsigned char*);

    
    /* setup buffer and pointers */    
    char line[MAXLINE]={};

    /* use buffer set up in basic() */
    extern unsigned char* buffer;
    unsigned char* buf = buffer;
    extern unsigned int position;
    unsigned int pos = position;
    Serial.println("\r\nOk,");  // we're ready

    /* this is the main edit loop */
    while (1) {
        memset(line,0,MAXLINE);
        Serial.print("> ");     // prompt
        sgets(line);
        if (line[0] == '\n') continue;
        
        /* exit - back to calling program */
        if (strncmp(line,"exit",4)==0) {
            return;
        }
        
        /* cls - clear the screen */
        if (strncmp(line,"cls",3)==0) {
            Serial.println("\e[H\e[J");
            Serial.println("\r\nOk,");
            continue;
        }
        
        /* list - show buffer contents */
        if (strncmp(line,"list",4)==0) {
            ledlist(buf,position);  //*
            Serial.println("\r\nOk,");
            continue;
        }

        /* new - clear the buffer, reset pointers */
        if (strncmp(line,"new",3)==0) {
            memset(buf,0,BUFSIZE);
            position = 0;   //*
            Serial.println("\r\nOk,");
            continue;
        }

        /* mem - show free memory */
        if (strncmp(line,"mem",3)==0) {
            Serial.print("Free edit memory: "); 
            Serial.print(BUFSIZE-position); //*
            Serial.println(" bytes");
            showmem();  // and show arduino stats
            Serial.println("\r\nOk,");
            continue;
        }

        /* help - show command summary */
        if (strncmp(line,"help",4)==0) {
            Serial.println("exit      exit the editor");
            Serial.println("cls       clear the screen");
            Serial.println("list      display the buffer");
            Serial.println("new       clear the buffer");
            Serial.println("mem       show memory stats");
            Serial.println("dir       display directory contents");
            Serial.println("save file save buffer to file");
            Serial.println("load file load file to buffer");
            Serial.println("a         append lines to end of buffer, .q to stop");
            Serial.println("i #       insert before line #, .q to stop");
            Serial.println("d #       delete a single line");
            Serial.println("f string  find a string in the buffer");
            Serial.println("\r\nOk,");
            continue;
        }

        /* dir [subdir] - show directory */
        if (strncmp(line,"dir",3)==0) {
            dir(line);
            Serial.println("\r\nOk,");
            continue;
        }

        /* save file - save buffer to file */
        if (strncmp(line,"save",4)==0) {
            filesave(line);
            Serial.println("\r\nOk,");
            continue;
        }

        /* load file - load file into buffer */
        if (strncmp(line,"load",4)==0) {
            fileload(line);
            Serial.println("\r\nOk,");
            continue;
        }

        /* d # - delete a line */
        if (strncmp(line,"d ",2)==0) {
            int res = bufferdelete(line,buf);  //*
            if (res > 0) position = res;    //*
            Serial.println("\r\nOk,");
            continue;
        }

        /* dump - hex dump of program listing */
        if (strncmp(line,"dump",4)==0) {
            char printmessage[MAXLINE];
            int addr = 0;
            while (addr < position) {   //*
                sprintf(printmessage,"%04X  ",addr);
                Serial.print(printmessage);
                for (int n=0; n<16; n++) {
                    sprintf(printmessage,"%02X ",buf[addr+n]);
                    Serial.print(printmessage);
                }
                Serial.print("  ");
                for (int n=0; n<16; n++) {
                    sprintf(printmessage,"%c",isprint(buf[addr+n])?buf[addr+n]:'.');
                    Serial.print(printmessage);
                }
                Serial.print("\r\n");
                addr += 16;
            }
            Serial.print("\r\n");
            continue;
        }

        /* i # - insert line(s) of text in buffer */
        if (strncmp(line,"i ",2)==0) {
            int res=0;
            res = bufferinsert(line,buf);  //*
            if (res > 0) position = res;    //*
            Serial.println("\r\nOk,");
            continue;
        }


        /* a - append text to end of buffer */
        if (line[0] == 'a') {
            append();
            Serial.println("\r\nOk,");
            continue;
        }

        /* f [string] - find a string in the buffer */
        if (strncmp(line,"f ",2)==0) {  
            find(line);
            Serial.println("\r\nOk,");
            continue;
        }

        
        Serial.println("syntax error");
        continue;

        
    } // end of main loop
    

    
}


/* append to end of text */
void append(void) {
    char line[MAXLINE];
    int n=0;
    while (1) {
        memset(line,0,MAXLINE);
        Serial.print("a> ");     // prompt
        sgets(line);
        if (strncmp(line,".q",2)==0) return;
        for (n=0; n < strlen(line); n++)
            buffer[position++] = line[n];
        continue;
    }
        
}

/* insert before line (ie ins 5) */
int bufferinsert(char cmdline[MAXLINE],unsigned char *buffer) {//*
char cmd[12]={},linenum[12]={}, line[MAXLINE];
int linen=0, start=-1, end=1, n=0, ctr=1, len=0, i=0;
    sscanf(cmdline,"%s %s ",cmd,linenum);
    linen = atoi(linenum);
    if (linen == 0) {  // bad line number
        Serial.println("bad line number");
        return -1;
    }
    for (n=0; n<position; n++) { //*
        if (buffer[n] == '\n') {
            end = n;
            if (linen == ctr) break;    // match
            start = end;      // skip \n at EOL
            end = start+1;
            ctr++;              // next line number
            continue;
        }
    }
    if (n == position || linen != ctr) {    // no match
        Serial.println("line not found");
        return -1;
    }
    // line num match. line is start->end
    start += 1; // past \n from prev line
loop:    // start insert at start
    Serial.print("i> ");
    memset(line,0,MAXLINE);
    sgets(line);     // get a line from the user
    if (strncmp(line,".q",2)==0) return position;   // done here
    len = strlen(line);
    /* shift up buffer by strlen(line) */
    memmove(&buffer[start+len],&buffer[start],position-start);
    position += len;
    /* and insert new line */
    for (i=start, n=0; i<=start+(strlen(line)-1); i++)
        buffer[i] = line[n++];
    start = i;
    /* keep doing until .q entered */
    goto loop;
}

    

/* delete a line given a line number (ie del 10) */
int bufferdelete(char line[MAXLINE],unsigned char *buffer) {
char cmd[12]={},linenum[12]={};
int linen=0, start=-1, end=1, n=0, ctr=1;
    sscanf(line,"%s %s ",cmd,linenum);
    linen = atoi(linenum);
    if (linen == 0) {  // bad line number
        Serial.println("bad line number");
        return -1;
    }
    for (n=0; n<position; n++) {
        if (buffer[n] == '\n') {
            end = n;
            if (linen == ctr) break;    // match
            start = end;      // skip \n at EOL
            end = start+1;
            ctr++;              // next line number
            continue;
        }
    }
    if (n == position || linen != ctr) {    // no match
        Serial.println("line not found");
        return -1;
    }
    // line num match. line is start->end
    end += 1; start += 1;  // skip past \n at EOL
    while (end < position)
        buffer[start++] = buffer[end++]; 
    for (n=start; n<BUFSIZE; n++) buffer[n] = '\0'; // set mem to NULL
    return start;   // new position value
}


/* show buffer contents */
void ledlist(unsigned char *buffer, unsigned int position) {
unsigned char ch; char temp[12]={};   
int n=0, lineno=1;
    if (position == 0) return;
    sprintf(temp,"%04d  ",lineno);
    Serial.print(temp);
    for (n=0; n<position-1; n++) {
        ch = buffer[n];
        if (ch == '\n') {
            Serial.print("\r\n");
            lineno += 1;
            sprintf(temp,"%04d  ",lineno);
            Serial.print(temp);
            continue;
        }
        Serial.write(ch); 
    }
    Serial.println("\r\nOk,");
    return;
}


void find(char temp[]) {
char cmd[6]={}, sstring[MAXLINE]={}, line[MAXLINE]={};
    char *p;
    sscanf(temp,"%s %s ",cmd,sstring);
    int n, linenum=0, i=0, start=0;
    while (1) {
        memset(line,0,MAXLINE);
        i=0; linenum++;
        for (n=start; n<position; n++) {
            line[i++]=buffer[n];
            if (buffer[n] == '\n') break;
        }
        if (n >= position) return;
        // we have a line
        start = n+1;    // skip \n
        p = strstr(line,sstring);
        if (p==NULL) continue;
        // match - show line w/string
        sprintf(temp,"%04d ",linenum);
        Serial.print(temp);
        // get rid of the \n
        line[strlen(line)-1] = '\0';
        Serial.println(line);
        continue;
    }
}
