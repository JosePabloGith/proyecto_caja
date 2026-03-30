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

# Regla principal
all: directory $(EXEC)

# Crear estructura de carpetas si no existe
directory:
	@mkdir -p bin

# Compilar el ejecutable
$(EXEC): $(SRC)
	$(CC) $(SRC) -o $(EXEC) $(CFLAGS) $(LDFLAGS)

# Limpiar archivos generados
clean:
	rm -rf bin

# Ejecutar el programa
run: all
	./$(EXEC)

# Ayuda
help:
	@echo "Opciones del Makefile:"
	@echo "  make       - Compila el proyecto"
	@echo "  make run   - Compila y ejecuta inmediatamente"
	@echo "  make clean - Borra el ejecutable generado"
