# =============================
# Proyecto de Vehículo Autónomo
# Compila y ejecuta servidor (C), admin (Java) y observer (Python)
# =============================

CC=gcc
CFLAGS=-Wall -lpthread
PYTHON=python3
PIP=pip3
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
VENV=.venv

# =============================
# COMANDO PRINCIPAL
# =============================

all: $(SERVER) admin observer

# =============================
# COMPILACIÓN
# =============================

$(SERVER): $(SERVER_SRC)
	$(CC) $(SERVER_SRC) -o $(SERVER) $(CFLAGS)

admin: $(ADMIN_SRC)
	$(JAVA) $(ADMIN_SRC)

observer:
	@echo "Observer listo (usa: make run-observer)"

# =============================
# EJECUCIÓN
# =============================

run-server: $(SERVER)
	@echo "🚀 Iniciando servidor..."
	./$(SERVER) $(PORT) $(LOGFILE) & echo $$! > $(PIDFILE)
	@echo "Servidor corriendo en background (PID guardado en $(PIDFILE))"

stop-server:
	@if [ -f $(PIDFILE) ]; then \
		kill -9 `cat $(PIDFILE)` && rm $(PIDFILE) && echo "Servidor detenido."; \
	else \
		echo "No hay PID registrado. ¿El servidor está corriendo?"; \
	fi

run-admin:
	@echo "🧩 Iniciando Admin..."
	$(JAVARUN) $(ADMIN_CLASS)

run-observer: $(VENV)/bin/activate
	@echo "👁 Iniciando Observer..."
	. $(VENV)/bin/activate && $(PYTHON) $(OBSERVER_SRC)

run-all: $(SERVER) admin observer
	@echo "🚀 Iniciando todos los componentes..."
	./$(SERVER) $(PORT) $(LOGFILE) & echo $$! > $(PIDFILE); \
	$(JAVARUN) $(ADMIN_CLASS) & \
	. $(VENV)/bin/activate && $(PYTHON) $(OBSERVER_SRC) & \
	wait

# =============================
# VERIFICACIONES
# =============================

check:
	@echo ""
	@echo "🔍 Verificando entorno de desarrollo..."
	@echo "-----------------------------------------"

	@echo "🧱 Comprobando GCC..."; \
	if ! command -v gcc >/dev/null 2>&1; then \
		echo "❌ GCC no encontrado. Instálalo con: sudo apt install gcc"; exit 1; \
	else echo "✅ GCC disponible."; fi

	@echo "\n☕ Comprobando Java..."; \
	if ! command -v javac >/dev/null 2>&1; then \
		echo "❌ Java no encontrado. Instálalo con: sudo apt install default-jdk"; exit 1; \
	else echo "✅ Java disponible."; fi

	@echo "\n🐍 Comprobando Python3..."; \
	if ! command -v python3 >/dev/null 2>&1; then \
		echo "❌ Python3 no encontrado. Instálalo con: sudo apt install python3"; exit 1; \
	else echo "✅ Python3 disponible."; fi

	@echo "\n📦 Verificando módulo venv..."; \
	if ! dpkg -s python3-venv >/dev/null 2>&1; then \
		echo "⚠️ python3-venv no está instalado. Ejecuta: sudo apt install python3-venv"; exit 1; \
	else echo "✅ Módulo venv disponible."; fi

	@echo "\n🐍 Creando entorno virtual (si no existe)..."; \
	if [ ! -d "$(VENV)" ]; then \
		python3 -m venv $(VENV) || { echo "❌ Error creando entorno virtual"; exit 1; }; \
	fi

	@echo "\n📥 Instalando dependencias de Python..."; \
	. $(VENV)/bin/activate && $(PIP) install --upgrade pip && $(PIP) install customtkinter || { echo "❌ Error instalando dependencias"; exit 1; }

	@echo "\n✅ Entorno de desarrollo verificado correctamente."

# =============================
# LIMPIEZA
# =============================

clean:
	rm -f $(SERVER) $(LOGFILE) *.class $(PIDFILE)
	rm -rf $(VENV)
	@echo "🧹 Limpieza completa."
