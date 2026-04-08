# Makefile Jose Pablo Perez De Anda
# Proyecto: final

# Compilador
CC = g++

# Banderas de compilación
# -I. para buscar encabezados en la carpeta actual
CFLAGS = -I. -Wall -O2 -Iinclude

# Librerías de enlace (Vital para Linux)
# -lGL: OpenGL
# -lGLU: Utilidades OpenGL
# -lglut: Interfaz GLUT
# -lm: Librería matemática para sin, cos, sqrt
#  Librerias para el sonido:
# -lasound: Para el sistema de audio (ALSA/PipeWire)
# -lpthread: Miniaudio usa hilos para no trabar OpenGL
# -ldl: Necesario para cargar controladores dinámicos
LDFLAGS = -lGL -lGLU -lglut -lm -lasound -lpthread -ldl

# Archivos
SRC = src/main.cpp
EXEC = bin/proyectoFinal

# Flags especificos para debug
# -g : incluye simbolos DWARF (GDB los nececita para debuggear)
#  -O0 : desactiva optimizaciones para facilitar el debug
#  -ggdb3 : incluye simbolos de debug de nivel 3 (máximo detalle)
DEBUG_CFLAGS = -I. -Wall -O0 -g -ggdb -Iinclude



# Regla principal
all: directory $(EXEC)

# Crear estructura de carpetas si no existe
directory:
	@mkdir -p bin

# Compilar el ejecutable
$(EXEC): $(SRC)
	$(CC) $(SRC) -o $(EXEC) $(CFLAGS) $(LDFLAGS)

# ------ Target debug ------
debug: directory
	$(CC) $(SRC) -o $(EXEC) $(DEBUG_CFLAGS) $(LDFLAGS)
	@echo "binario de debug generado en $(EXEC)"

# Limpiar archivos generados
clean:
	rm -rf bin

# Ejecutar el programa
run: all
	./$(EXEC)

# ─── Ejecutar versión debug (útil para probar) ──────────
run-debug: debug
	./$(EXEC)

# Ayuda
help:
	@echo "Opciones del Makefile:"
	@echo "  make           - Compila en modo release"
	@echo "  make debug     - Compila con símbolos para GDB"
	@echo "  make run       - Compila y ejecuta (release)"
	@echo "  make run-debug - Compila y ejecuta (debug)"
	@echo "  make clean     - Borra el ejecutable generado"
