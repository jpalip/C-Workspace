#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char **argv) {

    // Opens file to store in memory
    int fd = open("data", O_RDONLY); 
    char* shM = mmap(NULL, 10000, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

    // Opens shared memory
    int mem = shm_open("/newmem", O_RDWR | O_CREAT, 0777);
    if(mem == -1) {
        printf("error");
    }
    int trunc = ftruncate(mem, 10); // truncates mem

    // Checks for failure
    if(shM == MAP_FAILED) { 
        printf("Map failed!");
    }

    // Prints Shared Memory
    printf("Shared Memory :\n");
    for(int i = 0; i < 1000; i++) {
        printf("%c", shM[i]);
    }

    // Creates new file
    int fd2 = open("newfile", O_CREAT | O_RDWR);
    write(fd2, shM, sizeof(shM));
    printf("\nWrote shared memory to new file");

    printf("\n");
}