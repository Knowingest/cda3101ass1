#include <stdlib.h>
#include <stdio.h>
#include <string.h>



struct mipsline{
    short haslabel;
    short address;
    char mips[80];      //original command stored in cstring
    char labelname[20];    //label (if any)
    short command;        //refers to instruction type
};

void null_line(struct mipsline* m)
{
    m->haslabel = 0;
    m->address = -1;
    m->mips[0] = '\n';
    m->labelname[0] = '\n';
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
        if (c[i] == '\n') return 0;
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
    struct mipsline data[100];

    short i;
    for (i = 0; i < 100; i++)
    {
        null_line(&data[i]);
    }

    for (i = 0; i < 100; i++)
    {
        scanf(" %[^\n]", data[i].mips);
        data[i].haslabel = findlabel(data[i].mips);
        if (data[i].haslabel == 1) copylabel(data[i].mips, data[i].labelname);
        data[i].address = i * 4;
    }

    for (i = 0; i < 100; i++)
    {
        if (data[i].address != -1)
        {
            printf("%s\n", data[i].mips);
            if (data[i].haslabel == 1) printf("%s\n", data[i].labelname);
        }
    }


    return 0;
}