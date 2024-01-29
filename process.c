#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

void contHandler(int signum){

}
int main(int agrc, char * argv[])
{
    signal(SIGCONT, contHandler);
    initClk();
    remainingtime = atoi(argv[1]);
    //remainingtime = 4;
    int schdPid = atoi(argv[2]);
    int schdClk = atoi(argv[3]);
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    kill(schdPid, SIGUSR1);
    
    while (remainingtime > 0)
    {
        remainingtime--;
       if (remainingtime == 0)
           break;
//
        raise(SIGSTOP);
    }
    
    kill(schdPid, SIGUSR2);
    destroyClk(false);
    
    return 0;
}
