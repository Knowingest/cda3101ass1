#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct mipsline{
    short haslabel;           //0 or 1
    short address;           //address (index * 4)
    char mips[80];          //original command stored in cstring
    char labelname[20];    //label (if any)
    short command;        //refers to instruction type
};

struct symboltable{
    short address;
    char labelname[20];
};

void read_data(struct mipsline* m, struct symboltable* s, unsigned short amount);
void str_copy(char* src, char* dst, unsigned short amount);

void null_line(struct mipsline* m)
{
    m->haslabel = 0;
    m->address = -1;
    m->mips[0] = (char) 0;
    m->labelname[0] = (char) 0;
    m->command = 0;
}

short regnum(char* c)       //returns numeric value of register in $(letter)(number) format
{
    if (c[0] != '$') return -1;             //return error if not pointed to a register
    if (c[1] == '0') return 0;              
    if (c[1] == 't') return ( (short) c[2] ) - 40;     //calculate return value based on ascii value of number - offset
    if (c[1] == 's') return ( (short) c[2] ) - 32;      // - 40 is equivalent to numerical value + 8
}                                                       // - 32 is equivalent to numerical value + 16

//boolean (0/1)
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

    for (i = 0; i < 100; i++)
    {
        if (data[i].mips[0] != (char) 0)
        {
            printf("0x%08x : %s\n", data[i].address, data[i].mips);
        }
    }

    return 0;
}

void str_copy(char* src, char* dst, unsigned short amount)
{
    unsigned short i;
    for (i = 0; i < amount; i++)
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