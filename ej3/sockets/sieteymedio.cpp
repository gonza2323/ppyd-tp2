// Autores: Lucía Alvarez, Paula Martínez, Gonzalo Padilla
// compilación= ./sieteymedio N 

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define HOST "127.0.0.1"

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
    int socket_fd = -1; // servidor guarda el socket de cada jugador
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
        case 1:  return "As";
        case 8:  return "Sota";
        case 9:  return "Caballo";
        case 10: return "Rey";
        default: return std::to_string(carta);
    }
}

// --- Jugador ---
void proceso_jugador(int id) {
    srand(getpid());
    // Crear socket para conectarse al servidor
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[Jugador " << id << "] Error al crear socket" << std::endl;
        return;
    }
    // Configurar la dirección del servidor
    struct sockaddr_in dir_servidor;
    dir_servidor.sin_family = AF_INET;
    dir_servidor.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST, &dir_servidor.sin_addr);
    // Intentar conectar
    int intentos = 0;
    while (connect(sock, (struct sockaddr*)&dir_servidor, sizeof(dir_servidor)) < 0) {
        intentos++;
        if (intentos >= 10) {
            std::cerr << "[Jugador " << id << "] No pude conectarme al servidor" << std::endl;
            close(sock);
            return;
        }
        usleep(100000);
    }
    std::cout << "[Jugador " << id << "] Conectado al servidor." << std::endl;
    double puntos = 0;

    while (true) {
        // Recibir carta 
        int carta;
        read(sock, &carta, sizeof(carta));

        double valor = valor_carta(carta);
        puntos += valor;

        std::cout << "[Jugador " << id << "] Recibi carta "
                  << nombre_carta(carta) << " (vale " << valor
                  << "). Total: " << puntos << std::endl;

        Respuesta resp;
        resp.puntos = puntos;

        if (puntos > 7.5) { // Se pasó
            resp.decision = SE_PASO;
            write(sock, &resp, sizeof(resp));
            std::cout << "[Jugador " << id << "] Me pase. Abandono." << std::endl;
            break;
        }
        //Lógica usada: 40% de probabilidad de plantarse, 60% de pedir carta
        if ((rand() % 10) < 4) {
            resp.decision = SE_PLANTA;
            write(sock, &resp, sizeof(resp));
            std::cout << "[Jugador " << id << "] Me planto con "
                      << puntos << " puntos." << std::endl;
            break;
        } else {
            resp.decision = PIDE_CARTA;
            write(sock, &resp, sizeof(resp));
        }
    }

    close(sock);
}

// --- Servidor ---
void proceso_servidor(int N, std::vector<Jugador>& jugadores) {
    // Crear el socket del servidor
    int servidor_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(servidor_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // Configurar dirección
    struct sockaddr_in dir;
    dir.sin_family = AF_INET;
    dir.sin_addr.s_addr = INADDR_ANY;
    dir.sin_port = htons(PORT);
    bind(servidor_fd, (struct sockaddr*)&dir, sizeof(dir));
    // Empezar a escuchar
    listen(servidor_fd, N);
    std::cout << "[Servidor] Escuchando en puerto " << PORT << std::endl;
    // Aceptar la conexión de cada jugador y guardar su socket
    for (int i = 0; i < N; i++) {
        struct sockaddr_in dir_cliente;
        socklen_t tam = sizeof(dir_cliente);
        int socket_jugador = accept(servidor_fd, (struct sockaddr*)&dir_cliente, &tam);
        jugadores[i].socket_fd = socket_jugador;
        std::cout << "[Servidor] Jugador " << i << " conectado." << std::endl;
    }
    srand(time(NULL));

    while (true) {
        bool hay_activos = false;
        for (auto& j : jugadores)
            if (j.activo) { hay_activos = true; break; }
        if (!hay_activos) break;

        // Repartir una carta a cada jugador activo
        for (auto& j : jugadores) {
            if (!j.activo) continue;
            int carta = repartir_carta();
            write(j.socket_fd, &carta, sizeof(carta));
        }
        // Leer respuesta de cada jugador activo
        for (auto& j : jugadores) {
            if (!j.activo) continue;
            Respuesta resp;
            read(j.socket_fd, &resp, sizeof(resp));
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
    // Cerrar todos los sockets de jugadores
    for (auto& j : jugadores) close(j.socket_fd);
    close(servidor_fd);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Uso: ./sieteymedio_sockets <N>" << std::endl;
        return 1;
    }
    int N = atoi(argv[1]);

    std::vector<Jugador> jugadores(N);

    // Crear los N procesos hijo antes de que el servidor empiece a escuchar
    for (int i = 0; i < N; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            proceso_jugador(i);
            return 0;
        }
        jugadores[i].pid = pid;
    }
    proceso_servidor(N, jugadores);
    for (int i = 0; i < N; i++) wait(NULL);

    // Ver quien gana
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