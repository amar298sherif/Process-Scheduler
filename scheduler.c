#include "headers.h"

#define quantum 2

int initSchQueue();

process getProcess(int qid);

void runRoundRobin();

void initializePCB( process p, int pid);

void logState(int time, int id, const char *state, PCB *pcb);

void calculatePerformance(PCB *pcbArray);

PCBQNode* currentProcess;

readyQueue readyQ;

int main(int argc, char * argv[])
{
    int sch_qid;
    sch_qid = initSchQueue();
    initClk();
    
    //algo = atoi(argv[0]);           // algorithm 
    /*if(argc == 1){
        quantum = atoi(argv[1]);        //quantum in RR 
    }*/
    
    
    //struct readyQueue readyQ;
    initReadyQ(&readyQ);

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

           int pid = forkPrcs();

           initializePCB(p, pid);
        }

        // Run Round Robin scheduling algorithm
        runRoundRobin(&readyQ, quantum);

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

void alarmHandler(int signum) //used with RR to wakeup after (time = quantum) has passed
{
     //CHECK so that recieved signals at this time step are placed before the one has just stopped
    
    currentProcess->remainingTime -= quantum; //decrement remaining time by quantum
    
    if(currentProcess->remainingTime > 0)
        if(getQueueSize(&readyQueue) > 0)   //do not stop if it is the only one in Q  
            stopPrcs();//
    // run again
    runRoundRobin();
}

void stopPrcs()
{
    enqueueReadyQ(currentProcess);
    currentProcess = NULL;
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

void initializePCB(process p, int pid) {
    PCB pcb;
    pcb.id = p.id;
    pcb.pid = pid;
    pcb.arrivalTime = p.arrivalTime;
    pcb.runTime = p.runTime;
    pcb.priority = p.priority;
    pcb.startTime = -1;
    pcb.endTime = -1;
    pcb.remainingTime = p.runTime;
    pcb.waitingTime = 0;
    pcb.turnaroundTime = 0;

    enqueuereadyQ(readyQ, pcb);
}

int forkPrcs(int executionTime)
{
    pid_t schdPid = getpid();
    pid_t prcsPid = fork();

    if(prcsPid == -1) //error happened when forking
    {
        perror("Error forking process");
        exit(EXIT_FAILURE);
    }
    else if(prcsPid == 0) //execv'ing to process
    {
        char sExecutionTime[5] = {0};
        char sPid[7] = {0};
        char sClk[7] = {0};
        sprintf(sPid, "%d", schdPid);
        sprintf(sExecutionTime, "%d", executionTime);
        sprintf(sClk, "%d", getClk());
        char *const paramList[] = {"./process.out", sExecutionTime, sPid, sClk,NULL};
        execv("./process.out", paramList);
        
        //if it executes what is under this line, then execv has failed
        perror("Error in execv'ing to clock");
        exit(EXIT_FAILURE);
    }
    return prcsPid;
}


void runRoundRobin()
{
    // Check if the queue is not empty and there's a process running
    if ( currentProcess == NULL)
    {
        // Get the front process
        currentProcess = dequeueReadyQ(&readyQ)

        // Check if the process is just starting
        if (currentProcess->startTime == -1 && currentProcess->remainingTime > 0)
        {
            currentProcess->startTime = getClk();
            currentProcess->pid = forkPrcs(currentProcess->runTime);
            // write started
        }
        else //process has run before
        {
            kill(currentProcess->pid, SIGCONT);
            // write resumed
        }
    }
    alarm(quantum);
}
