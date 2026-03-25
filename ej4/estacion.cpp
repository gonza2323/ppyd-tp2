// Autores: Lucía Alvarez, Paula Martínez, Gonzalo Padilla
// Para compilar, en el directorio de este tp ejecutar:
// g++ -std=c++17 *.cpp -pthread -o peaje

#include "estacion.h"

#include <fstream>
#include <iostream>
#include <chrono>
#include <algorithm>


void EstacionPeaje::process_vehicles(const std::string &file_path)
{
    std::ifstream file(file_path);

    if (!file.is_open()) {
        std::cerr << "No se pudo abrir el archivo\n";
        return;
    }

    nro_tick = 0;

    std::ofstream("autopista.out", std::ios::trunc).close();

    for (int i = 0; i < no_threads; ++i) {
        threads.emplace_back(&EstacionPeaje::toll_booth_thread_fn, this);
    }

    std::string line;
    while (std::getline(file, line)) {
        Vehicle v = Vehicle::from_str(line);
        queue.enqueue(v);
    }

    queue.close();

    for (auto &t : threads) {
        t.join();
    }

    file.close();
}


void EstacionPeaje::toll_booth_thread_fn()
{
    Vehicle v;

    while (queue.deque(v)) {

        float fee = get_vehicle_toll_fee(v);
        v.money -= fee;

        {
            std::lock_guard<std::mutex> lock(mtx_tickets);
            v.ticket_no = ++nro_tick;
        }

        int seconds = (int)get_vehicle_processing_time(v);
        std::this_thread::sleep_for(std::chrono::seconds(seconds));

        {
            std::lock_guard<std::mutex> lock(mtx_output_file);

            std::ofstream out("autopista.out", std::ios::app);

            std::string money_str = std::to_string(v.money);
            std::replace(money_str.begin(), money_str.end(), '.', ',');

            out << "id:" << v.id << ";cat:";

            switch (v.category) {
                case MOTOCICLETA: out << "motocicleta"; break;
                case COCHE:       out << "coche"; break;
                case UTILITARIO:  out << "utilitario"; break;
                case PESADO:      out << "pesado"; break;
            }

            out << ";din:" << money_str
                << ";tick:" << v.ticket_no
                << "\n";
        }
    }
}
