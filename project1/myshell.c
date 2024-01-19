#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h> 

//define struct to handle and store aliases 
typedef struct Alias {
    char *name;
    char *command;
} Alias;

//function to add an alias to the linked list
void addAlias(char *name, char *command) {

    Alias *newAlias = malloc(sizeof(Alias));
    if (!newAlias) {
        perror("Failed to allocate memory for alias");
        exit(EXIT_FAILURE);
    }

    newAlias->name = strdup(name);
    newAlias->command = strdup(command);

    
    // Write the alias to the file
    FILE *file = fopen("AliasFile.txt", "a");
    if (!file) {
        perror("Failed to open AliasFile.txt for writing");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "%s=%s\n", name, command);

    fclose(file);
}

//function to find an alias in the linked list
char *findAlias(char *name) {

    //check the file AliasFile.txt exit, return null if not
    FILE *file = fopen("AliasFile.txt", "r");
    if (!file) {
        return NULL;
    }

    
    char buffer[256];
    strcpy(buffer, "");

    //search the file and if alias exists in the file, return the command
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        char *aliasName = strtok(buffer, "=");
        
        char *aliasCommand = strtok(NULL, "\n");
        

        if (aliasName != NULL && aliasCommand != NULL && strcmp(aliasName, name) == 0) {
            fclose(file);
            return strdup(aliasCommand);
        }
    }

    fclose(file);

    return NULL;
}

//function to concatenate a string to the front of another string (used when alias execution)
char* concatenateToFront(const char* prefix, const char* original) {
    // Allocate memory for the new string
    char* result = malloc(strlen(prefix) + strlen(original) + 1);

    // Check if memory allocation was successful
    if (result == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    // Copy the prefix to the new string
    strcpy(result, prefix);

    // Add a space between the prefix and original
    strcat(result, " ");

    // Concatenate the original string to the front of the new string
    strcat(result, original);

    return result;
}

//function to print the shell prompt
void printPrompt() {
    struct passwd *pw = getpwuid(getuid());
    const char *username = pw->pw_name;

    // Get the machine name (hostname)
    char hostname[1024];
    gethostname(hostname, sizeof(hostname));

    // Get the current directory
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));

    printf("%s@%s %s --- ", username, hostname, cwd);
}

//bello function
void displayUserInfo(char *lastExCmd) {
    struct passwd *pw = getpwuid(getuid());
    const char *username = pw->pw_name;
    char hostname[1024];
    gethostname(hostname, sizeof(hostname));
    char tty[1024];
    tty[readlink("/proc/self/fd/0", tty, sizeof(tty))] = '\0';
    char *shellName = getenv("SHELL");
    char *homeLocation = pw->pw_dir;
    time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
    // Get the load average information
    double loadavg[3];
    getloadavg(loadavg, 3);

    printf("1. Username: %s\n", username);
    printf("2. Hostname: %s\n", hostname);
    printf("3. Last Executed Command: %s\n", lastExCmd);  
    printf("4. TTY: %s\n", tty);
    printf("5. Current Shell Name: %s\n", shellName);
    printf("6. Home Location: %s\n", homeLocation);
    printf("7. Current Time and Date: %s", asctime(localTime));
    printf("8. Current number of processes being executed: %.2f\n", loadavg[0]);  // Replace <process_count> with actual count
}

//function to reverse the buffer (used when invert operation (>>>) is used)
void reverseBuffer(char *buffer, size_t length) {
    if (buffer == NULL || length <= 1) {
        // Nothing to reverse
        return;
    }

    size_t start = 0;
    size_t end = length - 2;

    while (start < end) {
        // Swap characters at start and end indices
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;

        // Move indices towards the center
        start++;
        end--;
    }
}


