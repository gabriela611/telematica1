#make file para compilar y ejecutar el servidor, admin y observer de  una sola vez
#Despues debe colocar make stop-server porque el servidor se ejecuta en background, queda corriendo 
CC=gcc
CFLAGS=-Wall -lpthread
PYTHON=python3
JAVA=javac
JAVARUN=java

SERVER=server
SERVER_SRC=server.c

ADMIN_SRC=AdminClient.java
ADMIN_CLASS=AdminClient

OBSERVER_SRC=Client.py

LOGFILE=logs.txt
PORT=5555
PIDFILE=server.pid

all: $(SERVER) admin observer

$(SERVER): $(SERVER_SRC)
	$(CC) $(SERVER_SRC) -o $(SERVER) $(CFLAGS)

admin: $(ADMIN_SRC)
	$(JAVA) $(ADMIN_SRC)

observer:
	@echo "Observer listo (ejecutar con: make run-observer)"

# Ejecuta el servidor en background y guarda su PID
run-server: $(SERVER)
	./$(SERVER) $(PORT) $(LOGFILE) & echo $$! > $(PIDFILE)
	@echo "Servidor iniciado en background (PID guardado en $(PIDFILE))"

# Detiene el servidor usando el PID guardado
stop-server:
	@if [ -f $(PIDFILE) ]; then \
		kill -9 `cat $(PIDFILE)` && rm $(PIDFILE) && echo "Servidor detenido."; \
	else \
		echo "No hay PID registrado. ¿El servidor está corriendo?"; \
	fi

run-admin: admin
	$(JAVARUN) $(ADMIN_CLASS)

run-observer:
	$(PYTHON) $(OBSERVER_SRC)


run-all: $(SERVER) admin observer
	@echo "Lanzando servidor, admin y observer..."
	./$(SERVER) $(PORT) $(LOGFILE) & echo $$! > $(PIDFILE); \
	$(JAVARUN) $(ADMIN_CLASS) & \
	$(PYTHON) $(OBSERVER_SRC) & \
	wait

clean:
	rm -f $(SERVER) $(LOGFILE) *.class $(PIDFILE)
