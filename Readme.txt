# 🚗 Vehicle Project - Usage Instructions

If you encounter any compilation issues, please contact us.

---

## 🛠️ Compilation

We use a **Makefile** to simplify compilation and execution.  
Ensure you're running in a **Linux environment** (Ubuntu or Kali recommended).

> ⚠️ **Important:** Windows is **not supported** for the C server due to socket library differences.

---

## 📋 Commands

### ▶️ Compile Everything
```bash
make all

This will:

Compile the C server (server)

Compile the Java Admin client

Prepare the Python Observer interface

🚀 Run All Components
```bash
make run-all

This command will:

Start the server in the background (it keeps running)

Open the Admin interface (Java)

Launch the Observer interface (Python)

You will see the vehicle’s telemetry and controls working together.

🛑 Stop the Server
```bash
make stop-server

Important:
The server runs in the background, so you must stop it manually before running it again.
Otherwise, you’ll get a “port already in use” error.

If the issue persists, kill the process manually:

```bash
ps aux | grep server
kill -9 XXX   # Replace XXX with the process number

🧹 Clean Everything
```bash
make clean

This removes:
Compiled files (server, .class)
Log file (logs.txt)
PID file (used for the running server)
Python virtual environment (.venv)

Use this if you want to rebuild everything from scratch.

💡 Additional Command
If it’s your first time running the project or you’ve cloned it on a new computer, run:

```bash
make check

This will:

Check if gcc, Java, and Python3 are installed
Verify that python3-venv is available
Create a virtual environment (.venv) if missing
Install customtkinter automatically for the Python GUI

🧠 Tips
Run all commands from the main project folder (where the Makefile is located)
Use Ctrl + C in the terminal to stop the Admin or Observer interfaces
If make check shows missing packages, follow the suggested install commands
