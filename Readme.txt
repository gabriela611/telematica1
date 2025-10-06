===========================================================
                 VEHICLE PROJECT - USAGE INSTRUCTIONS
===========================================================

If you have any trouble with the compilation, contact us.
-----------------------------------------------------------


==========================
      COMPILATION
==========================

We use a Makefile to simplify the compilation and execution process.  
Make sure you are running in a Linux environment (Ubuntu or Kali recommended).

!!! IMPORTANT !!!
Windows is NOT supported for the C server because of socket library differences.


==========================
        COMMANDS
==========================


>>> COMPILE EVERYTHING <<<
-----------------------------------------------------------
Command:
    make all

This will:
 - Compile the C server (server)
 - Compile the Java Admin client
 - Prepare the Python Observer interface


>>> RUN ALL COMPONENTS <<<
-----------------------------------------------------------
Command:
    make run-all

This command will:
 - Start the server in background (it keeps running)
 - Open the Admin interface (Java)
 - Launch the Observer interface (Python)

You will see the vehicle’s telemetry and controls working together.


>>> STOP THE SERVER <<<
-----------------------------------------------------------
Command:
    make stop-server

Important:
The server runs in the background, so you must stop it manually
with this command before running again.

Otherwise, you will get a “port already in use” error.

If you continue with the issue, kill the process manually:
  1. ps aux | grep server
  2. kill -9 XXX   (replace XXX with the process number)


>>> CLEAN EVERYTHING <<<
-----------------------------------------------------------
Command:
    make clean

This removes:
 - Compiled files (server, .class)
 - Log file (logs.txt)
 - PID file (used for the running server)
 - Python virtual environment (.venv)

Use this if you want to rebuild everything from scratch.


>>> ADDITIONAL COMMAND <<<
-----------------------------------------------------------
If it’s your first time running the project or you cloned it on a new computer, run:
    make check

This will:
 - Check if gcc, Java, and Python3 are installed
 - Verify that python3-venv is available
 - Create a virtual environment (.venv) if missing
 - Install customtkinter automatically for the Python GUI


==========================
           TIPS
==========================
 - Run all commands from the main project folder (where the Makefile is located)
 - Use Ctrl + C in the terminal to stop the Admin or Observer interfaces
 - If “make check” shows missing packages, follow the suggested install commands

===========================================================
                  END OF README FILE
===========================================================
