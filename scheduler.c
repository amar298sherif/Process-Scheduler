#include "headers.h"

int initSchQueue();
process getProcess(int qid);

int main(int argc, char * argv[])
{
    int sch_qid;
    sch_qid = initSchQueue();
    initClk();

    // array of PCBs, NPROC = max number of processes 
    PCB pcbArray[NPROC];
    // initializing PCB with zeros 
    memset(pcbArray, 0, sizeof(pcbArray));

    int currentTime = 0;
    int completedProcesses = 0;
    
    //while loop to maks sure its synced with the proc gen. 
    fflush(stdout);
    while (1){
        sleep(1);
        int x = getClk();
        process p = getProcess(sch_qid);
        if (p.arrivalTime != -1){   
            printf("\ncurrent scheduler time is %d\n", x);
        }
    }
    //TODO implement the scheduler :)
    //upon termination release the clock resources
    
    destroyClk(true);
}

int initSchQueue()
{
    int msgqid = msgget(GEN_TO_SCH_KEY, IPC_CREAT | 0644);
    if (msgqid == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    return msgqid;
}

process getProcess(int qid)
{
    msgbuff msg;
    msgrcv(qid, &msg, sizeof(msg.mprocess), 0, !IPC_NOWAIT);
    if (msg.mprocess.arrivalTime != -1)
    { //add it to the array here (?)
        printf("\n%p \t %d \t %d \t %d \t %d", &msg, msg.mprocess.id, msg.mprocess.arrivalTime, msg.mprocess.runTime, msg.mprocess.priority);
    }
    
    return msg.mprocess;
}