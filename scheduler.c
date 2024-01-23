#include "headers.h"


int main(int argc, char * argv[])
{
    initClk();

    //while loop to maks sure its synced with the proc gen. 
    fflush(stdout);
    while (1){
        //sleep(1);
        int x = getClk();
        printf("current scheduler time is %d\n", x);
    }
    //TODO implement the scheduler :)
    //upon termination release the clock resources
    
    destroyClk(true);
}