void executeCommand(char **argv, int background, int argc) {

    char *success_executable_path = NULL;
    int file_exists = 0;
    
    //initialize the flags and file name for the redirection operators
    char *output_file = NULL;
    int append = 0;
    int invert = 0;

    // Look for redirection operators and extract output file
    for (int i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], ">") == 0 || strcmp(argv[i], ">>") == 0 || strcmp(argv[i], ">>>") == 0) {
            if (argv[i + 1] == NULL) {
                fprintf(stderr, "Syntax error: Missing output file after redirection operator\n");
                return;
            }

            //assign output file
            output_file = argv[i + 1];
           
            //assign flags

            /* if (strcmp(argv[i], ">") == 0) {
                append = 0;
            } */

            if (strcmp(argv[i], ">>") == 0) {
                append = 1;
            }

            if (strcmp(argv[i], ">>>") == 0) {
                append = 1;
                invert = 1;
            }

            // Remove the redirection operator and output file from argv
            argv[i] =NULL;
            argv[i + 1] =NULL;
            argc += -2;
            break;
        }
    }

    // look at the path
    char *path = getenv("PATH");
    if (path == NULL) {
        fprintf(stderr, "PATH environment variable not set\n");
        return;
    }

    char *path_copy = strdup(path);
    if (path_copy == NULL) {
        perror("Failed to duplicate PATH");
        return;
    }

    char *dir = strtok(path_copy, ":");

    while (dir != NULL) {
        //allocate memory
        char *executable_path = malloc(strlen(dir) + strlen(argv[0]) + 2);
        if (executable_path == NULL) {
            perror("Failed to allocate memory for executable path");
            free(path_copy);
            return;
        }

        // Check if the last character of dir is '/'
        if (dir[strlen(dir) - 1] == '/') {
            sprintf(executable_path, "%s%s", dir, argv[0]);
        } else {
            sprintf(executable_path, "%s/%s", dir, argv[0]);
        }

        // Check if the executable exists in the current directory
        if (access(executable_path, X_OK) == 0) {
            file_exists = 1;

            // Copy the successful executable_path to success_executable_path
            success_executable_path = strdup(executable_path);
            if (success_executable_path == NULL) {
                perror("Failed to duplicate executable path");
                free(executable_path);
                free(path_copy);
                return;
            }

            break;
        }

        free(executable_path);
        dir = strtok(NULL, ":");
    }

    free(path_copy);

    // Check if the command was found in the path
    if (!file_exists) {
        // If the loop completes, the command was not found
        fprintf(stderr, "Command not found: %s\n", argv[0]);
        errno = 31; //update errno to check whether command was executed
        return;
    }

    //for >>> operation, we need to create a pipe
    int pipe_fd[2];

    if (pipe(pipe_fd) == -1) {
        perror("Failed to create pipe");
        return;
    }


    // Execute the command in the child process
    pid_t pid = fork();

    if (pid == -1) {
        perror("Failed to create child process");
    }

    if (pid == 0) {
        // child process

        // Handle output redirection
        if (output_file != NULL) {
            //printf("yes");
            int flags = O_WRONLY | O_CREAT;
            if (append) {
                flags |= O_APPEND;
            } else {
                flags |= O_TRUNC;
            }

            if (invert) {

                //pipe to invert message
                close(pipe_fd[0]);

                // Redirect stdout to the pipe
                dup2(pipe_fd[1], STDOUT_FILENO);
                close(pipe_fd[1]);

            } 

            if (!invert){

                int output_fd = open(output_file, flags, 0644);
                if (output_fd == -1) {
                    perror("Failed to open output file");
                    free(success_executable_path);
                    exit(EXIT_FAILURE);
                }
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);

            }
                        
        }

        //update the argv with the correct path
        argv[0] = success_executable_path;
        argv[argc + 1] = NULL;


        //execute the command
        execv(success_executable_path, argv);
        
        //if execv fails, print an error message
        perror("Failed to execute execv");
        free(success_executable_path);
        //free(argv);
        exit(EXIT_FAILURE);
    } else {

        //parent process
        if (!background) {
            //if not in the background, wait for the child to finish
            waitpid(pid, NULL, 0);
        }


        if (invert) {
            close(pipe_fd[1]);  //close the read end of the pipe

            //read the result from the pipe, invert it, and write to the file
            char buffer[4096];
            //clear buffer
            strcpy(buffer, "");
            
            ssize_t bytesRead = read(pipe_fd[0], buffer, sizeof(buffer) - 1);
            
            
            
            if (bytesRead == -1) {
                perror("Failed to read from pipe");
            } else {

                buffer[strlen(buffer)] = '\0';
                //invert the message
                reverseBuffer(buffer, strlen(buffer));
                
                //open the file
                int output_fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (output_fd == -1) {
                    perror("Failed to open output file");
                } else {
                    //write the inverted message to the file
                    write(output_fd, buffer, bytesRead);
                    //size_t bytesWritten = write(output_fd, buffer, bytesRead);
                    /* if (bytesWritten == -1) {
                        perror("Failed to write to file");
                    } */
                    close(output_fd);
                }
            }   
            
        close(pipe_fd[0]);  //close the write end of the pipe

        }

    }

    // Free memory for success_executable_path after the child process completes
    free(success_executable_path);
}


