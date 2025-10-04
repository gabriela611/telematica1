

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> //ignore, run in linux not in windows
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100
#define ADMIN_PASS "admin123"
#define MIN_SPEED 0
#define MAX_SPEED 120

//estructuras

typedef struct {
    int sockfd;
    struct sockaddr_in addr;
    int isAdmin;
    char username[32];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

FILE *logFile;

// Admin persistent token (memoria)
char admin_token[129] = "";

// Estado del vehículo
int speed = 50;
int battery = 100;
int temperature = 30;
char direction[16] = "FORWARD";

// -----Utilidades-----

// escribir en archivo de logs y en consola
void write_log(const char *msg) {
    time_t now = time(NULL);
    char tbuf[64];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(logFile, "[%s] %s\n", tbuf, msg);
    fflush(logFile);
    printf("[%s] %s\n", tbuf, msg);
}

// enviar a un cliente
void send_to_client(Client *cli, const char *msg) {
    if (cli == NULL) return;
    if (cli->sockfd <= 0) return;
    send(cli->sockfd, msg, strlen(msg), 0);
}

// enviar a todos
void broadcast(const char *msg) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        send_to_client(&clients[i], msg);
    }
    pthread_mutex_unlock(&clients_mutex);
}

// remover cliente desconectado
void remove_client(int sockfd) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].sockfd == sockfd) {
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// - Telemetria -

void *telemetry_thread(void *arg) {
    while (1) {
        sleep(10);

        char buffer[BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer),
                 "TLM speed=%d;battery=%d;temp=%d;dir=%s;ts=%ld\n",
                 speed, battery, temperature, direction, time(NULL));

        broadcast(buffer);
        write_log(buffer);

        //bateria y temperatura dependen de velocidad
        int consumo = 1 + (speed / 20);
        battery -= consumo;
        if (battery < 0) battery = 0;

        if (battery == 0) {
            if (speed > 0) {
                speed -= 10;
                if (speed < 0) speed = 0;
            }
            if (temperature > 25) {
                temperature -= 2;
            }
        } else {
            if (speed > 100) {
                temperature += 3 + (rand() % 2);
            } else if (speed > 80) {
                temperature += 2 + (rand() % 2);
            } else if (speed > 60) {
                temperature += 1;
            } else if (speed >= 40) {
                int cambio = (rand() % 3) - 1;
                temperature += cambio;
            } else if (speed > 0) {
                temperature -= 1;
            } else {
                if (temperature > 25) temperature -= 2;
                else if (temperature < 25) temperature += 1;
            }
        }

        if (temperature < 20) temperature = 20;
        if (temperature > 100) temperature = 100;
    }
    return NULL;
}

// - manejo de clientes -

