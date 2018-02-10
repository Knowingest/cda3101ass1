#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

struct mipsline{
    short haslabel;             //0 or 1
    int address;             //address
    int laddress;           //address of second line (for la)
    char mips[80];           //original command stored in cstring
    char lamips[80];        //seperate line of mips for lui commands
    char labelname[20];    //label (if any)
    int labeladdress;   //the address that the label points to
    int binary;          //the binary translation of the mips code
    int labinary;       //binary translation for second line (for la)
};

struct symboltable{
    int address;
    char labelname[20];
};

/*
#1  ADD     R   32  add $rd,$rs,$rt
#2  ADDI    I   8   addi $rt,$rs,immed
#3  NOR     R   39  nor $rd,$rs,$rt
#4  ORI     I   13  ori $rt, $rs, immed
#5  SLL     R   0   sll $rd, $rt, shamt
#6  LUI     I   15  lui $rt, immed
#7  SW      I   43  sw $rt,immed($rs)
#8  LW      I   35  lw $rt,immed($rs)
#9  BNE     I   5   bne $rs,$rt,label
#10 J       J   2   j label
#11 LA      -   -   la $rx,label
*/
int add(char* c);
int addi(char* c);
int nor(char* c);
int ori(char* c);
int sll(char* c);
int lui(char* c);
int sw(char* c);
int lw(char* c);
int bne(char* c, int address, struct symboltable* s);
int jump(char* c, struct symboltable* s);
void la(char* c, struct mipsline* m, struct symboltable* s);

void str_copy(char* src, char* dst, unsigned short amount);
void null_line(struct mipsline* m);
void null_symbol(struct symboltable* s);
short regnum(char* c);
short findlabel(char* c);
void copylabel(char* src, char* dst);
void read_data(struct mipsline* m, struct symboltable* s, unsigned short amount);
void translate(struct mipsline* m, struct symboltable* s, unsigned short amount);

int main(void)
{
    //create main array
    struct mipsline data[100];
    struct symboltable symbols[100];

    //zero out the array of structures
    short i;
    for (i = 0; i < 100; i++)
    {
        null_line(&data[i]);
        null_symbol(&symbols[i]);
    }

        //this reads 100 lines into our tables
    read_data(data, symbols, 100);
    translate(data, symbols, 100);

    for (i = 0; i < 100; i++)
    {
        if (data[i].binary == 0) break;
        //printf("0x%08x : %s : 0x%08x\n", data[i].address, data[i].mips, data[i].binary);

        if (data[i].labinary == 0) printf("0x%08x : 0x%08x\n", data[i].address, data[i].binary);
        else printf("0x%08x : 0x%08x\n0x%08x : 0x%08x\n", 
            data[i].address, data[i].binary,
            data[i].laddress, data[i].labinary);
    }

    return 0;
}

void null_line(struct mipsline* m)
{
    m->haslabel = 0;
    m->address = -1;
    m->laddress = -1;
    m->mips[0] = (char) 0;
    m->lamips[0] = (char) 0;
    m->labelname[0] = (char) 0;
    m->binary = 0;
    m->labinary = 0;
}

void null_symbol(struct symboltable* s)
{
    s->address = -1;
    s->labelname[0] = 0;
}

void dump_symbols(struct symboltable* s)
{
    printf("dumping symbols....\n");
    int i = 0;
    for (i = 0; i < 100; i++)
    {
        if (s[i].address == -1)
        {
            printf("breaking symbol dump on interation %d\n", i);
            break;
        }
        printf("%s : 0x%08x\n", s[i].labelname, s[i].address);
    }
}

void str_copy(char* src, char* dst, unsigned short amount)
{
    unsigned short i;
    for (i = 0; i < amount; i++)
    {
        dst[i] = src[i];
    }
}

short regnum(char* c)       //returns numeric value of register in $(letter)(number) format
{
    if (c[0] != '$') return -1;             //return error if not pointed to a register
    if (c[1] == '0') return 0;              
    if (c[1] == 't') return ( (short) c[2] ) - 40;//based on ascii - offset
    if (c[1] == 's') return ( (short) c[2] ) - 32;  // - 40 is equivalent to numerical value + 8
}                                                  // - 32 is equivalent to numerical value + 16

short findlabel(char* c)
{
    unsigned short i;
    for(i = 0; i < 20; i++)         //Original concept:  for(i = 0; c[i] != '\n'; i++)  < --- doesn't work for some reason, crashes immediately
    {
        if (c[i] == ':') return 1;
        if (c[i] == (char) 0) return 0;
    }
    return 0;
}

void copylabel(char* src, char* dst)
{
    short i;
    for (i = 0; i < 20 && src[i] != ':'; i++)
    {
        dst[i] = src[i];
    }
}

