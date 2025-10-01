// servidor.c
// gcc servidor.c -o server -lpthread

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int sockfd;  
    struct sockaddr_in addr;
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

FILE *logFile;

// Variables de telemetría
int velocidad = 50;
int bateria = 100;
int temperatura = 25;
char direccion[10] = "NORTH";

void log_message(const char *msg, struct sockaddr_in *addr) {
    time_t now = time(NULL);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    if (addr != NULL) {
        fprintf(logFile, "[%s] (%s:%d) %s\n",
                timebuf, inet_ntoa(addr->sin_addr), ntohs(addr->sin_port), msg);
        printf("[%s] (%s:%d) %s\n",
               timebuf, inet_ntoa(addr->sin_addr), ntohs(addr->sin_port), msg);
    } else {
        fprintf(logFile, "[%s] %s\n", timebuf, msg);
        printf("[%s] %s\n", timebuf, msg);
    }
    fflush(logFile);
}

void send_to_all(char *message) {
    pthread_mutex_lock(&clients_mutex);
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(clients[i]) {
            if(send(clients[i]->sockfd, message, strlen(message), 0) < 0) {
                perror("Error enviando a cliente");
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *telemetry_thread(void *arg) {
    char buffer[BUFFER_SIZE];
    while(1) {
        sleep(10);

        snprintf(buffer, sizeof(buffer),
                 "TELEMETRY SPEED=%d BATTERY=%d TEMP=%d DIR=%s\n",
                 velocidad, bateria, temperatura, direccion);
        send_to_all(buffer);
        log_message(buffer, NULL);

        // Simulación: la batería se descarga
        if (bateria > 0) {
            bateria -= 5;
        } 
        if (bateria < 0) {
            bateria = 0;
            velocidad = 0;
        }
    }
    return NULL;
}

void *client_handler(void *arg) {
    client_t *cli = (client_t *)arg;
    char buffer[BUFFER_SIZE];
    int n;

    // Lista de direcciones en orden horario
    char *direcciones[] = {"NORTH", "EAST", "SOUTH", "WEST"};
    int direccion_idx = 0; // 0 = NORTH inicial

    // Función para actualizar dirección
    void girar(int izquierda) {
        if (izquierda) {
            direccion_idx = (direccion_idx + 3) % 4;  // -1 mod 4
        } else {
            direccion_idx = (direccion_idx + 1) % 4;  // +1 mod 4
        }
        strcpy(direccion, direcciones[direccion_idx]);
    }

    while((n = recv(cli->sockfd, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[n] = '\0';
        log_message(buffer, &cli->addr);

        if (bateria <= 10) {
            send(cli->sockfd, "LOW BATTERY. COMMAND NOT EXECUTED\n", strlen("LOW BATTERY. COMMAND NOT EXECUTED\n"), 0);
        }
        else if (strncmp(buffer, "SPEED UP", 8) == 0) {
            if (velocidad < 100){
                velocidad += 10;
                send(cli->sockfd, "ACK SPEED UP\n", strlen("ACK SPEED UP\n"), 0);
            }
            if (velocidad >= 100){
                send(cli->sockfd, "SPEED LIMIT REACHED. COMMAND NOT EXECUTED\n", strlen("SPEED LIMIT REACHED. COMMAND NOT EXECUTED\n"), 0);
            }
        } else if (strncmp(buffer, "SLOW DOWN", 9) == 0) {
            velocidad -= 10;
            send(cli->sockfd, "ACK SLOW DOWN\n", strlen("ACK SLOW DOWN\n"), 0);
        } else if (strncmp(buffer, "TURN LEFT", 9) == 0) {
            girar(1);
            send(cli->sockfd, "ACK TURN LEFT\n", strlen("ACK TURN LEFT\n"), 0);
        } else if (strncmp(buffer, "TURN RIGHT", 10) == 0) {
            girar(0);
            send(cli->sockfd, "ACK TURN RIGHT\n", strlen("ACK TURN RIGHT\n"), 0);
        } else if (strncmp(buffer, "LIST USERS", 10) == 0) {
            char userlist[BUFFER_SIZE];
            strcpy(userlist, "USERS CONNECTED:\n");

            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i]) {
                    char entry[128];
                    snprintf(entry, sizeof(entry), " - %s:%d\n",
                             inet_ntoa(clients[i]->addr.sin_addr),
                             ntohs(clients[i]->addr.sin_port));
                    strcat(userlist, entry);
                }
            }
            pthread_mutex_unlock(&clients_mutex);

            send(cli->sockfd, userlist, strlen(userlist), 0);
        } else {
            send(cli->sockfd, "ERR UNKNOWN CMD\n", strlen("ERR UNKNOWN CMD\n"), 0);
        }
    }

    close(cli->sockfd);
    pthread_mutex_lock(&clients_mutex);
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(clients[i] == cli) {
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    free(cli);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if(argc != 3) {
        printf("Uso: %s <port> <logFile>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);

    logFile = fopen(argv[2], "a");
    if(!logFile) {
        perror("No se pudo abrir archivo de logs");
        exit(1);
    }

    int server_fd, new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0) {
        perror("Error creando socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);  // 🔹 puerto desde argv

    if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        exit(1);
    }

    if(listen(server_fd, 5) < 0) {
        perror("Error en listen");
        exit(1);
    }

    char msg[64];
    snprintf(msg, sizeof(msg), "Servidor iniciado en puerto %d...", port);
    log_message(msg, NULL);

    pthread_t tid;
    pthread_create(&tid, NULL, &telemetry_thread, NULL);

    while(1) {
        new_sock = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if(new_sock < 0) {
            perror("Error en accept");
            continue;
        }

        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->sockfd = new_sock;
        cli->addr = client_addr;

        pthread_mutex_lock(&clients_mutex);
        int added = 0;
        for(int i=0; i<MAX_CLIENTS; i++) {
            if(!clients[i]) {
                clients[i] = cli;
                added = 1;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        if(!added) {
            log_message("SERVER FULL. CLIENT NOT CONNECTED", &client_addr);
            close(new_sock);
            free(cli);
            continue;
        }

        pthread_t client_tid;
        pthread_create(&client_tid, NULL, &client_handler, (void*)cli);
        pthread_detach(client_tid);

        log_message("NEW CLIENT CONNECTED", &client_addr);
    }

    fclose(logFile);
    close(server_fd);
    return 0;
}
