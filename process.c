#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char * argv[])
{
    initClk();
    int remainingTime = atoi(argv[1]);
    int schdPid = atoi(argv[2]);
    int schdClk = atoi(argv[3]);
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    while (remainingtime > 0)
    {
        printf("\nProcess with pid %d started remaining time is %d", getpid(), remainingTime);
    }
    
    destroyClk(false);
    
    return 0;
}