void read_data(struct mipsline* m, struct symboltable* s, unsigned short amount)
{
	short directive_index = -1;  //-1 means wer are at the start, 0 means .text, 1 means .data
    unsigned short symbol_index = 0;	 //used to count through the symbol table
    unsigned int address_index = 0;   //we need a seperate counter because the la command counts as two lines
    unsigned int data_offset = 0;   //offset for the extra data allocated in the .data section

    unsigned short i;				   //for loop index
    char* c;

    //////////////////
    //main loop
    //////////////////
    for (i = 0; i < amount; i++)
    {
        scanf(" %[^\n]", m[i].mips);                 //copy mips code
        if (m[i].mips[0] == (char) 0) break;		//if we just read NOTHING, break
        //printf("found line: %s\n", m[i].mips);

        if (directive_index >= 1)   //this means we're in the .data section
        {
            m[i].haslabel = 1;
            copylabel(m[i].mips, m[i].labelname);
            m[i].address = (address_index++ * 4) + data_offset;

            c = strchr(m[i].mips, '.');
            if (c[1] == 's')
            {
                strchr(c, ' '); //c now points to start of n (".space n")
                c = &c[1];
                data_offset = data_offset + (atoi(c)); //add n bytes to the address_index
            }
            else
            {
                data_offset = data_offset + 4;
            }

            copylabel(m[i].labelname, s[symbol_index].labelname);
            s[symbol_index++].address = m[i].address;

            continue;
        }


        	//check if the line we read had a directive
        if (strchr(m[i].mips, '.') != 0)
        {
            printf("found directive\n");
        	if (++directive_index >= 1)
                continue;  //<--- we have now entered .data
            --i;    //semantically speaking: ignore this line, go scanf again
            continue;
        }

        /////////////
        //find label
        //////////////
            if (strchr(m[i].mips, ':') != 0)
                {
                    printf("found a label while reading\n");
                    m[i].haslabel = 1;
                    copylabel(m[i].mips, m[i].labelname);
                }       

        ///////////////////
        //determine address
        ///////////////////
            m[i].address = address_index++ * 4; //main address

            c = m[i].mips;     //point to start of code

            if (m[i].haslabel == 1)
                    c = strchr(m[i].mips, ':'); //skip to the ':' if this line has a label

            c = strchr(c, 'l');             //account for la commands having two addresses
            if (c != 0)
                if (c[1] == 'a')        //we know we're past the label, so no false positives
                    m[i].laddress = address_index++ * 4;

        //////////////////////
        //symbol table stuff
        /////////////////////
            if (m[i].haslabel == 1)
                {
                    printf("copying label into symbol table...\n");
                    copylabel(m[i].labelname, s[symbol_index].labelname);
                    printf("label copied as: %s\n", s[symbol_index].labelname);
                    s[symbol_index++].address = m[i].address;
                }

    }
    printf("end of read loop\n");
}

void translate(struct mipsline* m, struct symboltable* s, unsigned short amount)
{
    unsigned short i = 0;   //index for lines of code
    unsigned short j = 0;   //index for characters in string
    char* c;
    for (i = 0; i < amount; i++)
    {
        if (m[i].mips[0] == (char) 0) break;    //break if we are at an empty line
        c = m[i].mips;

        if (m[i].haslabel == 1)
        {
            c = strchr(c, ':');     //skip to after the label
            while (!isalpha(*c))
                c = &c[1];
        }
        //printf("line %d starts with character %c\n", i, *c);

        if (*c == 'a')
        {
            if (c[3] == 'i')
            {
                m[i].binary = addi(c);
                continue;
            }
            else 
            {
                m[i].binary = add(c);
                continue;
            }
        }
        if (*c == 'n')
        {
            m[i].binary = nor(c);
            continue;
        }
        if (*c == 'o')
        {
            m[i].binary = ori(c);
            continue;
        }
        if (*c == 's')
            if (c[1] == 'w')
            {
                m[i].binary = sw(c);
                continue;
            }
            else
            {
                m[i].binary = sll(c);
                continue;
            }
        if (*c == 'l')
            if (c[1] == 'u')
            {
                m[i].binary = lui(c);
                continue;
            }
            else if (c[1] == 'w')
            {
                m[i].binary = lw(c);
                continue;
            }
            else
            {
                la(c, &m[i], s);
                continue;
            }
        if (*c == 'b')
        {
            m[i].binary = bne(c, m[i].address, s);
            continue;
        }
        m[i].binary = jump(c, s);
    }

}
/*
#1  ADD     R   32  add $rd,$rs,$rt
#2  ADDI    I   8   addi $rt,$rs,immed
#3  NOR     R   39  nor $rd,$rs,$rt
#4  ORI     I   13  ori $rt, $rs, immed
#5  SLL     R   0   sll $rd, $rt, shamt
#6  LUI     I   15  lui $rt, immed
#7  SW      I   43  sw $rt,immed($rs)
#8  LW      I   35  lw $rt,immed($rs)
#9  BNE     I   5   bne $rs,$rt,label
#10 J       J   2   j label
#11 LA      -   -   la $rx,label
*/
int add(char* c)
{
    int i = 0;
    int result = 32;    //opcode is 0 but func is 32
    
    while (c[i] != '$') ++i;
    result = result | (regnum(&c[i]) << 11); //add $rd to the result
    ++i;

    while (c[i] != '$') ++i;
    result = result | (regnum(&c[i]) << 21); //add $rs
    ++i;

    while (c[i] != '$') ++i;
    result = result | (regnum(&c[i]) << 16); // add $rt
    return result;
}

