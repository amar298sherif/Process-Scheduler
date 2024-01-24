#include "headers.h"

#define quantum 2

int initSchQueue();

process getProcess(int qid);

void runRoundRobin(PCB *pcb, int timeQuantum);

void initializePCB(PCB pcbArray[], process p);

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
    
    
    // array of PCBs, NPROC = max number of processes 
    PCB pcbArray[NPROC];
    // initializing PCB with zeros 
    memset(pcbArray, 0, sizeof(pcbArray));

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
           initializePCB(pcbArray, p);
        }

        // Run Round Robin scheduling algorithm
        runRoundRobin(pcbArray, quantum);

        // Check for completed processes
        for (int i = 0; i < NPROC; i++)
        {
            if (pcbArray[i].endTime == -1 && pcbArray[i].remainingTime == 0)
            {
                pcbArray[i].endTime = currentTime;
                completedProcesses++;
            }
        }
        calculatePerformance(pcbArray);

    }
    /*while (1){
        sleep(1);
        int x = getClk();
        process p = getProcess(sch_qid);
        if (p.arrivalTime != -1){   
            printf("\ncurrent scheduler time is %d\n", x);
        }
    }*/
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

void initializePCB(PCB pcbArray[], process p) {
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

    pcbArray[p.id] = pcb;
}

void runRoundRobin(PCB *pcbArray, int timeQuantum)
{
    static int currentProcess = 0;

    // Check if there's a process running
    if (pcbArray[currentProcess].startTime == -1 && pcbArray[currentProcess].remainingTime > 0)
    {
        pcbArray[currentProcess].startTime = getClk();
    }

    // Run the process for the specified time quantum
    int remainingTime = pcbArray[currentProcess].remainingTime;
    int runTime = (remainingTime > timeQuantum) ? timeQuantum : remainingTime;
    pcbArray[currentProcess].remainingTime -= runTime;

    // Update waiting time for other processes
    for (int i = 0; i < NPROC; i++)
    {
        if (i != currentProcess && pcbArray[i].startTime != -1)
        {
            pcbArray[i].waitingTime += runTime;
        }
    }

    // Check if the process has completed its run
    if (pcbArray[currentProcess].remainingTime == 0)
    {
        pcbArray[currentProcess].turnaroundTime = getClk() - pcbArray[currentProcess].arrivalTime;
    }

    currentProcess = (currentProcess + 1) % NPROC;
}

void logState(int time, int id, const char *state, PCB *pcb)
{
    printf("\nAt time %d process %d state %s arr %d total %d remain %d wait %d",
           time, id, state, pcb->arrivalTime, pcb->runTime, pcb->remainingTime, pcb->waitingTime);
}

void calculatePerformance(PCB *pcbArray)
{
    int totalTurnaroundTime = 0;
    int totalWaitingTime = 0;
    double totalWeightedTurnaroundTime = 0.0;

    for (int i = 0; i < NPROC; i++)
    {
        totalTurnaroundTime += pcbArray[i].turnaroundTime;
        totalWaitingTime += pcbArray[i].waitingTime;
        totalWeightedTurnaroundTime += (double)pcbArray[i].turnaroundTime / pcbArray[i].runTime;
    }

    double avgWeightedTurnaroundTime = totalWeightedTurnaroundTime / NPROC;
    double avgWaitingTime = (double)totalWaitingTime / NPROC;

    double stdWeightedTurnaroundTime = 0.0;
    for (int i = 0; i < NPROC; i++)
    {
        stdWeightedTurnaroundTime += pow(((double)pcbArray[i].turnaroundTime / pcbArray[i].runTime) - avgWeightedTurnaroundTime, 2);
    }
    stdWeightedTurnaroundTime = sqrt(stdWeightedTurnaroundTime / NPROC);

    double cpuUtilization = (double)totalTurnaroundTime / getClk() * 100;

    printf("\n\nCPU utilization = %.2f%%", cpuUtilization);
    printf("\nAvg WTA = %.2f", avgWeightedTurnaroundTime);
    printf("\nAvg Waiting = %.2f", avgWaitingTime);
    printf("\nStd WTA = %.2f\n", stdWeightedTurnaroundTime);
}