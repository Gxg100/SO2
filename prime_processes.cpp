#include <iostream>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <sys/wait.h>

bool is_prime(int num) {
    if (num < 2) return false;
    for (int i = 2; i * i <= num; ++i) {
        if (num % i == 0) return false;
    }
    return true;
}

void find_primes(int start, int end, int write_pipe) {
    std::vector<int> primes;
    for (int i = start; i <= end; ++i) {
        if (is_prime(i)) primes.push_back(i);
    }

    
    for (int prime : primes) {
        write(write_pipe, &prime, sizeof(prime));
    }

    
    int end_signal = -1;
    write(write_pipe, &end_signal, sizeof(end_signal));
    close(write_pipe);
}

int main() {
    const int RANGE = 1000;
    const int PROCESS_COUNT = 10;

    int pipes[PROCESS_COUNT][2];
    pid_t pids[PROCESS_COUNT];

    for (int i = 0; i < PROCESS_COUNT; ++i) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return 1;
        }

        int start = i * RANGE + 1;
        int end = (i + 1) * RANGE;

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            return 1;
        }

        if (pids[i] == 0) { 
            close(pipes[i][0]); 
            find_primes(start, end, pipes[i][1]);
            return 0; 
        } else { 
            close(pipes[i][1]); 
        }
    }

   
    for (int i = 0; i < PROCESS_COUNT; ++i) {
        int prime;
        while (read(pipes[i][0], &prime, sizeof(prime)) > 0) {
            if (prime == -1) break; 
            std::cout << prime << " ";
        }
        close(pipes[i][0]);
    }

    
    for (int i = 0; i < PROCESS_COUNT; ++i) {
        waitpid(pids[i], nullptr, 0);
    }

    std::cout << "\nToate numerele prime au fost afișate.\n";
    return 0;
}
