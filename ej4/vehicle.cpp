// Autores: Lucía Alvarez, Paula Martínez, Gonzalo Padilla
// Para compilar, en el directorio de este tp ejecutar:
// g++ -std=c++17 *.cpp -pthread -o peaje

#include "vehicle.h"
#include <sstream>
#include <algorithm>

float get_vehicle_toll_fee(Vehicle &vehicle)
{
    switch (vehicle.category)
    {
    case MOTOCICLETA: return 1.00;
    case COCHE:       return 2.50;
    case UTILITARIO:  return 3.25;
    case PESADO:      return 5.00;
    }
    return 0.0;
}

float get_vehicle_processing_time(Vehicle &vehicle)
{
    switch (vehicle.category)
    {
    case MOTOCICLETA: return 1.0;
    case COCHE:       return 2.0;
    case UTILITARIO:  return 3.0;
    case PESADO:      return 4.0;
    }
    return 0.0;
}

Vehicle Vehicle::from_str(std::string &str)
{
    Vehicle v{};

    std::string token;
    std::stringstream ss(str);

    while (std::getline(ss, token, ';')) {
        size_t pos = token.find(':');
        if (pos == std::string::npos) continue;

        std::string key = token.substr(0, pos);
        std::string value = token.substr(pos + 1);

        key.erase(0, key.find_first_not_of(" "));
        value.erase(0, value.find_first_not_of(" "));

        if (key == "id") {
            v.id = std::stoi(value);
        }
        else if (key == "cat") {
            if (value == "motocicleta") v.category = MOTOCICLETA;
            else if (value == "coche") v.category = COCHE;
            else if (value == "utilitario") v.category = UTILITARIO;
            else if (value == "pesado") v.category = PESADO;
        }
        else if (key == "din") {
            std::replace(value.begin(), value.end(), ',', '.');
            v.money = std::stof(value);
        }
    }

    return v;
}