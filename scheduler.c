#include "headers.h"

#define quantum 2

int initSchQueue();

process getProcess(int qid);

void runRoundRobin(struct PCBQueue* pcbQ, int timeQuantum, struct PCBQueue* pcbQDone);

void initializePCB(struct PCBQueue* pcbQ, process p);

void logState(int time, int id, const char *state, PCB *pcb);

void calculatePerformance(PCB *pcbArray);

int main(int argc, char * argv[])
{
    int sch_qid;
    sch_qid = initSchQueue();
    initClk();
    
    //algo = atoi(argv[0]);           // algorithm 
    /*if(argc == 1){
        quantum = atoi(argv[1]);        //quantum in RR 
    }*/
    
    
    struct PCBQueue pcbQ;
    initPCBQueue(&pcbQ);

    struct PCBQueue pcbQDone;
    initPCBQueue(&pcbQDone);

    int currentTime = 0;    
    int receivedProcesses = 0;
    
    //while loop to maks sure its synced with the proc gen. 
    fflush(stdout);

    while (receivedProcesses < NPROC)
    {
        sleep(1);
        currentTime = getClk();

        // check for arriving processes
        process p = getProcess(sch_qid);
        if (p.arrivalTime != -1)
        {
           // PCB for the new process
           initializePCB(&pcbQ, p);
           receivedProcesses++;
        }

        // Run Round Robin scheduling algorithm
        runRoundRobin(&pcbQ, quantum, &pcbQDone);

    }
    while(getQueueSize(&pcbQDone)<NPROC)
    {runRoundRobin(&pcbQ, quantum, &pcbQDone);}
    //CAlulate
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
        //printf("\n%p \t %d \t %d \t %d \t %d", &msg, msg.mprocess.id, msg.mprocess.arrivalTime, msg.mprocess.runTime, msg.mprocess.priority);
    }
    
    return msg.mprocess;
}

void initializePCB(struct PCBQueue* pcbQ, process p) {
    PCB pcb;
    pcb.id = p.id;
    pcb.arrivalTime = p.arrivalTime;
    pcb.runTime = p.runTime;
    pcb.priority = p.priority;
    pcb.startTime = -1;
    pcb.endTime = -1;
    pcb.remainingTime = p.runTime;
    pcb.waitingTime = 0;
    pcb.turnaroundTime = 0;

    enqueuePCBQ(pcbQ, pcb);
}

void runRoundRobin(struct PCBQueue* pcbQ, int timeQuantum, struct PCBQueue* pcbQDone)
{
   
    if (pcbQ->front != NULL)
    {
        // Get the front process
        PCB* currentProcess = &pcbQ->front->data;

        // Check if the process is just starting
        if (currentProcess->startTime == -1 && currentProcess->remainingTime > 0)
        {
            currentProcess->startTime = getClk();
        }

        // Run the process for the specified time quantum
        int remainingTime = currentProcess->remainingTime;
        int runTime = (remainingTime > timeQuantum) ? timeQuantum : remainingTime;
        currentProcess->remainingTime -= runTime;

        // Print process information
        printf("\nProcess %d: Arrival Time %d, Run Time %d, Remaining Time %d, Waiting Time %d",
               currentProcess->id, currentProcess->arrivalTime, currentProcess->runTime,
               currentProcess->remainingTime, currentProcess->waitingTime);

        // Update waiting time for other processes in the queue
        struct PCBQNode* temp = pcbQ->front->next;
        while (temp != NULL)
        {
            temp->data.waitingTime += runTime;
            temp = temp->next;
        }

        // Check if the process has completed its run
        if (currentProcess->remainingTime == 0)
        {
            currentProcess->turnaroundTime = getClk() - currentProcess->arrivalTime;
            enqueuePCBQ(pcbQDone, dequeuePCBQ(pcbQ));
        }
        else
        {
            // Move the process to the end of the queue
            enqueuePCBQ(pcbQ, dequeuePCBQ(pcbQ));
        }
    }
}
