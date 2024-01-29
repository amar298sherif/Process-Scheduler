#include "headers.h"
#include <errno.h>
// #define quantum 2

int initSchQueue();

process getProcess(int qid);

void runRoundRobin();

void runSRTN();

void runSJF();

void initializePCB(process p, int pid);

FILE *writeToLog(char t[]);

void logTS();

struct readyQueue *readyQ;

struct PCB pcbArray[NPROC];

// struct PCB pcbDoneArray[NPROC+1];

int runningProcess;

int quantum_steps;

int quantum = 2;

int completed = 0;

int idleTime = 0;

Node *pq = NULL;

int lastArrived;

void updateWaitingTime();

void logPerf();

void sigusr2_handler(int signum)
{
    completed++;
    int t = getClk();
    printf("Received SIGUSR2 signal. Process %d finished\n", runningProcess);
    char data[80];
    int TA = pcbArray[runningProcess - 1].waitingTime + pcbArray[runningProcess - 1].runTime;
    double WTA = TA / completed;
    pcbArray[runningProcess - 1].turnaroundTime = TA;
    pcbArray[runningProcess - 1].weightedturnaroundTime = WTA;

    sprintf(data, "At time %d process %d finsihed arr %d remain %d wait %d TA %d WTA %.3f\n", t, runningProcess, pcbArray[runningProcess - 1].arrivalTime, pcbArray[runningProcess - 1].remainingTime, pcbArray[runningProcess - 1].waitingTime, pcbArray[runningProcess - 1].turnaroundTime, pcbArray[runningProcess - 1].weightedturnaroundTime);
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
void sigIntHandler(int signum)
{
    FILE *f = writeToLog("\n");
    fclose(f);
    logPerf();
    signal(SIGINT, SIG_DFL);
    kill(getpid(), SIGINT);
}
int main(int argc, char *argv[])
{
    algorithm alg = SJF;
    if (argc > 1)
    {
        if (argv[1][1] == 'r')
        {
            quantum = atoi(argv[2]);
            alg = RR;
        }
        else if (argv[1][1] == 'r')
        {
            alg = SRTF;
        }
    }

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
        updateWaitingTime();
        switch (alg)
        {
        case RR:
            runRoundRobin();
            break;
        case SJF:
            runSJF();
            break;
        case SRTF:
            runSRTN();
            break;
        default:
            raise(SIGINT);
            break;
        }

        if (completed == NPROC)
            raise(SIGINT);
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
    pcb.waitingTime = -1;
    pcb.turnaroundTime = 0;

    if (lastArrived < p.id)
        lastArrived = p.id;

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
    if (runningProcess == 0) // if no process running
    {
        if (!isEmpty(readyQ))
        {
            runningProcess = dequeue(readyQ);
            if (pcbArray[runningProcess - 1].startTime == -1) // process running for the first time
            {
                pcbArray[runningProcess - 1].startTime = time;
                // kill(pcbArray[runningProcess - 1].pid, SIGCONT);
                // pcbArray[runningProcess - 1].remainingTime--;
                char data[80];
                sprintf(data, "At time %d process %d started arr %d remain %d wait %d\n", time, runningProcess, pcbArray[runningProcess - 1].arrivalTime, pcbArray[runningProcess - 1].remainingTime, pcbArray[runningProcess - 1].waitingTime);
                writeToLog(data);
                quantum_steps = quantum - 1;
            }
            else
            {
                quantum_steps = quantum - 1;
                kill(pcbArray[runningProcess - 1].pid, SIGCONT);
                pcbArray[runningProcess - 1].remainingTime--;
                char data[80];
                sprintf(data, "At time %d process %d resumed arr %d remain %d wait %d\n", time, runningProcess, pcbArray[runningProcess - 1].arrivalTime, pcbArray[runningProcess - 1].remainingTime, pcbArray[runningProcess - 1].waitingTime);
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
            // char data[80];
            // sprintf(data, "At time %d process %d started arr %d remain %d wait %d\n", time, runningProcess, pcbArray[runningProcess - 1].arrivalTime, pcbArray[runningProcess - 1].remainingTime, pcbArray[runningProcess - 1].waitingTime);
            // writeToLog(data);
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
                sprintf(data, "At time %d process %d stopped arr %d remain %d wait %d\n", time, temp, pcbArray[temp - 1].arrivalTime, pcbArray[temp - 1].remainingTime, pcbArray[temp - 1].waitingTime);
                writeToLog(data);
                if (pcbArray[runningProcess - 1].startTime == -1) // process running for the first time
                {
                    pcbArray[runningProcess - 1].startTime = time;
                    sprintf(data, "At time %d process %d started arr %d remain %d wait %d\n", time, runningProcess, pcbArray[runningProcess - 1].arrivalTime, pcbArray[runningProcess - 1].remainingTime, pcbArray[runningProcess - 1].waitingTime);
                    writeToLog(data);
                }
                else
                {
                    sprintf(data, "At time %d process %d resumed arr %d remain %d wait %d\n", time, runningProcess, pcbArray[runningProcess - 1].arrivalTime, pcbArray[runningProcess - 1].remainingTime, pcbArray[runningProcess - 1].waitingTime);
                    writeToLog(data);
                }
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
    char data[80];
    int time = getClk();
    if (runningProcess == 0) // if no process running
    {
        if (!isEmptyPrio(&pq))
        {
            int prio;
            runningProcess = peek(&pq, &prio);
            pop(&pq);
            if (pcbArray[runningProcess - 1].startTime == -1) // process running for the first time
            {
                sprintf(data, "At time %d process %d started arr %d remain %d wait %d\n", time, tempid, pcbArray[tempid - 1].arrivalTime, pcbArray[tempid - 1].remainingTime, pcbArray[tempid - 1].waitingTime);
                writeToLog(data);
                strcpy(data,'\0');
                pcbArray[runningProcess - 1].startTime = getClk();
            }
            else{
                sprintf(data, "At time %d process %d resumed arr %d remain %d wait %d\n", time, tempid, pcbArray[tempid - 1].arrivalTime, pcbArray[tempid - 1].remainingTime, pcbArray[tempid - 1].waitingTime);
                writeToLog(data);
                strcpy(data,'\0');
            }
            kill(pcbArray[runningProcess - 1].pid, SIGCONT);
            pcbArray[runningProcess - 1].remainingTime--;
            sprintf(data, "At time %d process %d started arr %d remain %d wait %d\n", time, runningProcess, pcbArray[runningProcess - 1].arrivalTime, pcbArray[runningProcess - 1].remainingTime, pcbArray[runningProcess - 1].waitingTime);
            writeToLog(data);
            strcpy(data,'\0');
        }
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
                sprintf(data, "At time %d process %d stopped arr %d remain %d wait %d\n", time, tempid, pcbArray[tempid - 1].arrivalTime, pcbArray[tempid - 1].remainingTime, pcbArray[tempid - 1].waitingTime);
                writeToLog(data);
                strcpy(data,'\0');
                if (pcbArray[runningProcess - 1].startTime == -1) // process running for the first time
                {
                    sprintf(data, "At time %d process %d started arr %d remain %d wait %d\n", time, runningProcess, pcbArray[runningProcess - 1].arrivalTime, pcbArray[runningProcess - 1].remainingTime, pcbArray[runningProcess - 1].waitingTime);
                    writeToLog(data);
                    strcpy(data,'\0');
                    pcbArray[runningProcess - 1].startTime = getClk();
                }
                else{
                    sprintf(data, "At time %d process %d resumed arr %d remain %d wait %d\n", time, runningProcess, pcbArray[runningProcess - 1].arrivalTime, pcbArray[runningProcess - 1].remainingTime, pcbArray[runningProcess - 1].waitingTime);
                    writeToLog(data);
                    strcpy(data,'\0');
                }
            }
        }
        if (runningProcess != 0)
        {
            kill(pcbArray[runningProcess - 1].pid, SIGCONT);
            pcbArray[runningProcess - 1].remainingTime--;
        }
    }
}

void runSJF()
{
    char data[80];
    int time = getClk();
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
                sprintf(data, "At time %d process %d started arr %d remain %d wait %d\n", time, runningProcess, pcbArray[runningProcess - 1].arrivalTime, pcbArray[runningProcess - 1].remainingTime, pcbArray[runningProcess - 1].waitingTime);
                writeToLog(data);
                strcpy(data,'\0');
            }
            else{
                sprintf(data, "At time %d process %d resumed arr %d remain %d wait %d\n", time, runningProcess, pcbArray[runningProcess - 1].arrivalTime, pcbArray[runningProcess - 1].remainingTime, pcbArray[runningProcess - 1].waitingTime);
                writeToLog(data);
                strcpy(data,'\0');
            }
            kill(pcbArray[runningProcess - 1].pid, SIGCONT);
            pcbArray[runningProcess - 1].remainingTime--;
        }
    }
    else // there is a running process
    {
        kill(pcbArray[runningProcess - 1].pid, SIGCONT);
        pcbArray[runningProcess - 1].remainingTime--;
    }
}

