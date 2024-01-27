#include "headers.h"
#include <errno.h>
#define quantum 2
algorithm algo;
int initSchQueue();

process getProcess(int qid);

void runRoundRobin();
void runSRTF();

void initializePCB(process p, int pid);

struct readyQueue* readyQ;
struct readyQueue* PriorityQueue;
struct PCB pcbArray[NPROC];

//struct PCB pcbDoneArray[NPROC+1];

int runningProcess;

int quantum_steps;

void sigusr2_handler(int signum) {
    printf("Received SIGUSR2 signal. Process %d finished\n", runningProcess);
    //pcbDoneArray[runningProcess-1] = pcbArray[runningProcess-1];
    quantum_steps = 0;
    runningProcess = 0;
    //runRoundRobin();
}
void sigusr1_handler(int signum) {
    //printf("Received SIGUSR1 signal. A new process started\n");
}
int main(int argc, char * argv[])
{
    if (signal(SIGUSR2, sigusr2_handler) == SIG_ERR) {
        perror("Error setting up SIGUSR2 handler");
        return 1;
    }
    if (signal(SIGUSR1, sigusr1_handler) == SIG_ERR) {
        perror("Error setting up SIGUSR1 handler");
        return 1;
    }
    int sch_qid;
    sch_qid = initSchQueue();
    initClk();
    
    readyQ = createQueue(NPROC+1);
    if(algo == 2) PriorityQueue = createQueue(NPROC + 1);
    runningProcess = 0;

    quantum_steps = 0;

    int currentTime = 0;  
    int oldclk = 0;  
    //while loop to maks sure its synced with the proc gen. 
    fflush(stdout);

    while (1)
    {
        //sleep(1);
        currentTime = getClk();
        while(currentTime == oldclk)
        {
            currentTime = getClk();
        }
        oldclk = currentTime;
                //printf("current time is %d\n", oldclk);
        //printf("%d", runningProcess);
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
            //printf("\n at time %d", currentTime);
            initializePCB(p, prcsPid);

            // put in pcb and enqueue
        }
        //runRoundRobin();
        runSRTF();
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
    while (1) {
        int ret = msgrcv(qid, &msg, sizeof(msg.mprocess), 0, !IPC_NOWAIT);
        if (ret == -1) {
            if (errno == EINTR) {
                continue; // Interrupted by a signal, try again
            } else {
                perror("Error in msgrcv");
                exit(-2);
            }
        }
        break; // Message received, exit the loop
    }
    if (msg.mprocess.arrivalTime != -1)
    { 
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
    pcb.recentStart = 0;

    //printf("\n id: %d \t arrival: %d \t runtime: %d \t pri: %d\t", p.id, p.arrivalTime, p.runTime, p.priority);

    //pcbArray[p.id-1] = pcb;
    pcbArray[p.id - 1] = pcb;

    enqueue(readyQ, p.id);
    //printf("added and enqueued\n");
}
void runRoundRobin()
{
    if(runningProcess == 0) //if no process running
    {
        if(!isEmpty(readyQ)){
            runningProcess = dequeue(readyQ);
            if(pcbArray[runningProcess-1].startTime==-1) // process running for the first time
            {
                pcbArray[runningProcess-1].startTime = getClk();
                kill(pcbArray[runningProcess-1].pid, SIGCONT);
                quantum_steps = 1;
            }
            else
            {
                quantum_steps = 1;
                kill(pcbArray[runningProcess-1].pid, SIGCONT);
            }
        }
    }
    else //there is a running process
    {
        if(quantum_steps < quantum){
            kill(pcbArray[runningProcess-1].pid, SIGCONT);
            quantum_steps++;
        }
        else if(quantum_steps == quantum){
            quantum_steps = 0;
            int temp = runningProcess;
            //dequeue
            if(!isEmpty(readyQ))
                runningProcess = dequeue(readyQ);
            else
                runningProcess = 0;
            //enqueue
            enqueue(readyQ, temp);
            //run
            //printf("\n----running is :%d\n",runningProcess);
           if (runningProcess > 0)
                kill(pcbArray[runningProcess-1].pid, SIGCONT);
        }
    }
}

void heapify(struct readyQueue* queue, struct PCB pcbArray[], int i) {
    int least = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < queue->size && pcbArray[queue->array[left] - 1].remainingTime < pcbArray[queue->array[least] - 1].remainingTime)
        least = left;

    if (right < queue->size && pcbArray[queue->array[right] - 1].remainingTime < pcbArray[queue->array[least] - 1].remainingTime)
        least = right;

    if (least != i) {
        int temp = queue->array[i];
        queue->array[i] = queue->array[least];
        queue->array[least] = temp;
        heapify(queue, pcbArray, least);
    }
}



void StopProcess()
{
    enqueue(readyQ,runningProcess);
    runningProcess = 0;
    for (int i = readyQ->size/2; i > 0; i--)
        heapify(readyQ, pcbArray, i);
}

void runSRTF() {
    if (runningProcess != 0) {
        pcbArray[runningProcess - 1].remainingTime -= (getClk() - pcbArray[runningProcess - 1].recentStart);

        if (!isEmpty(readyQ) && pcbArray[readyQ->front - 1].remainingTime < pcbArray[runningProcess - 1].remainingTime) {
            StopProcess();
            runningProcess = dequeue(readyQ);
            heapify(readyQ, pcbArray, 0);
        } else {
            pcbArray[runningProcess - 1].recentStart = getClk();
            return;
        }
    } 
    else
    {
        if(!isEmpty(readyQ))
            runningProcess = dequeue(readyQ);
    }

    if(pcbArray[runningProcess-1].startTime==-1) // process running for the first time
    {
        pcbArray[runningProcess-1].startTime = getClk();
        kill(pcbArray[runningProcess-1].pid, SIGCONT);
    }

    else 
        kill(pcbArray[runningProcess-1].pid, SIGCONT);
    
    pcbArray[runningProcess - 1].recentStart = getClk();
}
