#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure to represent an instruction and its duration
typedef struct {
    char name[10];
    int duration;
} Instruction;

// Enumeration to represent process types
typedef enum {
    PLATINUM,
    GOLD,
    SILVER
} ProcessType;

// Structure to represent a process
typedef struct {
    char name[4];
    int realArrivalTime;
    int arrivalTime;
    int priority;
    char typeString[10];
    ProcessType type;
    Instruction *instructions;
    int currentInstructionIndex;
    int exitIndex;
    int instructionCount;
    int executionTime;
    int waitingTime;
    int turnaroundTime;
    int CPUBurst;
    int executingTurn;
    int completed;
    int num;
    int timeQuantumUsed;
    int quantumIncrementFlag;
    int addedtoQueue;
} Process;

// Function to read instructions from the instruction file
void readInstructions(Instruction *instructions, int *instructionCount) {
    FILE *file = fopen("instructions.txt", "r");
    if (!file) {
        perror("Error opening instructions.txt");
        exit(EXIT_FAILURE);
    }

    *instructionCount = 0;
    while (fscanf(file, "%s %d", instructions[*instructionCount].name, &instructions[*instructionCount].duration) == 2) {
        (*instructionCount)++;
    }

    fclose(file);
}

// Function to find the duration of an instruction by name
int getInstructionDuration(Instruction *instructions, const char *name) {
    for (int i = 0; i < 21; i++) {
        if (strcmp(instructions[i].name, name) == 0) {
            return instructions[i].duration;
        }
    }
    return -1;  // Return -1 if the instruction is not found (error condition)
}

//function to get next arrival time when cpu is idle
int getNextArrivalTime(Process *processes, int processCount, int currentTime) {

    int nextArrivalTime = 150000;

    for (int i = 0; i < processCount; i++) {

        if(!(processes[i].completed) && (processes[i].realArrivalTime < nextArrivalTime) && (nextArrivalTime > currentTime)) {
            nextArrivalTime = processes[i].realArrivalTime;
        }

    }

    return nextArrivalTime;

}

int getQueueIndex ( Process *processes, char name[4], int readyQueue[10]) {

    for (int i = 0; i < 10; i++) {
        if (strcmp(name, processes[readyQueue[i]].name) == 0) {
            return i;
        }
    }

    return -1;

}

// Function to read processes from the definition file
void readProcesses(Process *processes, int *processCount) {
    FILE *file = fopen("definition.txt", "r");
    if (!file) {
        perror("Error opening definition file");
        exit(EXIT_FAILURE);
    }

    *processCount = 0;
    while (fscanf(file, "%s %d %d %s", processes[*processCount].name, &processes[*processCount].priority, &processes[*processCount].arrivalTime, processes[*processCount].typeString) == 4) {
        // Allocate memory for instructions based on the process's name

        if (strcmp(processes[*processCount].typeString, "PLATINUM") == 0) {
            processes[*processCount].type = 0;
        } else if (strcmp(processes[*processCount].typeString, "GOLD") == 0) {
            processes[*processCount].type = 1;
        } else if (strcmp(processes[*processCount].typeString, "SILVER") == 0) {
            processes[*processCount].type = 2;
        } else {
            processes[*processCount].type = 2;
        }

        char fileName[50];
        sprintf(fileName, "%s.txt", processes[*processCount].name);

        FILE *processFile = fopen(fileName, "r");
        if (!processFile) {
            perror("Error opening process file");
            exit(EXIT_FAILURE);
        }

        // Count the number of instructions in the process file
        int instructionCount = 0;
        char tempName[10];  // Temporary variable to store instruction names
        while (fscanf(processFile, "%s", tempName) == 1) {
            instructionCount++;
        }

        // Allocate memory for the correct number of instructions
        processes[*processCount].instructions = malloc(instructionCount * sizeof(Instruction));
        if (!processes[*processCount].instructions) {
            perror("Error allocating memory for instructions");
            exit(EXIT_FAILURE);
        }

        // Reset the file pointer to the beginning of the process file
        fseek(processFile, 0, SEEK_SET);

        for (int i = 0; i < instructionCount; i++) {
            if (fscanf(processFile, "%s", processes[*processCount].instructions[i].name) != 1) {
                perror("Error reading instruction from process file");
                exit(EXIT_FAILURE);
            } 
        }

        processes[*processCount].instructionCount = instructionCount;
        processes[*processCount].exitIndex = instructionCount;

        processes[*processCount].realArrivalTime = processes[*processCount].arrivalTime;

        processes[*processCount].executionTime = 0;
        processes[*processCount].waitingTime = 0;
        processes[*processCount].turnaroundTime = 0;
        processes[*processCount].CPUBurst = 0;
        processes[*processCount].currentInstructionIndex = 0;
        processes[*processCount].executingTurn = 0;
        processes[*processCount].completed = 0;

        int nameNumber = atoi(&processes[*processCount].name[1]);
        processes[*processCount].num = nameNumber;

        processes[*processCount].timeQuantumUsed = 0;
        processes[*processCount].quantumIncrementFlag = 0;
        processes[*processCount].addedtoQueue = 0;

        fclose(processFile);

        (*processCount)++;
        
    }
    

    fclose(file);
}

