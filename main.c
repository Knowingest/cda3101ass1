#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

struct mipsline{
    short haslabel;           //0 or 1
    short address;           //address (index * 4)
    char mips[80];          //original command stored in cstring
    char labelname[20];    //label (if any)
    short labeladdress;   //the address that the label points to
    int binary;          //the binary translation of the mips code
};

struct symboltable{
    short address;
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
int bne(char* c);
int jump(char* c);
int la(char* c);

void str_copy(char* src, char* dst, unsigned short amount);
void null_line(struct mipsline* m);
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
    }

        //this reads 100 lines into our tables
    read_data(data, symbols, 100);
    translate(data, symbols, 100);

    for (i = 0; i < 100; i++)
    {
        if (data[i].mips[0] == (char) 0) break; //null character signals end of list

        printf("0x%08x : %s : 0x%08x\n", data[i].address, data[i].mips, data[i].binary);
    }

    return 0;
}

void null_line(struct mipsline* m)
{
    m->haslabel = 0;
    m->address = -1;
    m->mips[0] = (char) 0;
    m->labelname[0] = (char) 0;
    m->binary = 0;
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
    unsigned short symbol_index = 0;
    unsigned short i;

    for (i = 0; i < amount; i++)
    {
        scanf(" %[^\n]", m[i].mips);                 //copy mips code
        if (m[i].mips[0] == (char) 0) break;
        m[i].haslabel = findlabel(m[i].mips);    //determine if mips line starts with a label

            //if the line has a label, record the name of the label seperately.
        if (m[i].haslabel == 1)
            {
                        //copy label from mips code into seperate column
                copylabel(m[i].mips, m[i].labelname);
               
                        //add address|labelname pair into the symbol table
                s[symbol_index].address = m[i].address;
                str_copy(m[i].labelname, s[symbol_index++].labelname, 20);
            }        
                          //^increment the index for symbol table
        m[i].address = i * 4;    //address = i * 4
    }
}

void translate(struct mipsline* m, struct symboltable* s, unsigned short amount)
{
    unsigned short i = 0;   //index for lines of code
    unsigned short j = 0;   //index for characters in string
    char* c;
    for (i = 0; i < amount; i++)
    {
        printf("m[i].mips[0] == %c\n", m[i].mips[0]);
        if (m[i].mips[0] == (char) 0) break;
        c = &m[i].mips[0];

        if (m[i].haslabel == 1)
        {
        while (!isalpha(*c))      //increment to start of instruction
            c = &c[++j];
        c = &c[++j];
        }

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
                printf("found add\n");
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
                m[i].binary = la(c);
                continue;
            }
        if (*c == 'b')
        {
            m[i].binary = bne(c);
            continue;
        }
        m[i].binary = jump(c);
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
    
    while (c[i] != '$')
    {
        ++i;
    }
    result = result | (regnum(&c[i]) << 11); //add $rd to the result
    ++i;

    while (c[i] != '$')
    {
        ++i;
    }
    result = result | (regnum(&c[i]) << 21); //add $rs
    ++i;

    while (c[i] != '$')
    {
        ++i;
    }
    result = result | (regnum(&c[i]) << 16); // add $rt
    return result;
}

int addi(char* c)
{
    int i = 0;
    int result = 8 << 26; //start with opcode

    while (c[i] != '$')
    {
        ++i;
    }
    result = result | (regnum(&c[i]) << 16); //add $rt
    ++i;

    while (c[i] != '$')
    {
        ++i;
    }
    result = result | (regnum(&c[i]) << 21); //add $rs
    ++i;

    while (c[i] != ',')
    {
        ++i;
    }
    ++i;
    result = result | atoi(&c[i]);
    return result;
}
int nor(char* c)
{
    int result = 39;

    while (c[i] != '$')
    {
        ++i;
    }
    result = result | (regnum(&c[i]) << 16); //$rd
    ++i;

    while (c[i] != '$')
    {
        ++i;
    }
    result = result | (regnum(&c[i]) << 26); //$rs
    ++i;

    while (c[i] != '$')
    {
        ++i;
    }
    result = result | (regnum(&c[i]) << 21); //$rt

    return result
}
int ori(char* c)
{
    int result = 13 << 26;
}
int sll(char* c)
{
    return 0;
}
int lui(char* c)
{
    return 0;
}
int sw(char* c)
{
    return  0;
}
int lw(char* c)
{
    return 0;
}
int bne(char* c)
{
    return 0;
}
int jump(char* c)
{
    return 0;
}
int la(char* c)
{
    return 0;
}