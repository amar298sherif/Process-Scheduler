#include "headers.h"
#include <errno.h>
#define quantum 2

int initSchQueue();

process getProcess(int qid);

void runRoundRobin();

void runSRTN();

void runSJF();

void initializePCB(process p, int pid);

FILE* writeToLog(char t[]);

void logTS();

struct readyQueue *readyQ;

struct PCB pcbArray[NPROC];

// struct PCB pcbDoneArray[NPROC+1];

int runningProcess;

int quantum_steps;

Node *pq = NULL;

int lastArrived;

void sigusr2_handler(int signum)
{
    printf("Received SIGUSR2 signal. Process %d finished\n", runningProcess);
    char data[80];
    int TA = -1;
    int WTA
    // sprintf(data, "At time %d process %d finsihed arr %d remain %d wait %d TA %d WTA %f\n", getClk(), runningProcess, pcbArray[runningProcess - 1].arrivalTime, pcbArray[runningProcess - 1].remainingTime, pcbArray[runningProcess - 1].waitingTime);
    writeToLog(data);
    // pcbDoneArray[runningProcess-1] = pcbArray[runningProcess-1];
    quantum_steps = 0;
    runningProcess = 0;
    // runRoundRobin();
}
void sigusr1_handler(int signum)
{
    // printf("Received SIGUSR1 signal. A new process started\n");
}
void sigIntHandler(int signum){
    FILE* f = writeToLog("\n");
    fclose(f);
        signal(SIGINT, SIG_DFL);
    kill(getpid(), SIGINT);
}
int main(int argc, char *argv[])
{
    if (signal(SIGUSR2, sigusr2_handler) == SIG_ERR)
    {
        perror("Error setting up SIGUSR2 handler");
        return 1;
    }
    if (signal(SIGUSR1, sigusr1_handler) == SIG_ERR)
    {
        perror("Error setting up SIGUSR1 handler");
        return 1;
    }
    if (signal(SIGINT, sigIntHandler) == SIG_ERR)
    {
        perror("Error setting up SIGUSR1 handler");
        return 1;
    }
    int sch_qid;
    sch_qid = initSchQueue();
    initClk();

    readyQ = createQueue(NPROC + 1);

    runningProcess = 0;

    quantum_steps = 0;

    lastArrived = 0;

    int currentTime = 0;

    int oldclk = 0;
    // while loop to maks sure its synced with the proc gen.
    fflush(stdout);

    while (1)
    {
        // sleep(1);
        currentTime = getClk();
        while (currentTime == oldclk)
        {
            currentTime = getClk();
        }
        oldclk = currentTime;
        // printf("current time is %d\n", oldclk);
        // printf("%d", runningProcess);
        //  check for arriving processes
        process p = getProcess(sch_qid);
        if (p.arrivalTime != -1)
        {
            pid_t schdPid = getpid();
            pid_t prcsPid = fork();

            if (prcsPid == -1) // error happened when forking
            {
                perror("Error forking process");
                exit(EXIT_FAILURE);
            }
            else if (prcsPid == 0) // execv'ing to process
            {
                char sExecutionTime[5] = {0};
                char sPid[7] = {0};
                char sClk[7] = {0};
                sprintf(sPid, "%d", schdPid);
                sprintf(sExecutionTime, "%d", p.runTime + 1);
                sprintf(sClk, "%d", getClk());
                char *const paramList[] = {"./process.out", sExecutionTime, sPid, sClk, NULL};
                execv("./process.out", paramList);

                // if it executes what is under this line, then execv has failed
                perror("Error in execv'ing to clock");
                exit(EXIT_FAILURE);
            }
            // printf("\n at time %d", currentTime);
            initializePCB(p, prcsPid);

            // put in pcb and enqueue
        }
        // runSJF();
        // runSRTN();
        runRoundRobin();
    }
    // freeQueue(readyQ);
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
    while (1)
    {
        int ret = msgrcv(qid, &msg, sizeof(msg.mprocess), 0, !IPC_NOWAIT);
        if (ret == -1)
        {
            if (errno == EINTR)
            {
                continue; // Interrupted by a signal, try again
            }
            else
            {
                perror("Error in msgrcv");
                exit(-2);
            }
        }
        break; // Message received, exit the loop
    }
    if (msg.mprocess.arrivalTime != -1)
    {
        // printf("\n%p \t %d \t %d \t %d \t %d", &msg, msg.mprocess.id, msg.mprocess.arrivalTime, msg.mprocess.runTime, msg.mprocess.priority);
    }

    return msg.mprocess;
}

