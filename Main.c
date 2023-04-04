#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <sys/ipc.h>                                
#include <sys/wait.h>
#include <sys/types.h>
#include <semaphore.h>

#define N 5                             // Total amount of children
#define MINIMUM_DOCUMENT_LINES 1001     // Minimum document's lines

#define INITIAL_VALUE_0 0
#define INITIAL_VALUE_1 1
#define SEMAPHORE_PERMS 0664
#define SEMAPHORE_FLAGS (O_CREAT)
#define SEMAPHORE1 "child_choice"           // Usage of 4 named posix
#define SEMAPHORE2 "having_request"         // semaphores because its
#define SEMAPHORE3 "sending_request"        // easier to understand 
#define SEMAPHORE4 "copying_informations"   // the code and they are newer

#define BUFFER_SIZE 100             // Maximum size of a line 100 characters
#define BUFFER_MAX_LINES 2000       // Maximum size of the segment 100 lines
#define IPC_RESULT_ERROR (-1)
#define SEGMENTPERM (0666 | IPC_CREAT)
#define SEGMENTSIZE sizeof(shared_memory)

typedef struct{
    int segment, last_request;                  // Variables to store useful informations
    char buffer[BUFFER_MAX_LINES][BUFFER_SIZE]; // Buffer to store a segment
}shared_memory;

