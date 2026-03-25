// Autores: Lucía Alvarez, Paula Martínez, Gonzalo Padilla
// Para compilar, en el directorio de este tp ejecutar:
// g++ -std=c++17 *.cpp -pthread -o peaje

#include "queue.h"

void VehicleQueue::enqueue(Vehicle vehicle) {
    std::unique_lock<std::mutex> lock(mtx);

    cv_producers.wait(lock, [this]{ return size < QUEUE_SIZE || closed; });

    if (closed) return;

    arr[(head + size) % QUEUE_SIZE] = vehicle;
    size++;

    cv_consumers.notify_one();
}

bool VehicleQueue::deque(Vehicle &vehicle) {
    std::unique_lock<std::mutex> lock(mtx);

    cv_consumers.wait(lock, [this]{ return size > 0 || closed; });

    if (size == 0 && closed) {
        return false;
    }

    vehicle = arr[head];
    head = (head + 1) % QUEUE_SIZE;
    size--;

    cv_producers.notify_one();
    return true;
}

void VehicleQueue::close() {
    std::lock_guard<std::mutex> lock(mtx);
    closed = true;
    cv_consumers.notify_all();
}
