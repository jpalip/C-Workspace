#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char **argv) {

    int fd = open("data", O_RDONLY, S_IRUSR | S_IWUSR); // Opens the file for memory

    // Gets thed filesize
    struct stat info;
    fstat(fd, &info);

    char* sharedMem = mmap(NULL, info.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0); // Stored "data" file into shared memory 

    // Opens shared memory
    int fd2 = shm_open("/shared", O_RDWR | O_CREAT, 0777);
    if(fd == -1) {
        printf("shm open error");
    }


    int trunc = ftruncate(fd2, 10); // truncates mem

    // Checks for failure
    if(sharedMem == MAP_FAILED) { 
        printf("Map failed!");
    }

    int pid = fork();

    // Child process Prints shared Memory
    if(pid == 0) {
        printf("Child\n");
        for(int i = 0; i < info.st_size; i++) {
            printf("%c", sharedMem[i]);
        }
    }
    // Parent process reads and writes shared memory to new file
    else {
        wait(NULL);
        printf("Parent\n");
        printf("Wrote shared memory to new file.");
        int newfile = open("dataCOPY", O_RDWR | O_CREAT); // Opens new file to copy from shared memory
        write(newfile, sharedMem, info.st_size); // writes the shared memory to the new file
        
    }
    printf("\n");
}