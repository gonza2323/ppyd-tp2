// Autores: Lucía Alvarez, Paula Martínez, Gonzalo Padilla

#include <iostream>
#include <vector>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>
#include <unistd.h>
#include <sys/wait.h>

int compartida = 0;

void* tarea_proceso1(int id) {
    std::cout << "Proceso1 instancia " << id << std::endl;
    int tiempo = rand() % 1000001;
    usleep(tiempo);
    compartida++;
    return NULL;
}

void* tarea_proceso2(int id) {
    std::cout << "Proceso2 instancia " << id << std::endl;
    int tiempo = rand() % 1000001;
    usleep(tiempo);
    std::cout << "Compartida = " << compartida << std::endl;
    return NULL;
}

int main() {
    int N, M;
    std::cout << "Ingrese N: ";
    std::cin >> N;
    std::cout << "Ingrese M: ";
    std::cin >> M;

    // N procesos tipo 1
    for (int i = 0; i < N; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            std::cout << "Hijo " << i << " PID: " << getpid() << std::endl;
            tarea_proceso1(i);
            return 0; 
        }
    }
    // EL padre espera a que todos los hijos tipo 1 terminen
    for (int i = 0; i < N; i++) {
        wait(NULL);
    }
    
    // M procesos tipo 2
    for (int i = 0; i < M; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            std::cout << "Hijo " << i << " PID: " << getpid() << std::endl;
            tarea_proceso2(i);
            return 0; 
        }
    }
    //  EL padre espera a que todos los hijos tipo 2 terminen
    for (int i = 0; i < M; i++) {
        wait(NULL);
    }

    std::cout << "Fin del programa" << std::endl;
    return 0;
}