// Autores: Lucía Alvarez, Paula Martínez, Gonzalo Padilla
// Para compilar, en el directorio de este tp ejecutar:
// g++ -std=c++17 *.cpp -pthread -o peaje

#include "estacion.h"

const int NO_THREADS = 4;
const std::string FILE_PATH = "autopista.in";

int main() {
    EstacionPeaje estacion(NO_THREADS);
    estacion.process_vehicles(FILE_PATH);
}