//function to execute the processes with while loop
void executeProcesses(Process *processes, int processCount, Instruction *instructions) {
    int currentTime = 0;
    int readyQueue[10];
    int front = -1, end = -1;
    int allProcessesCompleted = 1;
    int zamburdopDuration;
    int platflag = 0;
    char currentProcessName[4] = "init";
    int idleCPUFlag = 0;    

    //start the loop for executing
    while (1) {

        // if cpu is idle for 4 while turn, than jump to the arrival time of first next process (4 is selected just for extra control)
        if (idleCPUFlag > 4) {
            currentTime = getNextArrivalTime(processes, processCount, currentTime);
            //reset the flag
            idleCPUFlag = 0;
        }

        // search all processes and check for arriving ones, then add the process that has the highest priority and ready to execution to the front of the ready queue
        for (int i = 0; i < processCount; i++) {
            //if process has arrived and not yet completed, and there are no plat processes executing currently, then the process may be executed if its priority is highest
            if (processes[i].arrivalTime <= currentTime && !processes[i].completed && !platflag) {
    
                //first, check whether the arriving process is platinum and add it to the front of the ready queue, then check for another plat processes in the queue for comparison, add the highest priority plat to the front
                if (processes[i].type == PLATINUM) {

                    //if the prcoess at the front is not PLATINUM, change the current plat process with the one in the front
                    if(processes[i].addedtoQueue && !processes[readyQueue[front]].completed && processes[readyQueue[front]].type != PLATINUM) {
                        int temp = readyQueue[front];
                        int index = getQueueIndex(processes, processes[i].name, readyQueue);
                        readyQueue[front] = i;
                        readyQueue[index] = temp;
                    }

                    // if the process is plat, add it to the front of the ready queue, if there is another process in the front, put the other one to the end of the queue
                    if (front == -1) {
                        front = end = 0;
                        readyQueue[front] = i;
                        processes[i].addedtoQueue = 1;
                    } else if (!processes[i].addedtoQueue){
                        int temp = readyQueue[front];
                        end = (end + 1) % 10;
                        readyQueue[end] = temp;
                        readyQueue[front] = i;
                        processes[i].addedtoQueue = 1;
                    }
                    
                    //compare the process with other plat processes, if there is a plat process with higher priority, add that process to the front 
                    for (int i = (front + 1) % 10; i != (end + 1) % 10; i = (i + 1) % 10) {

                        int currentProcessIndex = readyQueue[front];
                        int nextProcessIndex = readyQueue[i];

                        

                        //do not compare the plat process with other processes
                        if (processes[nextProcessIndex].type != PLATINUM) {
                            continue;
                        }

                        
                        //if a plat process with higher priority is found, add it to the front, if there are plat processes with equal priorities, execute the firstcomer
                        if (/* (processes[currentProcessIndex].type == PLATINUM) &&  */(processes[nextProcessIndex].priority > processes[currentProcessIndex].priority)) {
                            //rearrange the ready queue 
                            readyQueue[front] = nextProcessIndex;
                            readyQueue[i] = currentProcessIndex;
                        } else if (/* (processes[currentProcessIndex].type == PLATINUM) &&  */(processes[currentProcessIndex].priority == processes[nextProcessIndex].priority) && (processes[currentProcessIndex].arrivalTime > processes[nextProcessIndex].arrivalTime)) {
                            //rearrange the ready queue 
                            readyQueue[front] = nextProcessIndex;
                            readyQueue[i] = currentProcessIndex;
                        } else if (/* (processes[currentProcessIndex].type == PLATINUM) &&  */(processes[currentProcessIndex].priority == processes[nextProcessIndex].priority) && (processes[currentProcessIndex].arrivalTime == processes[nextProcessIndex].arrivalTime) && (processes[currentProcessIndex].num > processes[nextProcessIndex].num)) {
                            //rearrange the ready queue 
                            readyQueue[front] = nextProcessIndex;
                            readyQueue[i] = currentProcessIndex;
                        }
                    }

                    //after ordering the plats, continue to look for another plats 
                    continue;

                }

                // if the ready queue is not empty or with one process, and the process to examine is not plat, compare the process in the front with other processes to check whether there is a process with higher priority that should be in front
                //first check the priorities, than arrival times, and then name order, than after choosing front, if it is not plat we will start round robin when executing
                if (front != -1) {


                    //do not add the process if it is already on the ready queue
                    if(!processes[i].addedtoQueue) {

                        end = (end + 1) % 10;
                        readyQueue[end] = i;
                        processes[i].addedtoQueue = 1;

                    }

                    // Check for higher priority processes in the ready queue
                    for (int j = (front + 1) % 10; j != (end + 1) % 10; j = (j + 1) % 10) {


                        int currentProcessIndex = readyQueue[front];
                        int nextProcessIndex = readyQueue[j];

                        //if the next process is completed, continue searching for uncompleted processes
                        if(processes[nextProcessIndex].completed) {
                            continue;
                        }

                        // Platinum processes cannot be compared with other type of processes
                        if((processes[currentProcessIndex].type != PLATINUM && processes[nextProcessIndex].type == PLATINUM) || (processes[currentProcessIndex].type == PLATINUM && processes[nextProcessIndex].type != PLATINUM)) {
                            continue;
                        }


                        //if there is a process with higher priority, add it to the front of the queue, else compare the arrival times and then the name order
                        if (processes[nextProcessIndex].priority > processes[currentProcessIndex].priority) {
                            //rearrange the ready queue 
                            //add the prior process to the front by replacing places with other process
                            readyQueue[front] = nextProcessIndex;
                            readyQueue[j] = currentProcessIndex;    
                        } else if ((processes[nextProcessIndex].priority == processes[currentProcessIndex].priority) && (processes[nextProcessIndex].arrivalTime < processes[currentProcessIndex].arrivalTime)) {
                            //rearrange the ready queue 
                            //add the prior process to the front by replacing places with other process
                            readyQueue[front] = nextProcessIndex;
                            readyQueue[j] = currentProcessIndex;
                        } else if ((processes[nextProcessIndex].priority == processes[currentProcessIndex].priority) && (processes[nextProcessIndex].arrivalTime == processes[currentProcessIndex].arrivalTime) && (processes[nextProcessIndex].num < processes[currentProcessIndex].num)) {
                            //rearrange the ready queue 
                            //add the prior process to the front by replacing places with other process
                            readyQueue[front] = nextProcessIndex;
                            readyQueue[j] = currentProcessIndex;
                        }

                        //if no conditions taken, then continue
                    }

                }

                // if ready queue is empty add the just add the process
                if (front == -1) {
                    front = end = 0;
                    readyQueue[end] = i;
                    processes[i].addedtoQueue = 1;
                }
            }
        }

        //execute the process at the front of the ready queue if the queue is not empty 
        if (front != -1 && !processes[readyQueue[front]].completed) {

            int currentProcessIndex = readyQueue[front];

            //if cpu is executing an instruction put down the idle flag 
            idleCPUFlag--;   

            //execute the plat process, first set the platflag in order to not interrupted when a plat process is executing, than execute the instructions
            if(processes[currentProcessIndex].type == PLATINUM) {

                //check if there is a context switch occured
                if(strncmp(processes[currentProcessIndex].name, currentProcessName, 3) != 0) {
                    currentTime += 10;
                    strncpy(currentProcessName, processes[currentProcessIndex].name, sizeof(processes[currentProcessIndex].name) - 1);
                    currentProcessName[sizeof(currentProcessName) - 1] = '\0';
                    
                }

                //set the plat flag in order to not to be preempted
                platflag = 1;

                //execute the instruction and increment current time
                zamburdopDuration = getInstructionDuration(instructions, processes[currentProcessIndex].instructions[processes[currentProcessIndex].currentInstructionIndex].name);
                currentTime += zamburdopDuration;

                //add the instruction time to process exec time
                processes[currentProcessIndex].executionTime += zamburdopDuration;

                //continue with next instruction
                processes[currentProcessIndex].currentInstructionIndex++;

                 // Check if the process is complete
                if (processes[currentProcessIndex].currentInstructionIndex == processes[currentProcessIndex].exitIndex) {
                    // Process completed
                    processes[currentProcessIndex].completed = 1;

                    //put down the platflag
                    platflag = 0;

                    //calculate wanted numbers
                    processes[currentProcessIndex].turnaroundTime = currentTime - processes[currentProcessIndex].realArrivalTime;
                    processes[currentProcessIndex].waitingTime = processes[currentProcessIndex].turnaroundTime - processes[currentProcessIndex].executionTime;

                    // Remove the process from the ready queue
                    if (front == end) {
                        front = end = -1;
                    } else {
                        front = (front + 1) % 10;
                    }
                }

            }
            
            //execute the gold process by doing round robin with time quantum of 120
            if (processes[currentProcessIndex].type == GOLD) {


                //if the process has been preempted by a higher priority process and forced to exit its round robin turn 
                //then count as one CPU burst used and start the process from where it left with time quantum reset
                if ((strncmp(processes[currentProcessIndex].name, currentProcessName, 3) != 0) && (processes[currentProcessIndex].timeQuantumUsed != 0)) {
                    processes[currentProcessIndex].timeQuantumUsed = 0;
                    //reset the quantum increment flag also for correctly updating the count when new instructions executed
                    processes[currentProcessIndex].quantumIncrementFlag = 0;

                    //if the used CPU burst has reached to 5, promote the process to PLATINUM
                    if (processes[currentProcessIndex].CPUBurst == 5) {
                        processes[currentProcessIndex].type = PLATINUM;
                        //reset the number of taken quantums
                        processes[currentProcessIndex].CPUBurst = 0;
                        
                    }

                }

                //if the process has used its time quantum, then it cannot execute instructions anymore and added to the ready queue again
                //else execute the instructions in the time quantum of 120
                if (processes[currentProcessIndex].timeQuantumUsed >= 120) {
                    
                    //put down the round robin flag as the process has used its given quantum
                    processes[currentProcessIndex].quantumIncrementFlag = 0;
                    
                    //reset the taken quantum time and increase the number of quantum taken
                    processes[currentProcessIndex].timeQuantumUsed = 0;
                    

                    //if the number of taken quantums has reached to 5, promote the process to plat
                    if (processes[currentProcessIndex].CPUBurst == 5) {
                        processes[currentProcessIndex].type = PLATINUM;
                        //reset the number of CPU bursts
                        processes[currentProcessIndex].CPUBurst = 0;
                    }

                    //assign the current time as new arrival time of the process, this will make round robin work i hope
                    //and raise the round robin negative flag to make sure that the process do not take consecutive turns
                    processes[currentProcessIndex].arrivalTime = currentTime;


                } else {

                    //check if there is a context switch occured
                    if(strncmp(processes[currentProcessIndex].name, currentProcessName, 3) != 0) {
                        currentTime += 10;
                        strncpy(currentProcessName, processes[currentProcessIndex].name, sizeof(processes[currentProcessIndex].name) - 1);
                        currentProcessName[sizeof(currentProcessName) - 1] = '\0';
                    }

                    //if its the first use of that quantum, increment the taken quantum number
                    if(!processes[currentProcessIndex].quantumIncrementFlag) {
                        processes[currentProcessIndex].CPUBurst++;
                    }

                    //raise this flag to correctly continue executing instructions in round robin time quantum
                    processes[currentProcessIndex].quantumIncrementFlag = 1;


                    //execute the instruction and increment current time
                    zamburdopDuration = getInstructionDuration(instructions, processes[currentProcessIndex].instructions[processes[currentProcessIndex].currentInstructionIndex].name);
                    currentTime += zamburdopDuration;

                    //add the instruction time to process exec time
                    processes[currentProcessIndex].executionTime += zamburdopDuration;

                    //increase the time of quantum used
                    processes[currentProcessIndex].timeQuantumUsed += zamburdopDuration;
                    

                    //continue with next instruction
                    processes[currentProcessIndex].currentInstructionIndex++;

                    // Check if the process is complete
                    if (processes[currentProcessIndex].currentInstructionIndex == processes[currentProcessIndex].exitIndex) {
                        // Process completed
                        processes[currentProcessIndex].completed = 1;

                        //put down the round robin positive flag
                        processes[currentProcessIndex].quantumIncrementFlag = 0;

                        //calculate wanted numbers
                        processes[currentProcessIndex].turnaroundTime = currentTime - processes[currentProcessIndex].realArrivalTime;
                        processes[currentProcessIndex].waitingTime = processes[currentProcessIndex].turnaroundTime - processes[currentProcessIndex].executionTime;

                        // Remove the process from the ready queue
                        if (front == end) {
                            front = end = -1;
                        } else {
                            front = (front + 1) % 10;
                        }
                    }

                }



            }

            //execute the silver process by doing round robin with time quantum of 80
            if (processes[currentProcessIndex].type == SILVER) {
                

                //if the process has been preempted by a higher priority process and forced to exit its round robin turn 
                //then count as one CPU burst used and start the process from where it left with time quantum reset
                if ((strncmp(processes[currentProcessIndex].name, currentProcessName, 3) != 0) && (processes[currentProcessIndex].timeQuantumUsed != 0)) {
                    processes[currentProcessIndex].timeQuantumUsed = 0;
                    //reset the quantum increment flag also for correctly updating the count when new instructions executed
                    processes[currentProcessIndex].quantumIncrementFlag = 0;

                    //if the used CPU burst has reached to 3, promote the process to GOLD
                    if (processes[currentProcessIndex].CPUBurst == 3) {
                        processes[currentProcessIndex].type = GOLD;
                        //reset the number of taken quantums
                        processes[currentProcessIndex].CPUBurst = 0;
                    }

                }

                //if the process has used its time quantum, then it cannot execute instructions anymore and added to the ready queue again
                //else execute the instructions in the time quantum of 80
                if (processes[currentProcessIndex].timeQuantumUsed >= 80) {
                    
                    //put down the round robin flag as the process has used its given quantum
                    processes[currentProcessIndex].quantumIncrementFlag = 0;
                    
                    //reset the taken quantum time and increase the number of quantum taken
                    processes[currentProcessIndex].timeQuantumUsed = 0;

                    //if the used CPU burst has reached to 3, promote the process to GOLD
                    if (processes[currentProcessIndex].CPUBurst == 3) {
                        processes[currentProcessIndex].type = GOLD;
                        //reset the number of taken quantums
                        processes[currentProcessIndex].CPUBurst = 0;
                    }

                    //assign the current time as new arrival time of the process, this will make round robin work i hope
                    //and raise the round robin negative flag to make sure that the process do not take consecutive turns
                    processes[currentProcessIndex].arrivalTime = currentTime;


                } else {

                    //check if there is a context switch occured
                    if(strncmp(processes[currentProcessIndex].name, currentProcessName, 3) != 0) {
                        currentTime += 10;
                        strncpy(currentProcessName, processes[currentProcessIndex].name, sizeof(processes[currentProcessIndex].name) - 1);
                        currentProcessName[sizeof(currentProcessName) - 1] = '\0';

                    }

                    //if its the first use of that quantum, increment the taken quantum number
                    if(!processes[currentProcessIndex].quantumIncrementFlag) {
                        processes[currentProcessIndex].CPUBurst++;
                    }

                    //raise this flag to correctly continue executing instructions in round robin time quantum
                    processes[currentProcessIndex].quantumIncrementFlag = 1;

                    //execute the instruction and increment current time
                    zamburdopDuration = getInstructionDuration(instructions, processes[currentProcessIndex].instructions[processes[currentProcessIndex].currentInstructionIndex].name);
                    currentTime += zamburdopDuration;

                    //add the instruction time to process exec time
                    processes[currentProcessIndex].executionTime += zamburdopDuration;

                    //increase the time of quantum used
                    processes[currentProcessIndex].timeQuantumUsed += zamburdopDuration;

                    //continue with next instruction
                    processes[currentProcessIndex].currentInstructionIndex++;

                    // Check if the process is complete
                    if (processes[currentProcessIndex].currentInstructionIndex == processes[currentProcessIndex].exitIndex) {
                        // Process completed
                        processes[currentProcessIndex].completed = 1;

                        //put down the round robin positive flag
                        processes[currentProcessIndex].quantumIncrementFlag = 0;

                        //calculate wanted numbers
                        processes[currentProcessIndex].turnaroundTime = currentTime - processes[currentProcessIndex].realArrivalTime;
                        processes[currentProcessIndex].waitingTime = processes[currentProcessIndex].turnaroundTime - processes[currentProcessIndex].executionTime;

                        // Remove the process from the ready queue
                        if (front == end) {
                            front = end = -1;
                        } else {
                            front = (front + 1) % 10;
                        }
                    }

                }



            }


        }

        // Check if there are uncompleted processes      
        for (int i = 0; i < processCount; i++) {
            if (processes[i].completed == 0) {
                allProcessesCompleted = 0;
                break;
            } else {
                allProcessesCompleted = 1;
            }
        }

        // exit if all processes completed
        if (allProcessesCompleted) {
            break;
        }

        // flag to check if CPU is waiting idle, without executing an instruction 
        idleCPUFlag++;      

    }
}

int main() {
    //read instructions and create an arrat
    Instruction instructions[20];
    int instructionCount;
    readInstructions(instructions, &instructionCount);

    //read processes and create an array
    Process processes[10];
    int processCount;
    readProcesses(processes, &processCount);

    // Execute processes
    executeProcesses(processes, processCount, instructions);

    // Calculate and print average waiting time and turnaround time
    int totalWaitingTime = 0;
    int totalTurnaroundTime = 0;

    for (int i = 0; i < processCount; i++) {
        totalWaitingTime += processes[i].waitingTime;
        totalTurnaroundTime += processes[i].turnaroundTime;
    }

    double avgWaitingTime = (double)totalWaitingTime / processCount;
    double avgTurnaroundTime = (double)totalTurnaroundTime / processCount;

    printf("%.1lf\n", avgWaitingTime);
    printf("%.1lf\n", avgTurnaroundTime);

    

    return 0;
}
