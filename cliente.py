import socket
import threading

# Configuración
HOST = "127.0.0.1"   # Cambia si el servidor está en otra máquina
PORT = 8080
ADMIN_PASSWORD = "admin123"  # Contraseña fija para administrador

# Variable de control
is_admin = False

def recibir_datos(sock):
    """Hilo para recibir mensajes del servidor (telemetría y respuestas)."""
    while True:
        try:
            data = sock.recv(1024).decode()
            if not data:
                print("\n[INFO] El servidor se ha cerrado. La conexión fue terminada.")
                break
            print("\n[Servidor]:", data.strip())
        except:
            print("\n[ERROR] Conexión perdida: el servidor dejó de responder.")
            break

def main():
    global is_admin

    # Crear socket TCP
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect((HOST, PORT))
    except Exception as e:
        print(f"[ERROR] No se pudo conectar al servidor: {e}")
        return

    print(f"Conectado al servidor en {HOST}:{PORT}")

    # Autenticación
    rol = input("¿Eres 'admin' o 'observer'? ").strip().lower()
    if rol == "admin":
        password = input("Ingrese la contraseña: ").strip()
        if password == ADMIN_PASSWORD:
            is_admin = True
            print("Autenticación exitosa. Ahora puedes enviar comandos.")
        else:
            print("Contraseña incorrecta. Solo podrás observar telemetría.")

    # Iniciar hilo para recibir telemetría
    thread = threading.Thread(target=recibir_datos, args=(sock,))
    thread.daemon = True
    thread.start()

    # Si es admin, puede mandar comandos
    if is_admin:
        while True:
            cmd = input("ENTER COMMAND: ").strip().upper()
            if cmd == "EXIT":
                print("[INFO] Cerrando conexión con el servidor por decisión del cliente...")
                sock.close()
                break
            elif cmd in ["SPEED UP", "SLOW DOWN", "TURN LEFT", "TURN RIGHT", "LIST USERS"]:
                try:
                    sock.sendall((cmd + "\n").encode())
                except:
                    print("[ERROR] No se pudo enviar el comando: conexión cerrada.")
                    break
            else:
                print("Comando no válido.")
    else:
        # Observador solo espera telemetría
        print("Modo observador: solo recibirás telemetría. Presiona Ctrl+C para salir.")
        thread.join()

if __name__ == "__main__":
    main()
