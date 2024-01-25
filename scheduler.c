#include "headers.h"

#define quantum 2

int initSchQueue();

process getProcess(int qid);

void runRoundRobin(struct PCBQueue* pcbQ, int timeQuantum, struct PCBQueue* pcbQDone);

void initializePCB(struct PCBQueue* pcbQ, process p);



int main(int argc, char * argv[])
{
    int sch_qid;
    sch_qid = initSchQueue();
    initClk();
    


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
            receivedProcesses++;
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
                sprintf(sExecutionTime, "%d", 5);
                sprintf(sClk, "%d", getClk());
                char *const paramList[] = {"./process.out", sExecutionTime, sPid, sClk,NULL};
                execv("./process.out", paramList);
                
                //if it executes what is under this line, then execv has failed
                perror("Error in execv'ing to clock");
                exit(EXIT_FAILURE);
            }
        }
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
#include <stdio.h>
#include <stdlib.h>

typedef struct PCB {
    // PCB structure definition
    int id;
    int pid;
    int arrivalTime;
    int priority; // For HPF
    int executionTime;
    int remainingTime;
    int startTime;
    int finishTime;
    int waitingTime;
    int TA;
    float WTA;
    int recentStart; // For SRTN
} PCB;

typedef struct Node {
    PCB *data;
    struct Node *next;
} Node;

typedef struct Queue {
    Node *front;
    Node *rear;
    int size;
} Queue;

Queue *initializeQueue() {
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    if (queue == NULL) {
        perror("Error initializing queue");
        exit(EXIT_FAILURE);
    }
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
    return queue;
}

void enqueue(Queue *queue, PCB *data) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        perror("Error enqueueing data");
        exit(EXIT_FAILURE);
    }
    newNode->data = data;
    newNode->next = NULL;

    if (queue->rear == NULL) {
        queue->front = newNode;
        queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }

    queue->size++;
}

PCB *dequeue(Queue *queue) {
    if (queue->front == NULL) {
        return NULL;
    }

    Node *tempNode = queue->front;
    PCB *tempData = tempNode->data;

    queue->front = queue->front->next;
    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    free(tempNode);
    queue->size--;

    return tempData;
}

PCB *peek(Queue *queue) {
    if (queue->front == NULL) {
        return NULL;
    }
    return queue->front->data;
}

void printQueue(Queue *queue) {
    Node *current = queue->front;
    while (current != NULL) {
        // Print PCB data or any other relevant information
        printf("%d ", current->data->id);
        current = current->next;
    }
    printf("\n");
}

void freeQueue(Queue *queue) {
    while (queue->front != NULL) {
        Node *tempNode = queue->front;
        queue->front = queue->front->next;
        free(tempNode->data); // Assuming PCB data is dynamically allocated
        free(tempNode);
    }
    free(queue);
}

int main() {
    // Example usage of the queue

    Queue *readyQueue = initializeQueue();

    // Example PCB data
    PCB *process1 = (PCB *)malloc(sizeof(PCB));
    process1->id = 1;
    process1->arrivalTime = 0;
    // Initialize other PCB fields...

    PCB *process2 = (PCB *)malloc(sizeof(PCB));
    process2->id = 2;
    process2->arrivalTime = 3;
    // Initialize other PCB fields...

    enqueue(readyQueue, process1);
    enqueue(readyQueue, process2);

    printQueue(readyQueue);

    PCB *frontProcess = peek(readyQueue);
    if (frontProcess != NULL) {
        printf("Front of the queue: %d\n", frontProcess->id);
    }

    PCB *dequeuedProcess = dequeue(readyQueue);
    if (dequeuedProcess != NULL) {
        printf("Dequeued process: %d\n", dequeuedProcess->id);
        free(dequeuedProcess); // Free the dequeued PCB data
    }

    printQueue(readyQueue);

    freeQueue(readyQueue); // Free the entire queue along with PCB data

    return 0;
}