FILE *writeToLog(char text[])
{
    static FILE *fptr = NULL;
    if (fptr == NULL)
    {
        fptr = fopen("output.log", "w");
        fprintf(fptr, "%s \n", text);
    }
    else
    {
        fprintf(fptr, "%s \n", text);
    }
    return fptr;
}

void updateWaitingTime()
{
    if (runningProcess == 0)
        idleTime++;
    for (size_t i = 0; i < lastArrived; i++)
    {
        if (i != runningProcess - 1 && pcbArray[i].remainingTime > 0)
        {
            pcbArray[i].waitingTime++;
        }
    }
}

void logPerf()
{
    int t = getClk();
    float util = ((t - idleTime) / t) * 100;
    float awta = 0;
    float aw = 0;
    float step1 = 0;
    float stdwta;
    for (size_t i = 0; i < completed; i++)
    {
        aw = aw + pcbArray[i].waitingTime;
        awta = awta + pcbArray[i].weightedturnaroundTime;
    }
    aw = aw/completed;
    awta = awta/completed;
    for (size_t i = 0; i < completed; i++)
    {
        step1 = step1 + (pcbArray[i].waitingTime - aw) * (pcbArray[i].waitingTime - aw);
    }
    stdwta = sqrt(((float)(step1)) / (completed - 1));

    FILE *fptr = NULL;
    fptr = fopen("output.perf", "w");
    fprintf(fptr, "CPU utilization = %.3f  \n", util);
    fprintf(fptr, "Avg WTA = %.3f \n", awta);
    fprintf(fptr, "Avg Waiting = %.3f \n", aw);
    fprintf(fptr, "Std WTA = %.3f \n", stdwta);
    fclose(fptr);
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