int main(int argc, char** argv){
    if(argc != 4){
        if(argc < 4){
            printf("Not enough arguments. Try again.\n");
            exit(EXIT_FAILURE);
        }
        printf("Too many arguments. Try again.\n");
        exit(EXIT_FAILURE);
    }

    if(atoi(argv[2]) > BUFFER_MAX_LINES){
        printf("Maximum lines in segment %d. Try again.\n", BUFFER_MAX_LINES);
        exit(EXIT_FAILURE);
    }

    int document_lines = 0;
    FILE* file = fopen(argv[1], "r");
    if(file != NULL){
        for(char c = getc(file); c != EOF; c = getc(file)){
            if(c == '\n'){
                document_lines++;
            }
        }
        if(document_lines <= MINIMUM_DOCUMENT_LINES){
            printf("The total amount of lines are not enough (%d). Need more than 1000. Try again.\n", document_lines);
            exit(EXIT_FAILURE);
        }  
    }else{
        printf("Unable to open the file. Try again.\n");
        exit(EXIT_FAILURE);
    }

    int pid;
    int child_requests = atoi(argv[3]);
    int lines_in_segment = atoi(argv[2]);
    int random_segment, random_line_in_segment;
    int segments = (int) document_lines/lines_in_segment;
    char* string = (char*) malloc(sizeof(char) * BUFFER_SIZE);  // String that the child will write from shared memory and read this string
    char* name_for_log_file = (char*) malloc(sizeof(char) * BUFFER_SIZE);

    /**** Setup semaphores and error checking (unlink in case of crash) ****/
    
    sem_unlink(SEMAPHORE1);
    sem_unlink(SEMAPHORE2);
    sem_unlink(SEMAPHORE3);
    sem_unlink(SEMAPHORE4);

    sem_t* child_choice = sem_open(SEMAPHORE1, SEMAPHORE_FLAGS, SEMAPHORE_PERMS, INITIAL_VALUE_1);  // Child whats to find segment and line
    sem_t* having_request = sem_open(SEMAPHORE2, SEMAPHORE_FLAGS, SEMAPHORE_PERMS, INITIAL_VALUE_0);    // Child having a request 
    sem_t* sending_request = sem_open(SEMAPHORE3, SEMAPHORE_FLAGS, SEMAPHORE_PERMS, INITIAL_VALUE_0);       // Father giving the request back to the child
    sem_t* copying_informations = sem_open(SEMAPHORE4, SEMAPHORE_FLAGS, SEMAPHORE_PERMS, INITIAL_VALUE_1);      // Father writing to the shared memory

    if(child_choice == SEM_FAILED || having_request == SEM_FAILED || sending_request == SEM_FAILED || copying_informations == SEM_FAILED){
        perror("Semaphore open failure.");
        exit(EXIT_FAILURE);
    }

    /**** Setup shared memory and error checking ****/

    int shm_id = shmget(IPC_PRIVATE, SEGMENTSIZE, SEGMENTPERM);
    if(shm_id == IPC_RESULT_ERROR){
        perror("Shared memory creation error.");
        exit(EXIT_FAILURE);
    }

    shared_memory* shm = (shared_memory*) shmat(shm_id, NULL, 0);
    if(shm == (void*) IPC_RESULT_ERROR){
        perror("Shared memory attach error.");
        exit(EXIT_FAILURE);
    }

    shm->segment = 0;
    shm->last_request = 0;
    
    /**** Child processes ****/

    for(int i = 0; i < N; i++){
        if((pid = fork()) < 0){
            perror("Fork failure.");
            exit(EXIT_FAILURE);
        }

        if(pid == 0){
            sprintf(name_for_log_file, "log_file_%d.txt", i + 1);
            FILE* log = fopen(name_for_log_file, "w+");

            srand((unsigned) getpid());
            clock_t start_time, end_time;

            for(int j = 0; j < child_requests; j++){
                sem_wait(child_choice);    // A child wants to find a segment and a line
                if(j != 0){
                    if((rand() % 100 + 1) <= 70){                                   // Child has 70% probability to 
                        random_line_in_segment = rand() % lines_in_segment + 1;     // pick the same segment but different 
                    }else{                                                          // line (segment remains the same),
                        random_segment = rand() % segments + 1;                     // but 30% to pick another random segment
                        random_line_in_segment = rand() % lines_in_segment + 1;     // and another random line
                    }
                }else{
                    random_segment = rand() % segments + 1;                         // If it is the first time just pick
                    random_line_in_segment = rand() % lines_in_segment + 1;         // random segment and random line
                }
                shm->segment = random_segment;  // My request will be for this segment that shared memory has
   
                shm->last_request++;     // Counting whenever a child request is done

                /**** Child making request-receiving the request and doing it's job ****/

                start_time = clock();
                sem_post(having_request);   // Child has request to the parent
                sem_wait(sending_request);  // Wait for informations to be writen
                sem_post(child_choice);     // Also another child can find what segment and line wants
                end_time = clock();

                strcpy(string, shm->buffer[random_line_in_segment - 1]);    // Copying the string that it wants to print it
                usleep(20000);                                              // Time to process

                fprintf(log, "Child with pid %d selected < %d , %d >. Message is : %s.", getpid(), random_segment, random_line_in_segment, string);
                fprintf(log, "Child process %d made the request : %f and received it : %f.\n", getpid(), (double) start_time/CLOCKS_PER_SEC, (double) end_time/CLOCKS_PER_SEC);
                
                sem_post(copying_informations); // Parent can write again in shared memory
            }
            fclose(log);        // Child is done so closing his file 
            exit(EXIT_SUCCESS); // Child is exiting with no error messages (by passing 0 argument in exit())
        }
    }

    /**** Parent process ****/

    bool finish = false;
    clock_t segment_entry_time, segment_exit_time;
    FILE* log_parent = fopen("log_parent.txt", "w");
    
    while(finish == false){
        sem_wait(having_request);   // Wait for the request

        if(shm->last_request == (N * child_requests)){  // Stop whenever all children are done with their requests
            finish = true;
        }

        /**** Find all lines in the requested segment (request segment == segment in shared memory (shm->segment) ****/

        sem_wait(copying_informations); // No child can write will parent is copying the segment
        segment_entry_time = clock();   // Segment is about to be in shared memory
        int i = 0;                                                                                 // 3 segments with 200 lines each
        int first_document_line = 1;                                                               // 1st seg -> lines : 1-200
        int last_line_of_segment = shm->segment * lines_in_segment;                                // 2nd seg -> lines : 201-400
        int first_line_of_segment = (shm->segment * lines_in_segment) - (lines_in_segment - 1);    // 3rd seg -> lines : 401-600
        rewind(file);   // Set the file pointer to the begging of the stream, to find segment end copy lines (if necessery) for the next child 
        while(first_document_line <= last_line_of_segment){                                             
            fgets(string, BUFFER_SIZE, file);   // Take the line
            if(first_document_line >= first_line_of_segment){           
                strcpy(shm->buffer[i], string); // Copy the segment into the shared memory
                i++;
            }
            first_document_line++;
        }

        fprintf(log_parent, "Segment %d entry time : %f.\n", shm->segment, (double) segment_entry_time/CLOCKS_PER_SEC);
        sem_post(sending_request);      // Child is ready...all information have been copied
        segment_exit_time = clock();    // another segment is about to be in shared memory
        fprintf(log_parent, "Segment %d exit time : %f.\n", shm->segment, (double) segment_exit_time/CLOCKS_PER_SEC);
    }

    /**** Collect children ****/

    while((pid = wait(NULL)) > 0){
        printf("Colecting child with pid %d.\n", pid);
    }

    fclose(file);
    fclose(log_parent);

    if(shmdt((void*) shm) == IPC_RESULT_ERROR){
        perror("Shared memory detachment failure.");
    }
    if(shmctl(shm_id, IPC_RMID, 0) == IPC_RESULT_ERROR){
        perror("Shared memory removal failure.");
    }
    if(sem_close(child_choice) < 0 || sem_close(having_request) < 0 || sem_close(sending_request) < 0 || sem_close(copying_informations)){
        perror("Semaphore closure failure.");    
    }

    free(string);
    free(name_for_log_file);

    exit(EXIT_SUCCESS);
}