void *client_handler(void *arg) {
    Client *cli = (Client *)arg;
    char buffer[BUFFER_SIZE];

    // autenticacion inicial
    int n = recv(cli->sockfd, buffer, BUFFER_SIZE - 1, 0);
    if (n <= 0) {
        close(cli->sockfd);
        remove_client(cli->sockfd);
        return NULL;
    }
    buffer[n] = '\0';

    if (strncmp(buffer, "LOGIN ADMIN", 11) == 0) {
        char pass[64];
        sscanf(buffer, "LOGIN ADMIN %63s", pass);
        if (strcmp(pass, ADMIN_PASS) == 0) {
            // genera un token simple y lo envia al admin para permanencia 
            unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)rand();
            char token[129];
            snprintf(token, sizeof(token), "%08x%08x", seed, (unsigned int)rand());
            strncpy(admin_token, token, sizeof(admin_token)-1);
            admin_token[sizeof(admin_token)-1] = '\0';

            cli->isAdmin = 1;
            send_to_client(cli, "OK Admin logged in\n");

            char token_msg[256];
            snprintf(token_msg, sizeof(token_msg), "ADMIN_TOKEN %s\n", admin_token);
            send_to_client(cli, token_msg);

            write_log("Admin conectado (password)");
        } else {
            send_to_client(cli, "ERROR Wrong password\n");
            close(cli->sockfd);
            remove_client(cli->sockfd);
            return NULL;
        }
    } else if (strncmp(buffer, "LOGIN ADMINTOKEN", 16) == 0) {
        char token[129];
        sscanf(buffer, "LOGIN ADMINTOKEN %128s", token);
        if (strlen(admin_token) > 0 && strcmp(token, admin_token) == 0) {
            cli->isAdmin = 1;
            send_to_client(cli, "OK Admin logged in (by token)\n");
            write_log("Admin conectado (token)");
        } else {
            send_to_client(cli, "ERROR Invalid admin token\n");
            close(cli->sockfd);
            remove_client(cli->sockfd);
            return NULL;
        }
    } else if (strncmp(buffer, "LOGIN OBSERVER", 14) == 0) {
        cli->isAdmin = 0;
        sscanf(buffer, "LOGIN OBSERVER %31s", cli->username);
        send_to_client(cli, "OK Observer logged in\n");
        write_log("Observer conectado");
    } else {
        send_to_client(cli, "ERROR Invalid login\n");
        close(cli->sockfd);
        remove_client(cli->sockfd);
        return NULL;
    }

    // escuchar comandos
    while ((n = recv(cli->sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[n] = '\0';

        char logbuf[BUFFER_SIZE];
        snprintf(logbuf, sizeof(logbuf),
                 "Mensaje de %s:%d -> %s",
                 inet_ntoa(cli->addr.sin_addr),
                 ntohs(cli->addr.sin_port),
                 buffer);
        write_log(logbuf);

        if (cli->isAdmin) {
            if (strncmp(buffer, "CMD SPEED UP", 12) == 0) {
                if (battery < 10) send_to_client(cli, "ERROR BATTERY LOW\n");
                else if (speed >= MAX_SPEED) send_to_client(cli, "ERROR MAX SPEED\n");
                else { speed += 10; send_to_client(cli, "OK CMD EXECUTED\n"); }
            } else if (strncmp(buffer, "CMD SLOW DOWN", 13) == 0) {
                if (speed <= MIN_SPEED) send_to_client(cli, "ERROR MIN SPEED\n");
                else { speed -= 10; send_to_client(cli, "OK CMD EXECUTED\n"); }
            } else if (strncmp(buffer, "CMD TURN LEFT", 13) == 0) {
                if (battery < 10) send_to_client(cli, "ERROR BATTERY LOW\n");
                else { strncpy(direction, "LEFT", sizeof(direction)-1); send_to_client(cli, "OK CMD EXECUTED\n"); }
            } else if (strncmp(buffer, "CMD TURN RIGHT", 14) == 0) {
                if (battery < 10) send_to_client(cli, "ERROR BATTERY LOW\n");
                else { strncpy(direction, "RIGHT", sizeof(direction)-1); send_to_client(cli, "OK CMD EXECUTED\n"); }
            } else {
                send_to_client(cli, "ERROR Unknown command\n");
            }
        } else {
            send_to_client(cli, "ERROR Not authorized\n");
        }
    }

    close(cli->sockfd);
    remove_client(cli->sockfd);
    return NULL;
}

// - main -

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <puerto> <archivoLogs>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    logFile = fopen(argv[2], "a");
    if (!logFile) {
        perror("Error abriendo archivo de logs");
        exit(EXIT_FAILURE);
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Error creando socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Error en listen");
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en puerto %d...\n", port);
    write_log("Servidor iniciado");

    pthread_t tid;
    pthread_create(&tid, NULL, telemetry_thread, NULL);

    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Error en accept");
            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        clients[client_count].sockfd = client_fd;
        clients[client_count].addr = client_addr;
        clients[client_count].isAdmin = 0;

        pthread_t t;
        pthread_create(&t, NULL, client_handler, &clients[client_count]);
        pthread_detach(t);

        client_count++;
        pthread_mutex_unlock(&clients_mutex);
    }

    close(server_fd);
    fclose(logFile);
    return 0;
}
