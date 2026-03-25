// Autores: Lucía Alvarez, Paula Martínez, Gonzalo Padilla
// Para compilar, en el directorio de este tp ejecutar:
// g++ -std=c++17 *.cpp -pthread -o peaje

#include <string>

enum VehicleCategory {
    MOTOCICLETA,
    COCHE,
    UTILITARIO,
    PESADO,
};

struct Vehicle {
    int id;
    VehicleCategory category;
    float money;
    int ticket_no;

    static Vehicle from_str(std::string &str);
};

float get_vehicle_toll_fee(Vehicle& vehicle);
float get_vehicle_processing_time(Vehicle& vehicle);
