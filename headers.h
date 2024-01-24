#include <stdio.h>      //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300

#define GEN_TO_SCH_KEY 1564

#define NPROC 10 //Maximum number of processes

///==============================
//don't mess with this variable//
int * shmaddr;                 //
//===============================



int getClk()
{
    return *shmaddr;
}


/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
}


/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}
struct process {
    int id;
    int arrivalTime;
    int runTime;
    int priority;
} typedef process;

enum algorithm {
  SJF,
  SRTF,
  RR
} typedef algorithm; 

struct msgbuff {
  long mtype;
  process mprocess;
} typedef msgbuff;

typedef struct PCB {
    int id;
    int arrivalTime;
    int runTime;
    int priority;
    int startTime;
    int endTime;
    int remainingTime;
    int waitingTime;
    int turnaroundTime;
} PCB;


// RR Queue Implementation 
// Define a structure for a queue node
struct PCBQNode {
    PCB data;
    struct PCBQNode* next;
};
// Define a structure for the queue
struct PCBQueue {
    struct PCBQNode* front;  // Front of the queue
    struct PCBQNode* rear;   // Rear of the queue
};
// Function to initialize a new empty queue
void initPCBQueue(struct PCBQueue* queue) {
    queue->front = queue->rear = NULL;
}
// Function to enqueue a process in the queue
void enqueuePCBQ(struct PCBQueue* queue, PCB data) {
    // Create a new node
    struct PCBQNode* newNode = (struct PCBQNode*)malloc(sizeof(struct PCBQNode));
    if (newNode == NULL) {
        // Handle memory allocation failure
        perror("Error in enqueue: Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    // Set data and next pointer
    newNode->data = data;
    newNode->next = NULL;

    // If the queue is empty, set the new node as both front and rear
    if (queue->rear == NULL) {
        queue->front = queue->rear = newNode;
        return;
    }

    // Otherwise, add the new node at the end and update rear
    queue->rear->next = newNode;
    queue->rear = newNode;
}
int getQueueSize(struct PCBQueue* queue) {
    int size = 0;
    struct PCBQNode* current = queue->front;

    while (current != NULL) {
        size++;
        current = current->next;
    }

    return size;
}
// Function to dequeue a process from the queue
PCB dequeuePCBQ(struct PCBQueue* queue) {
    // If the queue is empty, return an "empty" process (you may define an empty process)
    if (queue->front == NULL) {
        PCB emptyProcess = {-1, -1, -1, -1};
        return emptyProcess;
    }

    // Otherwise, dequeue the front node and update front
    struct PCBQNode* temp = queue->front;
    PCB data = temp->data;

    queue->front = temp->next;

    // If front becomes NULL, update rear as well
    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    free(temp);  // Free the dequeued node

    return data;
}
