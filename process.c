#include "headers.h"

/* Modify this file as needed*/
void siguser1();
int main(int agrc, char * argv[])
{
    signal(SIGUSR1, siguser1);
    initClk();
    int remainingTime = atoi(argv[1]);
    int schdPid = atoi(argv[2]);
    //int remainingTime = 2;
    int schdClk = atoi(argv[3]);
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    
    while (remainingTime > 0)
    {
        remainingTime--;
        if(remainingTime > 0){
            
        pause();
        }
        else{
            break;
        }
        //sleep(1);
        //printf("\n\n%d\n\n", remainingTime);
        //raise(SIGSTOP);
    }
    
    kill(schdPid, SIGUSR2);
    destroyClk(false);
    
    return 0;
}

void siguser1(int sig)
{

}