int main() {
    //initialize last executed command for bello
    char *lastExCmd = NULL;
    while (1) {
        
        //initialize 
        char *cmdLine = NULL;
        char *cmd_cpy = NULL;
        char *cmdLineCopy = NULL;  // this for alias
        char *token = NULL;
        char *delim = " \n";
        size_t n = 0;
        int argc = 0;
        int i = 0;
        char **argv = NULL;

        printPrompt();

        //get line from user
        getline(&cmdLine, &n, stdin);

        //add ending character
        cmdLine[strcspn(cmdLine, "\n")] = '\0';    

        //continue if enter typed
        if (strlen(cmdLine) == 0 || strcmp(cmdLine, "") == 0 || cmdLine == NULL) {
            continue;
        } 



        int background = 0;  // Flag to indicate background execution
        cmd_cpy = strdup(cmdLine); //this for argv assigning after first cmdline used for arg
        cmdLineCopy = strdup(cmdLine); //this for alias executing and creating
        
        

        //take first token (which is our command) in the line
        token = strtok(cmdLine, delim);

        //continue if space typed
        if (token == NULL) {
            continue;
        }
        


        //exit if command is exit
        if (strcmp(token, "exit") == 0) {
            // Free allocated memory before exiting
            free(cmdLine);
            free(cmd_cpy);
            free(cmdLineCopy);
            free(argv);
            break;
        }

        //parse accordingly if command is alias
        if (strcmp(token, "alias") == 0) {
            // Find the position of the equal sign (=)
            char *equalSign = strchr(cmdLineCopy, '=');

            if (equalSign != NULL) {
                // Extract the alias name
                char *aliasName = strtok(cmdLineCopy + 6, " =\"");
           
                // Extract the alias command along with its arguments
                char *aliasCmd = equalSign + 1;
                //printf("aliasCmd: %s.\n", aliasCmd);

                // Remove leading and trailing spaces from aliasCmd
                char *trimmedAliasCmd = strtok(aliasCmd, "\" \t\n");
                //printf("trimmedAliasCmd: %s.\n", trimmedAliasCmd);               
            
                // Save the alias
                addAlias(aliasName, trimmedAliasCmd);
            }
            // Update last executed command
            if(errno != 32) {
                lastExCmd = strdup(token);
            }
            continue;
        }
        

        //checking alias and updating the command line accordingly if alias is valid
        char *aliascmd = findAlias(token);
        if (aliascmd != NULL) {
            //find the index of the first space character
            size_t firstSpaceIndex = strcspn(cmdLineCopy, " ");

            char *cmdLineWithoutFirstWord = strdup(cmdLineCopy + firstSpaceIndex + 1);
            //concatenate the alias command to the front of the modified cmdLine
            char *tempcmd = concatenateToFront(aliascmd, cmdLineWithoutFirstWord);
            
            char *newCmdLine = strdup(tempcmd);
            free(cmdLine);  //free old cmdLine
            free(cmd_cpy);  //and old cmd_cpy 
            cmdLine = newCmdLine;
            cmd_cpy = strdup(cmdLine);

            token = strtok(cmdLine, delim); //continue parsing new cmdline if alias is found

            //free the completed variables
            free(tempcmd);
            free(cmdLineWithoutFirstWord);
            
            
        }

        

        //loop to calculate arg count
        while (token) {
            token = strtok(NULL, delim);
            argc++;
        }

        
        //memory allocation for args
        argv = malloc(sizeof(char *) * (argc + 1));

        //parsing again for arguments
        token = strtok(cmd_cpy, delim);

        //loop to assign arg values
        while (token) {
            argv[i] = token;
            token = strtok(NULL, delim);
            i++;
        }
        //last element is assigned to null
        argv[i] = NULL;

        if (argc > 0) {

            //check if process works in background
             if (strcmp(argv[argc - 1], "&") == 0) {
                background = 1;
                argv[argc - 1] = NULL;  // Remove the "&" token
            }

            // Display user information for the "bello" command
            if (strcmp(argv[0], "bello") == 0) {
                displayUserInfo(lastExCmd);
                //update last command to bello
                lastExCmd = strdup(argv[0]);
                continue;
            }
            

            // Execute the command
            executeCommand(argv, background, argc);

            //check if command executed properly and update last command
            if(errno != 31) {
                lastExCmd = strdup(argv[0]);
            }

            //free(argv);


        }


        // Free allocated memory
        free(cmdLine);
        free(cmd_cpy);
        free(cmdLineCopy);
        free(argv);
        

    }

    if (lastExCmd != NULL) {
        free(lastExCmd);
    }


    return 0;
}