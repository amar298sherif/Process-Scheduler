#include "headers.h"

#define quantum 2

int initSchQueue();

process getProcess(int qid);

void runRoundRobin(struct PCBQueue* pcbQ, int timeQuantum);

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

    int currentTime = 0;    
    int completedProcesses = 0;
    
    //while loop to maks sure its synced with the proc gen. 
    fflush(stdout);

    while (completedProcesses < NPROC)
    {
        sleep(1);
        currentTime = getClk();

        // check for arriving processes
        process p = getProcess(sch_qid);
        if (p.arrivalTime != -1)
        {
           // PCB for the new process
           initializePCB(&pcbQ, p);
        }

        // Run Round Robin scheduling algorithm
        runRoundRobin(&pcbQ, quantum);

    }

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

void runRoundRobin(struct PCBQueue* pcbQ, int timeQuantum)
{
    static int currentProcess = 0;

    // Check if there's a process running
    if (pcbQ->front != NULL && pcbQ->front->data.id == currentProcess &&
        pcbQ->front->data.startTime == -1 && pcbQ->front->data.remainingTime > 0)
    {
        pcbQ->front->data.startTime = getClk();
    }

    // Run the process for the specified time quantum
    if (pcbQ->front != NULL)
    {
        int remainingTime = pcbQ->front->data.remainingTime;
        int runTime = (remainingTime > timeQuantum) ? timeQuantum : remainingTime;
        pcbQ->front->data.remainingTime -= runTime;

        printf("\nArrival time %d run time%d remaining time %d wait %d",
            pcbQ->front->data.arrivalTime,  pcbQ->front->data.runTime,  pcbQ->front->data.remainingTime,  pcbQ->front->data.waitingTime);        
        // Update waiting time for other processes
        struct PCBQNode* temp = pcbQ->front->next;
        while (temp != NULL)
        {
            temp->data.waitingTime += runTime;
            temp = temp->next;
        }

        // Check if the process has completed its run
        if (pcbQ->front->data.remainingTime == 0)
        {
            pcbQ->front->data.turnaroundTime = getClk() - pcbQ->front->data.arrivalTime;
            dequeuePCBQ(pcbQ);
        }
        else
        {
            // Move the process to the end of the queue
            enqueuePCBQ(pcbQ, dequeuePCBQ(pcbQ));
        }
    }

    currentProcess = (currentProcess + 1) % NPROC;
}
