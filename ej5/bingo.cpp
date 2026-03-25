// Autores: Lucía Alvarez, Paula Martínez, Gonzalo Padilla

//  - La BANCA corre en el hilo principal (main).
//  - Cada JUGADOR corre en su propio std::thread.
//  - Un mutex global protege la sección crítica donde
//    un jugador anuncia un premio (evita que dos jugadores
//    griten al mismo tiempo y que se dupliquen premios).
//  - Una variable atómica 'partida_terminada' señaliza a
//    todos los hilos que alguien completó el cartón.
// Premios intermedios:
//  - Ambo    : 2 aciertos en la misma fila
//  - Terna   : 3 aciertos en la misma fila
//  - Cuaterna: 4 aciertos en la misma fila
//  - Quintina: 5 aciertos en la misma fila
//  - Cartón lleno: todos los 15 números marcados
// Compilación:
//  g++ -std=c++11 bingo.cpp -o bingo -pthread
// Ejecución:
//  ./bingo

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <array>
#include <algorithm>
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <queue>

// CONSTANTES
static const int TOTAL_BOLILLAS = 90;
static const int FILAS = 3;
static const int COLUMNAS = 9;
static const int NUMEROS_POR_CARTON = 15;

//SINCRONIZACIÓN
std::mutex mtx_bolillas;
std::mutex mtx_eventos;
std::mutex mtx_cout;

std::atomic<bool> partida_terminada(false);
std::vector<int> bolillas_extraidas;

//LOG
void log(const std::string &msg) {
    std::lock_guard<std::mutex> lock(mtx_cout);
    std::cout << msg << std::endl;
}

//EVENTOS
enum TipoEvento { AMBO = 2, TERNA = 3, CUATERNA = 4, QUINTINA = 5, CARTON = 15 };

struct Evento {
    std::string jugador;
    int carton_id;
    int fila;
    TipoEvento tipo;
};

std::queue<Evento> cola_eventos;

// PREMIOS
struct Premios {
    bool ambo = false;
    bool terna = false;
    bool cuaterna = false;
    bool quintina = false;
} premios;

//CARTON
class Carton {
public:
    std::array<std::array<int, COLUMNAS>, FILAS> grilla{};
    std::array<std::array<bool, COLUMNAS>, FILAS> marcado{};

    Carton() {
        for (auto &f : grilla) f.fill(0);
        for (auto &f : marcado) f.fill(false);
        generar();
    }

    void generar() {
        std::mt19937 rng(std::random_device{}());
        std::array<std::vector<int>, COLUMNAS> pool;

        for (int c = 0; c < COLUMNAS; ++c) {
            int lo = c * 10 + 1;
            int hi = (c == 8) ? 90 : (c + 1) * 10;
            for (int n = lo; n <= hi; ++n)
                pool[c].push_back(n);
        }

        for (int f = 0; f < FILAS; ++f) {
            std::vector<int> cols = {0,1,2,3,4,5,6,7,8};
            std::shuffle(cols.begin(), cols.end(), rng);

            for (int i = 0; i < 5; ++i) {
                int c = cols[i];
                std::uniform_int_distribution<int> dist(0, pool[c].size()-1);
                int idx = dist(rng);
                grilla[f][c] = pool[c][idx];
                pool[c].erase(pool[c].begin() + idx);
            }
        }
    }

    bool marcar(int num) {
        for (int f = 0; f < FILAS; ++f)
            for (int c = 0; c < COLUMNAS; ++c)
                if (grilla[f][c] == num) {
                    marcado[f][c] = true;
                    return true;
                }
        return false;
    }

    int marcados_en_fila(int f) const {
        int cnt = 0;
        for (int c = 0; c < COLUMNAS; ++c)
            if (grilla[f][c] != 0 && marcado[f][c]) cnt++;
        return cnt;
    }

    int total_marcados() const {
        int cnt = 0;
        for (int f = 0; f < FILAS; ++f)
            for (int c = 0; c < COLUMNAS; ++c)
                if (grilla[f][c] != 0 && marcado[f][c]) cnt++;
        return cnt;
    }

    bool carton_lleno() const {
        return total_marcados() == NUMEROS_POR_CARTON;
    }
};

