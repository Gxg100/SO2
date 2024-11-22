#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

#define SHM_NAME "/shm_counter"
#define SEM_NAME "/sem_counter"
#define MAX_COUNT 1000

void run_writer() {
    int shm_fd;
    int *shared_mem;
    sem_t *sem;

    
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("shm_open");
        exit(1);
    }
    ftruncate(shm_fd, sizeof(int)); 
    shared_mem = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    *shared_mem = 0; 
    
    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    
    printf("Proces scriere: Lansare cititori...\n");
    pid_t pid1 = fork();
    if (pid1 == 0) {
        execlp("./program", "./program", "reader", "1", NULL);
        perror("execlp cititor 1");
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        execlp("./program", "./program", "reader", "2", NULL);
        perror("execlp cititor 2");
        exit(1);
    }

    
    srand(time(NULL));
    for (int i = 1; i <= MAX_COUNT;) {
        sem_wait(sem); 
        if (*shared_mem == i - 1) {
            if (rand() % 2) {
                *shared_mem = i;
                printf("Proces scriere: Scris %d în memorie.\n", i);
                i++;
            }
        }
        sem_post(sem); 
        usleep(10000); 
    }

    
    wait(NULL);
    wait(NULL);

    munmap(shared_mem, sizeof(int));
    close(shm_fd);
    sem_close(sem);
}

void run_reader(int reader_id) {
    int shm_fd;
    int *shared_mem;
    sem_t *sem;
    int last_read = 0; 

    
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("shm_open");
        exit(1);
    }
    shared_mem = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

   
    sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    
    while (1) {
        sem_wait(sem); 
        if (*shared_mem > last_read) { 
            last_read = *shared_mem;
            printf("Cititor %d: Citit %d din memorie.\n", reader_id, last_read);
            if (last_read == MAX_COUNT) break; 
        }
        sem_post(sem); 
        usleep(5000); 
    }

    munmap(shared_mem, sizeof(int));
    close(shm_fd);
    sem_close(sem);
}

void run_cleanup() {
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);
    printf("Resursele au fost eliberate.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Utilizare: %s [writer|reader|cleanup] [reader_id dacă este reader]\n", argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "writer") == 0) {
        run_writer();
    } else if (strcmp(argv[1], "reader") == 0) {
        if (argc < 3) {
            printf("Trebuie să specificați un ID pentru cititor (ex: ./program reader 1).\n");
            exit(1);
        }
        int reader_id = atoi(argv[2]);
        run_reader(reader_id);
    } else if (strcmp(argv[1], "cleanup") == 0) {
        run_cleanup();
    } else {
        printf("Opțiune necunoscută: %s\n", argv[1]);
        printf("Utilizare: %s [writer|reader|cleanup] [reader_id dacă este reader]\n", argv[0]);
        exit(1);
    }

    return 0;
}