void initializePCB(process p, int pid)
{
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

    // if (lastArrived < p.id)
    //     lastArrived = p.id;

    // printf("\n id: %d \t arrival: %d \t runtime: %d \t pri: %d\t", p.id, p.arrivalTime, p.runTime, p.priority);

    // pcbArray[p.id-1] = pcb;
    pcbArray[p.id - 1] = pcb;

    enqueue(readyQ, p.id);
    push(&pq, p.id, p.runTime);
    // printf("added and enqueued\n");
}
void runRoundRobin()
{
    int time = getClk();
    if(runningProcess == 0) //if no process running
    {
        if (!isEmpty(readyQ))
        {
            runningProcess = dequeue(readyQ);
            if (pcbArray[runningProcess - 1].startTime == -1) // process running for the first time
            {
                pcbArray[runningProcess - 1].startTime = getClk();
                //kill(pcbArray[runningProcess - 1].pid, SIGCONT);
                //pcbArray[runningProcess - 1].remainingTime--;
                char data[80];
                sprintf(data, "At time %d process %d started arr %d remain %d wait %d\n", getClk(), runningProcess, pcbArray[runningProcess - 1].arrivalTime, pcbArray[runningProcess - 1].remainingTime, pcbArray[runningProcess - 1].waitingTime);
                writeToLog(data);
                quantum_steps = quantum - 1;
            }
            else
            {
                quantum_steps = quantum - 1;
                kill(pcbArray[runningProcess - 1].pid, SIGCONT);
                pcbArray[runningProcess - 1].remainingTime--;
                char data[80];
                sprintf(data, "At time %d process %d resumed arr %d remain %d wait %d\n", getClk(), runningProcess, pcbArray[runningProcess - 1].arrivalTime, pcbArray[runningProcess - 1].remainingTime, pcbArray[runningProcess - 1].waitingTime);
                writeToLog(data);
            }
        }
    }
    else // there is a running process
    {
        if (quantum_steps < quantum)
        {
            kill(pcbArray[runningProcess - 1].pid, SIGCONT);
            pcbArray[runningProcess - 1].remainingTime--;
            //char data[80];
            //sprintf(data, "At time %d process %d started arr %d remain %d wait %d\n", getClk(), runningProcess, pcbArray[runningProcess - 1].arrivalTime, pcbArray[runningProcess - 1].remainingTime, pcbArray[runningProcess - 1].waitingTime);
            //writeToLog(data);
            quantum_steps++;
        }
        else if (quantum_steps == quantum)
        {
            quantum_steps = 0;
            int temp = runningProcess;
            // dequeue
            if (!isEmpty(readyQ))
                runningProcess = dequeue(readyQ);
            else
                runningProcess = 0;
            // enqueue
            enqueue(readyQ, temp);
            // run
            // printf("\n----running is :%d\n",runningProcess);
            if (runningProcess > 0)
            {
                kill(pcbArray[runningProcess - 1].pid, SIGCONT);
                pcbArray[runningProcess - 1].remainingTime--;
                char data[80];
                sprintf(data, "At time %d process %d stopped arr %d remain %d wait %d\n", getClk(), temp, pcbArray[temp - 1].arrivalTime, pcbArray[temp - 1].remainingTime, pcbArray[temp - 1].waitingTime);
                writeToLog(data);
                sprintf(data, "At time %d process %d resumed arr %d remain %d wait %d\n", getClk(), runningProcess, pcbArray[runningProcess - 1].arrivalTime, pcbArray[runningProcess - 1].remainingTime, pcbArray[runningProcess - 1].waitingTime);
                writeToLog(data);
            }
        }
    }
    if (runningProcess != 0)
    {
        // printf("Process %d is running \n", runningProcess);
        // printf("Remaining Time is %d \n", pcbArray[runningProcess - 1].remainingTime);
    }
}