//JUGADOR
class Jugador {
public:
    std::string nombre;
    int dinero;
    std::vector<Carton> cartones;

    Jugador(std::string n, int dinero_inicial) : nombre(n), dinero(dinero_inicial) {
        int cantidad = dinero / 100;
        for (int i = 0; i < cantidad; ++i)
            cartones.emplace_back();
    }

    void jugar() {
        int ultima = 0;

        while (!partida_terminada) {
            std::vector<int> nuevas;

            {
                std::lock_guard<std::mutex> lock(mtx_bolillas);
                while (ultima < bolillas_extraidas.size()) {
                    nuevas.push_back(bolillas_extraidas[ultima++]);
                }
            }

            for (int num : nuevas) {
                for (int i = 0; i < cartones.size(); ++i) {
                    Carton &ct = cartones[i];
                    ct.marcar(num);

                    for (int f = 0; f < FILAS; ++f) {
                        int m = ct.marcados_en_fila(f);

                        if (m >= 2) enviar_evento(i, f, AMBO);
                        if (m >= 3) enviar_evento(i, f, TERNA);
                        if (m >= 4) enviar_evento(i, f, CUATERNA);
                        if (m >= 5) enviar_evento(i, f, QUINTINA);
                    }

                    if (ct.carton_lleno()) {
                        enviar_evento(i, 0, CARTON);
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    void enviar_evento(int carton_id, int fila, TipoEvento tipo) {
        std::lock_guard<std::mutex> lock(mtx_eventos);
        cola_eventos.push({nombre, carton_id, fila, tipo});
    }
};

//BANCA
bool validar_evento(const Evento &e, const std::vector<Jugador> &jugadores) {
    for (const auto &j : jugadores) {
        if (j.nombre == e.jugador) {
            const Carton &ct = j.cartones[e.carton_id];

            if (e.tipo == CARTON)
                return ct.carton_lleno();

            return ct.marcados_en_fila(e.fila) >= e.tipo;
        }
    }
    return false;
}

void procesar_eventos(std::vector<Jugador> &jugadores) {
    std::lock_guard<std::mutex> lock(mtx_eventos);

    while (!cola_eventos.empty()) {
        Evento e = cola_eventos.front();
        cola_eventos.pop();

        if (!validar_evento(e, jugadores)) continue;

        if (e.tipo == AMBO && !premios.ambo) {
            premios.ambo = true;
            log("AMBO → " + e.jugador);
        }
        else if (e.tipo == TERNA && !premios.terna) {
            premios.terna = true;
            log("TERNA → " + e.jugador);
        }
        else if (e.tipo == CUATERNA && !premios.cuaterna) {
            premios.cuaterna = true;
            log("CUATERNA → " + e.jugador);
        }
        else if (e.tipo == QUINTINA && !premios.quintina) {
            premios.quintina = true;
            log("QUINTINA → " + e.jugador);
        }
        else if (e.tipo == CARTON && !partida_terminada) {
            partida_terminada = true;
            log("\nGANADOR → " + e.jugador + "\n");
        }
    }
}

void banca(std::vector<Jugador> &jugadores) {
    std::vector<int> bolsa;
    for (int i = 1; i <= 90; i++) bolsa.push_back(i);

    std::shuffle(bolsa.begin(), bolsa.end(), std::mt19937(std::random_device{}()));

    for (int i = 0; i < bolsa.size() && !partida_terminada; i++) {
        int num = bolsa[i];

        {
            std::lock_guard<std::mutex> lock(mtx_bolillas);
            bolillas_extraidas.push_back(num);
        }

        log("Bolilla → " + std::to_string(num));

        procesar_eventos(jugadores);

        std::this_thread::sleep_for(std::chrono::milliseconds(120));
    }
}

//MAIN
int main() {
    std::vector<Jugador> jugadores = {
        Jugador("Ana", 300),
        Jugador("Bruno", 100),
        Jugador("Carla", 200),
        Jugador("Diego", 100)
    };

    std::vector<std::thread> hilos;

    for (auto &j : jugadores)
        hilos.emplace_back(&Jugador::jugar, &j);

    banca(jugadores);

    for (auto &h : hilos)
        h.join();

    return 0;
}
