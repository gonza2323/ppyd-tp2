// Autores: Lucía Alvarez, Paula Martínez, Gonzalo Padilla
// compilación= ./sieteymedio N 

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/wait.h>

enum Decision { PIDE_CARTA = 0, SE_PLANTA = 1, SE_PASO = 2 };

//jugador manda al servidor su ultima respuesta
struct Respuesta {
    Decision decision;
    double puntos;
};

struct Jugador {
    pid_t pid;
    double puntos = 0;
    bool plantado = false;
    bool activo = true;
    int pipe_recibe[2];
    int pipe_envia[2]; 
};

double valor_carta(int carta) {
    if (carta <= 7) return (double)carta;
    return 0.5; //sota=8, caballo=9, rey=10 -> valen 0.5
}

int repartir_carta() {
    return (rand() % 10) + 1;
}

std::string nombre_carta(int carta) {
    switch(carta) {
        case 1: return "As";
        case 8: return "Sota";
        case 9: return "Caballo";
        case 10: return "Rey";
        default: return std::to_string(carta);
    }
}

// --- Jugador ---
void proceso_jugador(int id, int fd_leer, int fd_escribir) {
    double puntos = 0;
    srand(getpid());

    while (true) {
        // Recibir carta 
        int carta;
        read(fd_leer, &carta, sizeof(carta));
        double valor = valor_carta(carta);
        puntos += valor;

        std::cout << "[Jugador " << id << "] Recibi carta "
                  << nombre_carta(carta) << " (vale " << valor
                  << "). Total: " << puntos << std::endl;
        Respuesta resp;
        resp.puntos = puntos;

        if (puntos > 7.5) { // Se pasó
            resp.decision = SE_PASO;
            write(fd_escribir, &resp, sizeof(resp));
            std::cout << "[Jugador " << id << "] Me pase. Abandono." << std::endl;
            break;
        }

        //Lógica usada: 40% de probabilidad de plantarse, 60% de pedir carta
        if ((rand() % 10) < 4) {
            resp.decision = SE_PLANTA;
            write(fd_escribir, &resp, sizeof(resp));
            std::cout << "[Jugador " << id << "] Me planto con "
                      << puntos << " puntos." << std::endl;
            break;
        } else {
            resp.decision = PIDE_CARTA;
            write(fd_escribir, &resp, sizeof(resp));
        }
    }
    close(fd_leer);
    close(fd_escribir);
}

// --- Servidor ---
void proceso_servidor(int N, std::vector<Jugador>& jugadores) {
    srand(time(NULL));
    for (auto& j : jugadores) {
        close(j.pipe_recibe[0]);
        close(j.pipe_envia[1]);
    }
    while (true) {
        bool hay_activos = false;
        for (auto& j : jugadores)
            if (j.activo) { hay_activos = true; break; }
        if (!hay_activos) break;
        // Repartir una carta a cada jugador activo
        for (auto& j : jugadores) {
            if (!j.activo) continue;
            int carta = repartir_carta();
            write(j.pipe_recibe[1], &carta, sizeof(carta));
        }
        // Leer respuesta de cada jugador activo
        for (auto& j : jugadores) {
            if (!j.activo) continue;
            Respuesta resp;
            read(j.pipe_envia[0], &resp, sizeof(resp));
            j.puntos = resp.puntos; // Guardamos los puntos que nos manda el jugador
            if (resp.decision == SE_PLANTA) {
                j.activo = false;
                j.plantado = true;
            } else if (resp.decision == SE_PASO) {
                j.activo = false;
                j.plantado = false;
            }
        }
    }
    // Cerrar pipes del servidor
    for (auto& j : jugadores) {
        close(j.pipe_recibe[1]);
        close(j.pipe_envia[0]);
    }
}

int main(int argc, char* argv[]) {
    int N = atoi(argv[1]);
    std::vector<Jugador> jugadores(N);

    for (int i = 0; i < N; i++) {
        pipe(jugadores[i].pipe_recibe);
        pipe(jugadores[i].pipe_envia);
        pid_t pid = fork();

        if (pid == 0) {
            // Cerrar pipes
            for (int j = 0; j < i; j++) {
                close(jugadores[j].pipe_recibe[0]);
                close(jugadores[j].pipe_recibe[1]);
                close(jugadores[j].pipe_envia[0]);
                close(jugadores[j].pipe_envia[1]);
            }
            // Cerrar extremos
            close(jugadores[i].pipe_recibe[1]);
            close(jugadores[i].pipe_envia[0]);

            proceso_jugador(i, jugadores[i].pipe_recibe[0], jugadores[i].pipe_envia[1]);
            return 0;
        }
        jugadores[i].pid = pid;
    }

    proceso_servidor(N, jugadores);
    for (int i = 0; i < N; i++) wait(NULL);
    // Solo pueden ganar los que se plantaron
    int ganador = -1;
    double mejor_puntaje = -1;

    for (int i = 0; i < N; i++) {
        if (jugadores[i].plantado && jugadores[i].puntos > mejor_puntaje) {
            mejor_puntaje = jugadores[i].puntos;
            ganador = i;
        }
    }

    // --- Fin de partida ---
    std::cout << "\n========== FIN DE LA PARTIDA ==========" << std::endl;
    std::cout << "Jugador | Puntos | Estado" << std::endl;
    std::cout << "--------|--------|--------" << std::endl;
    for (int i = 0; i < N; i++) {
        std::string estado;
        if (jugadores[i].plantado)  estado = "Plantado";
        else                        estado = "Abandono";
        std::cout << "   " << i << "    |  "
                  << jugadores[i].puntos << "   | "
                  << estado << std::endl;
    }

    std::cout << "\n=======================================" << std::endl;
    if (ganador != -1) {
        std::cout << "GANADOR: Jugador " << ganador
                  << " con " << mejor_puntaje << " puntos." << std::endl;
    } else {
        std::cout << "No hay ganador: todos los jugadores abandonaron." << std::endl;
    }
    std::cout << "=======================================" << std::endl;

    return 0;
}