void runSRTN()
{
    int time= getClk();
    if (runningProcess == 0) // if no process running
    {
        if (!isEmptyPrio(&pq))
        {
            int prio;
            runningProcess = peek(&pq, &prio);
            pop(&pq);
            if (pcbArray[runningProcess - 1].startTime == -1) // process running for the first time
            {
                pcbArray[runningProcess - 1].startTime = getClk();
            }
            kill(pcbArray[runningProcess-1].pid, SIGCONT);
            pcbArray[runningProcess-1].remainingTime--;
            printf("At time %d process %d started arr %d total %d remain %d wait \n",time,runningProcess,pcbArray[runningProcess-1].arrivalTime,pcbArray[runningProcess-1].runTime,pcbArray[runningProcess-1].remainingTime);        }
    }
    else // there is a running process
    {
        if (!isEmptyPrio(&pq))
        {
            int prio;
            int id = peek(&pq, &prio);
            int tempid = runningProcess;
            int tempPrio = pcbArray[runningProcess - 1].remainingTime;
            if (pcbArray[runningProcess - 1].remainingTime > prio)
            {
                runningProcess = id;
                pop(&pq);
                push(&pq, tempid, tempPrio);
                if (pcbArray[runningProcess - 1].startTime == -1) // process running for the first time
                {
                    pcbArray[runningProcess - 1].startTime = getClk();
                }
                printf("At time %d process %d stopped arr %d total %d remain %d wait \n",time,tempid,pcbArray[tempid-1].arrivalTime,pcbArray[tempid-1].runTime,pcbArray[tempid-1].remainingTime);
            }
        }
        if(runningProcess!=0){
            kill(pcbArray[runningProcess-1].pid, SIGCONT);
            pcbArray[runningProcess-1].remainingTime--;
            printf("At time %d process %d started arr %d total %d remain %d wait \n",time,runningProcess,pcbArray[runningProcess-1].arrivalTime,pcbArray[runningProcess-1].runTime,pcbArray[runningProcess-1].remainingTime);
        }
    }
}


void runSJF(){
    int time = getClk();
    if(runningProcess == 0) //if no process running
    {
        if (!isEmptyPrio(&pq))
        {
            int prio;
            runningProcess = peek(&pq, &prio);
            pop(&pq);
            if (pcbArray[runningProcess - 1].startTime == -1) // process running for the first time
            {
                pcbArray[runningProcess - 1].startTime = getClk();
            }
            kill(pcbArray[runningProcess-1].pid, SIGCONT);
            pcbArray[runningProcess-1].remainingTime--;
            printf("At time %d process %d started arr %d total %d remain %d wait \n",time,runningProcess,pcbArray[runningProcess-1].arrivalTime,pcbArray[runningProcess-1].runTime,pcbArray[runningProcess-1].remainingTime);
        }
    }
    else // there is a running process
    {
            kill(pcbArray[runningProcess-1].pid, SIGCONT);
            pcbArray[runningProcess-1].remainingTime--;
        printf("At time %d process %d started arr %d total %d remain %d wait \n",time,runningProcess,pcbArray[runningProcess-1].arrivalTime,pcbArray[runningProcess-1].runTime,pcbArray[runningProcess-1].remainingTime);
    }
}

FILE* writeToLog(char text[])
{
    static FILE *fptr = NULL;
    if (fptr == NULL)
    {
        fptr = fopen("Lognew.txt", "w");
        fprintf(fptr, "%s \n", text);
    }
    else
    {
        fprintf(fptr, "%s \n", text);
    }
    return fptr;
}

// void logTS()
// {
//     for (size_t i = 0; i < lastArrived; i++)
//     {
//         if (i == runningProcess - 1)
//         {
//             writeToLog()
//         }
//     }
// }