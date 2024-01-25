#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char * argv[])
{
    initClk();
    remainingtime = atoi(argv[1]);
    int schPID = atoi(argv[2]);
    int clk = atoi(argv[3]);
    //TODO it needs to get the remaining time from somewhere
    
    if(remainingtime>0){
        while (remainingtime--)
        {
            pause();
        }
    }
    kill(schPID, SIGUSR1);
    destroyClk(false);
    
    return 0;
}
