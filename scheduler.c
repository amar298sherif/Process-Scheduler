#include "headers.h"

#define quantum 2

int initSchQueue();

process getProcess(int qid);

void runRoundRobin();

void initializePCB(process p, int pid);

struct readyQueue* readyQ;

struct PCB pcbArray[NPROC+1];

//struct PCB pcbDoneArray[NPROC+1];

int runningProcess;

int quantum_steps;

void sigusr2_handler(int signum) {
    printf("Received SIGUSR2 signal. Process %d finished\n", runningProcess);
    //pcbDoneArray[runningProcess] = pcbArray[runningProcess];
    quantum_steps = 0;
    runningProcess = 0;
    runRoundRobin();
}
void sigusr1_handler(int signum) {
    printf("Received SIGUSR1 signal. A new process started\n");
}
int main(int argc, char * argv[])
{
    if (signal(SIGUSR2, sigusr2_handler) == SIG_ERR) {
        perror("Error setting up SIGUSR2 handler");
        return 1;
    }
    if (signal(SIGUSR1, sigusr1_handler) == SIG_ERR) {
        perror("Error setting up SIGUSR2 handler");
        return 1;
    }
    int sch_qid;
    sch_qid = initSchQueue();
    initClk();
    
    readyQ = createQueue(NPROC+1);
    
    runningProcess = 0;

    quantum_steps = 0;

    int currentTime = 0;    
    //while loop to maks sure its synced with the proc gen. 
    fflush(stdout);

    while (1)
    {
        sleep(1);
        currentTime = getClk();
        printf("%d", runningProcess);
        // check for arriving processes
        process p = getProcess(sch_qid);
        if (p.arrivalTime != -1)
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
                sprintf(sExecutionTime, "%d", p.runTime+1);
                sprintf(sClk, "%d", getClk());
                char *const paramList[] = {"./process.out", sExecutionTime, sPid, sClk,NULL};
                execv("./process.out", paramList);
                
                //if it executes what is under this line, then execv has failed
                perror("Error in execv'ing to clock");
                exit(EXIT_FAILURE);
            }

            initializePCB(p, prcsPid);

            // put in pcb and enqueue
        }
        runRoundRobin();
    }
    //freeQueue(readyQ);
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

    //pcbArray[p.id-1] = pcb;
    pcbArray[p.id] = pcb;

    enqueue(readyQ, p.id);
}
void runRoundRobin()
{
    if(runningProcess == 0) //if no process running
    {
        if(!isEmpty(readyQ)){
            runningProcess = dequeue(readyQ);
            if(pcbArray[runningProcess].startTime==-1) // process running for the first time
            {
                pcbArray[runningProcess].startTime = getClk();
                kill(pcbArray[runningProcess].pid, SIGCONT);
                quantum_steps = 1;
            }
            else
            {
                quantum_steps = 1;
                kill(pcbArray[runningProcess].pid, SIGCONT);
            }
        }
    }
    else //there is a running process
    {
        if(quantum_steps < quantum){
            kill(pcbArray[runningProcess].pid, SIGCONT);
            quantum_steps++;
        }
        else if(quantum_steps == quantum){
            quantum_steps = 0;
            int temp = runningProcess;
            //dequeue
            runningProcess = dequeue(readyQ);
            //enqueue
            enqueue(readyQ, temp);
            //run
            kill(pcbArray[runningProcess].pid, SIGCONT);
        }
    }
}