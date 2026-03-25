// Autores: Lucía Alvarez, Paula Martínez, Gonzalo Padilla

#include <iostream>
#include <vector>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>
#include <mutex>

int compartida = 0;
std::mutex mtx;

void* tarea_thread1(void* arg) {
    int id = *((int*) arg);
    std::cout << "Thread1 instancia " << id << std::endl;
    int tiempo = rand() % 1000001;
    usleep(tiempo);
    mtx.lock();
    compartida++;
    mtx.unlock();
    return NULL;
}

void* tarea_thread2(void* arg) {
    int id = *((int*) arg);
    std::cout << "Thread2 instancia " << id << std::endl;
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

    std::vector<pthread_t> threads1(N);
    std::vector<pthread_t> threads2(M);
    std::vector<int> ids1(N);
    std::vector<int> ids2(M);

    for (int i = 0; i < N; i++) {
        ids1[i] = i;
        pthread_create(&threads1[i], NULL, tarea_thread1, &ids1[i]);
    }
    for (int i = 0; i < M; i++) {
        ids2[i] = i;
        pthread_create(&threads2[i], NULL, tarea_thread2, &ids2[i]);
    }
    for (int i = 0; i < N; i++) {
        pthread_join(threads1[i], NULL);
    }
    for (int i = 0; i < M; i++) {
        pthread_join(threads2[i], NULL);
    }

    std::cout << "Fin del programa" << std::endl;
    return 0;
}