// Autores: Lucía Alvarez, Paula Martínez, Gonzalo Padilla
// Para compilar, en el directorio de este tp ejecutar:
// g++ -std=c++17 *.cpp -pthread -o peaje

#include "vehicle.h"
#include <mutex>
#include <condition_variable>

class VehicleQueue
{
public:
    void enqueue(Vehicle vehicle);
    bool deque(Vehicle &vehicle);
    void close();

private:
    static const int QUEUE_SIZE = 15;
    Vehicle arr[QUEUE_SIZE];
    std::mutex mtx;
    std::condition_variable cv_consumers;
    std::condition_variable cv_producers;
    int head = 0;
    int size = 0;
    bool closed = false; // NEW
};