// Autores: Lucía Alvarez, Paula Martínez, Gonzalo Padilla
// Para compilar, en el directorio de este tp ejecutar:
// g++ -std=c++17 *.cpp -pthread -o peaje

#include "queue.h"
#include <vector>
#include <thread>

class EstacionPeaje
{
public:
    EstacionPeaje(int no_threads) : no_threads(no_threads) {}
    void process_vehicles(const std::string &file_path);

private:
    int no_threads;
    int nro_tick;
    VehicleQueue queue;
    std::vector<std::thread> threads;
    std::mutex mtx_tickets;
    std::mutex mtx_output_file;

    void toll_booth_thread_fn();
};
