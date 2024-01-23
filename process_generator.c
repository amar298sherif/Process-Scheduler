#include "headers.h"
#define NPROC 10
void clearResources(int);
void readInput(process plist[], int nproc, char* filepath);

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    process procList[NPROC]; // 100 is maximum number of processes since we cannot make variable arrays.
    readInput(procList, NPROC, "processes.txt");

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    // 3. Initiate and create the scheduler and clock processes.
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    printf("current time is %d\n", x);
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    // TODO Clears all resources in case of interruption
    system("ipcrm -a");
    signal(SIGINT, SIG_DFL);
    kill(getpid(), SIGINT);
}

/**
 * @brief Takes in an array of processes and fills them with their params from the input file
 * 
 * @param plist array of processes
 * @param nproc number of total processes
 * @param filepath input file to read data from (MUST BE GENERATED WITH TEST_GENERATOR.C FILE)
 */
void readInput(process plist[], int nproc, char* filepath)
{
  FILE *file = fopen(filepath, "r");
  if (!file)
  {
    perror("Couldn't open file");
    exit(-1);
  }
  const int buffsize = (nproc * 25) + 200;
  char buff[buffsize];

  fseek(file, 0, SEEK_END);

  int filesize = ftell(file);
  if (filesize > buffsize)
  {
    printf("The filesize exceeds the declared maxsize.\n");
    exit(-1);
  }
  fseek(file, 0, SEEK_SET);

  fread(buff, sizeof(char), filesize, file);
  buff[filesize] = '\0';

  // Close the file
  fclose(file);

  size_t i = 0;
  size_t index = 0;
  while (buff[i] != '0')
  {
    // bypass comments
    if (buff[i] == '#')
    {
      while (buff[i] != '\n' && buff[i] != '\0')
      {
        i++;
      }
      if (buff[i] == '\n')
        i++;
      else
        break;
      continue;
    }
    if (buff[i] == '\n')
    {
      i++;
      continue;
    }
    if (buff[i] == '\0')
    {
      break;
    }

    // dealing with numbers
    while (buff[i] != '\n' && buff[i] != '\0')
    {
      for (size_t j = 0; j < 4; j++)
      {
        int n = 0;
        int d = 1;
        while (buff[i] != '\t' && buff[i] != '\n' && buff[i] != '\0')
        {
          n = d * n + (buff[i] - '0');
          i++;
          d = d * 10;
        }
          i++;

        switch (j)
        {
        case 0:
          plist[index].pid = n;
          break;
        case 1:
          plist[index].arrivalTime = n;
          break;
        case 2:
          plist[index].runTime = n;
          break;
        case 3:
          plist[index++].priority = n;
          break;
        default:
          break;
        }
      }
    }
    i++;
  }
}