int addi(char* c)
{
    int i = 0;
    int result = 8 << 26; //start with opcode

    while (c[i] != '$') ++i;
    result = result | (regnum(&c[i]) << 16); //add $rt
    ++i;

    while (c[i] != '$') ++i;
    result = result | (regnum(&c[i]) << 21); //add $rs
    ++i;

    while (c[i] != ',') ++i;
    ++i;
    result = result | (atoi(&c[i] & 65535));
    return result;
}
int nor(char* c)
{
    unsigned int result = 39;
    return (result | add(c));     //we just change the opcode of add() slightly
}
int ori(char* c)
{
    int result = 13 << 26;          //we can use addi() because these are both i types
    return result | addi(c);    //overlay the opcode over the result from addi
}
int sll(char* c)
{
    int i = 0;
    int result = 0;

    while (c[i] != '$') ++i;
    result = result | (regnum(&c[i]) << 11); //$rd
    ++i;

    while (c[i] != '$') ++i;
    result = result | (regnum(&c[i]) << 16); //$rt
    ++i;

    while (c[i] != ',') ++i;
    ++i;
    result = result | (atoi(&c[i]) << 6); //shamt

    return result;
}
int lui(char* c)
{
    int i = 0;
    int result = 15 << 26;
    printf("0x%08x\n", result);
    while (c[i] != '$')
        ++i;
    result = result | (regnum(&c[i]) << 16); //add $rt
    ++i;

    while (c[i] != ',') ++i;
    ++i;
    return (result | (atoi(&c[i]) & 65535));
}
int sw(char* c)
{
    return  lw(c) | (43 << 26);
}
int lw(char* c)
{
    int i = 0;
    int result = 35 << 26;
    
    while (c[i] != '$') ++i;
    result = result | (regnum(&c[i]) << 16); //add $rt
    ++i;

    while (c[i] != ',') ++i;
    ++i;
    result = result | (atoi(&c[i])); //add immediate
    
    while (c[i] != '$') ++i;
    result = result | (regnum(&c[i]) << 21); //add $rs
    ++i;

    return result;
}
int bne(char* c, int address, struct symboltable* s)
{
    int i = 0;
    int j = 0;
    int result = (5 << 26);
    int offset = 0;

    while (c[i] != '$') ++i;
    result = result | (regnum(&c[i]) << 21); //add $rs
    ++i;

    while (c[i] != '$') ++i;
    result = result | (regnum(&c[i]) << 16); //add $rt
    ++i;

    while (c[i] != ',') ++i;
    ++i;

    //printf("%s\n", &c[i]);
    //dump_symbols(s);
    for (j = 0; j < 100; j++)
    {
        //if (s[j].address == -1) printf("no match in symboltable for bne instruction.  this is bad\n");
        if (strcmp(s[j].labelname, &c[i]) == 0)
        {
            offset = (s[j].address - address) / 4;
            break;
        }
    }

    offset = offset & 65535;    //16 zeroes and 16 ones
    return (result | offset);
}
int jump(char* c, struct symboltable* s)
{
    int i = 0;
    int j = 0;
    int result = 2 << 26;
    int address = 0;

    while (c[i] != 'j') ++i;
    ++i; ++i;
    printf("jump string is: %s\n", &c[i]);
   
    for (j = 0; j < 100; j++)
    {
        //if (s[j].address == -1) printf("no match in symboltable for bne instruction.  this is bad\n");
        if (strcmp(s[j].labelname, &c[i]) == 0)
        {
            address = s[j].address;
            break;
        }
    }

    return result | (address & 67108863);
}
void la(char* c, struct mipsline* m, struct symboltable* s)
{
    int i = 0;
    int j = 0;
    int result = 0;
    int finalreg = 0;
    int address;

    while (c[i] != '$') ++i;
    finalreg = regnum(&c[i]); //find the register we use for the ori
    ++i;

    while (c[i] != ',') ++i;
    ++i;
                //lui          //$1
    m->binary = (15 << 26) | (1 << 16);
                    //ori          //register       /$1
    m->labinary = (13 << 26) | (finalreg << 21) | (1 << 16);

    for (j = 0; j < 100; j++)
    {
        if (s[j].address == -1) printf("no match in symboltable for la instruction.  this is bad\n");
        if (strcmp(s[j].labelname, &c[i]) == 0)
        {
            address = s[j].address;
            break;
        }
    }
                                                //16 ones and 16 zeroes
    m->binary = m->binary | ((address >> 16) & 65535); //(in case sign extension happens or something)
    m->labinary = m->labinary | (address & 65535);
}