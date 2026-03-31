/*
 * Alumno : Jose pablo Perez De Anda
 * Materia: Graficas Por Computadora
 * Profesor: Georgii Khachaturov
 *
 * Proyecto : proyecto_caja (Caja Enigmática estilo The Room)
 */

/*
 * Control de versiones:
 * 1.- Establecimos el esqueleto inicial del proyecto de graficas por computadora
 * 2.- Agregamos el sistema de cámara con rotación polar
 * 3.- Creamos el plano base (piso) con normales
 * 4.- Sombras 
 * 5.- Limpieza de código y organización
 */

// ============================================================================
// INCLUDES
// ============================================================================
#include <math.h>
#include <stdio.h>    // ← necesario para fopen/fread (cargar BMPs)
#include <stdlib.h>   // ← necesario para malloc/free
#include <time.h>     // <- para una semilla pseudoaleatoria 
#include <GL/glut.h>


//seccion para el audio
#define MINIAUDIO_IMPLEMENTATION
#include "include/miniaudio.h"



// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

//variable global para controlar el sonido
ma_engine motorAudio;
bool audioListo = false; //flag de seguridada


/*
 * IDs de texturas del Skybox:
 * OpenGL maneja las texturas con números enteros (IDs)
 * Necesitamos 6 IDs, uno por cada cara del cubo del skybox
 *
 * skyboxTexturas[0] = derecha  (+X)
 * skyboxTexturas[1] = izquierda(-X)
 * skyboxTexturas[2] = arriba   (+Y)
 * skyboxTexturas[3] = abajo    (-Y)
 * skyboxTexturas[4] = frente   (-Z)
 * skyboxTexturas[5] = atrás    (+Z)
 */
GLuint skyboxTexturas[6];

// Tamaño del cubo del skybox (debe ser mayor que camDist máxima)
float skyboxTamanio = 200.0f;

//textura para la caja
GLuint texturaMaderaCaja = 0; //ID de la textura de la caja (cargada solo una vez)
GLuint texturaTapaMadera = 0 ; //ID de la textura de la tapa
               
//textura para la base (cristaloso :D)
GLuint texturaPisoCristal = 0;

//textura de oro para el toroide
GLuint texturaOro = 0;




// ---------VARIABLES PARA EL PICKING (PUNTO 11) ----------
float velocidadNormal = 1.5f;
float velocidadRapida = 5.0f;

// ------ CÁMARA ------
float camDist = 25.0f;      // Distancia de la cámara al origen
float camAngleX = 0.0f;     // Ángulo horizontal (rotación izquierda-derecha)
float camAngleY = 20.0f;    // Ángulo vertical (rotación arriba-abajo)

/*
 * Variables para el MOUSE:
 * Necesitamos recordar DÓNDE estaba el mouse en el frame anterior
 * para calcular cuánto se movió (el "delta")
 *
 * lastMouseX, lastMouseY → posición del mouse en el último frame
 * mousePresionado        → ¿está el botón izquierdo apretado ahora mismo?
 * mouseDeltaX/Y          → cuántos píxeles se movió desde el último frame
 */
int  lastMouseX     = 0;
int  lastMouseY     = 0;
bool mousePresionado = false;

// ------ LUZ PRINCIPAL ------
/*
 * lightPos[4] = { x, y, z, w }
 * w = 1.0 → luz puntual (tiene posición)
 * w = 0.0 → luz direccional (como el sol)
 */
float lightPos[4] = { 5.0f, 10.0f, 5.0f, 1.0f };


// ------ ANIMACIONES Y ARTICULACIONES ------
//float tapRotation = 0.0f;        // Ángulo de apertura de tapa
//float ruedaRotation = 0.0f;      // Ángulo de rueda giratoria
//float brazoAngulo1 = 0.0f;       // Primer joint del brazo (articulación 1)
//float brazoAngulo2 = 0.0f;       // Segundo joint del brazo (articulación 2)


// ------ FLAGS DE VISUALIZACIÓN (Controles con teclas) ------
bool bCull = false;              // F1: Culling (quitar caras ocultas)
bool bDepth = true;              // F2: Depth test (prueba de profundidad)
bool bWireframe = false;         // F3: Wireframe (modo alambre)
bool bSmooth = false;             // F4: Smooth shading (sombreado suave vs plano) 

// ============================================================================
// VARIABLES PARA CAJA, TAPA Y GEMA
// ============================================================================

// ------ DIMENSIONES DE LA CAJA ------
float cajaAncho = 6.0f;          // Ancho en X
float cajaProfundo = 8.0f;       // Profundidad en Z
float cajaAlto = 5.0f;           // Alto en Y

// ------ TAPA (BISAGRA LATERAL) ------
float tapRotation = 0.0f;        // Ángulo actual de rotación
float tapMaxRotation = 120.0f;   // Ángulo máximo que puede girar
float tapRotationSpeed = 1.5f;   // Grados por frame
bool tapIsOpening = false;       // ¿Está en proceso de abrir?

// ------ GEMA ADENTRO ------
float gemaRadius = 1.65f;         // Radio de la esfera de la gema
float gemaY = 2.4f;              // Altura a la que flota dentro de la caja


// ============================================================================
// PROTOTIPOS DE FUNCIÓN (Forward declarations) para mas comodidad
// ============================================================================

// Funciones de dibujo
void dibujarSkybox();
void dibujarFoco();
void dibujarPiso();
void dibujarCaja();
void dibujarReflejoMadera();
void dibujarTapa();
void dibujarSombraCaja(); //-> nueva

// Funciones principales de openGL y GLUT
void display();
void myReshape(int w, int h);
void procesarSeleccion(int x, int y);

// Callbacks de interaccion
void teclado(unsigned char key, int x, int y);
void tecladoEspecial(int key, int x, int y);
void mouseBoton(int boton, int estado, int x, int y);
void mouseMovimiento(int x, int y);

// Funciones de configuracion y utilidades
void configurarOpenGL();
void cargarTexturasSkybox();
GLuint cargarBMP(const char* ruta);


/*esta funcion nos ayudara a aplicar la matris de sombra
 * Es uno de los ejemplos dados por el profesor
 * en una de sus clases de shadow matrix
 * */
void gltMakeShadowMatrix(GLfloat vPlaneEquation[], GLfloat vLightPos[], GLfloat destMat[]) {
	GLfloat dot;
	dot = vPlaneEquation[0] * vLightPos[0] + vPlaneEquation[1] * vLightPos[1] +
		vPlaneEquation[2] * vLightPos[2] + vPlaneEquation[3] * vLightPos[3];

	destMat[0] = dot - vLightPos[0] * vPlaneEquation[0];
	destMat[4] = 0.0f - vLightPos[0] * vPlaneEquation[1];
	destMat[8] = 0.0f - vLightPos[0] * vPlaneEquation[2];
	destMat[12] = 0.0f - vLightPos[0] * vPlaneEquation[3];

	destMat[1] = 0.0f - vLightPos[1] * vPlaneEquation[0];
	destMat[5] = dot - vLightPos[1] * vPlaneEquation[1];
	destMat[9] = 0.0f - vLightPos[1] * vPlaneEquation[2];
	destMat[13] = 0.0f - vLightPos[1] * vPlaneEquation[3];

	destMat[2] = 0.0f - vLightPos[2] * vPlaneEquation[0];
	destMat[6] = 0.0f - vLightPos[2] * vPlaneEquation[1];
	destMat[10] = dot - vLightPos[2] * vPlaneEquation[2];
	destMat[14] = 0.0f - vLightPos[2] * vPlaneEquation[3];

	destMat[3] = 0.0f - vLightPos[3] * vPlaneEquation[0];
	destMat[7] = 0.0f - vLightPos[3] * vPlaneEquation[1];
	destMat[11] = 0.0f - vLightPos[3] * vPlaneEquation[2];
	destMat[15] = dot - vLightPos[3] * vPlaneEquation[3];
}

/*
 * Nececitamos dibujar un toroide, el cual tendra el objetico de agarrar nuestro diamante 
 *
 */
void dibujarToroide(){

    glPushMatrix();
    
    // Posicionar toroide en el centro, flotando sobre la caja
    glTranslatef(0.0f, 2.0f, 0.0f);

    //rotamos 90 grados sobre x para acostarlo en el plano xz
    //glutSolidTorus dibuja la dona parada xD
    glRotatef(90.0f , 1.0f , 0.0f , 0.0f);

    
    // Textura de oro
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaOro);

    // Configurar GL_TEXTURE_GEN correctamente
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);

    // Usar SPHERE_MAP para proyección realista en superficies curvas
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);


    glColor3f(1.0f, 0.84f, 0.0f);  // Oro
  
    /*
    //aplicamos la textura
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    */


    // radioTubo=0.25, radioAnillo=1.4 — cabe dentro de la caja (ancho/2 = 3)
    glutSolidTorus(0.135f, 1.4f, 20, 72);
	
    
    //deshabilitamos la generacion automatica para no arruinar objetos
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T); 
     
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}


/*
 *Nececitamos hacer un diamante el cual reposara en el toroide
 */
void dibujarGema(){

glPushMatrix();
    
    // Posicionar gema en el centro del toroide
    glTranslatef(0.0f, gemaY, 0.0f);
    
    // =========================================================
    // PARTE 1: PASADA SÓLIDA (Caras de cristal reflectante)
    // =========================================================
    GLfloat gemaEspecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };  // Reflejos blancos
    GLfloat gemaBrillo[] = { 128.0f };  // Máximo brillo
    GLfloat gemaAmbiente[] = { 0.1f, 0.2f, 0.6f, 1.0f };  // Tono base azulado
    
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, gemaAmbiente);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, gemaEspecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, gemaBrillo);
    
    glColor4f(0.2f, 0.5f, 1.0f, 0.85f); // Azul cristalino
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Habilitar la textura de madera con mapeo esférico
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaMaderaCaja); 
    
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
    GLfloat blendColor[] = { 0.2f, 0.5f, 1.0f, 1.0f }; // Teñimos la madera de azul
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, blendColor);

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

    // FIX Z-FIGHTING: Empujamos las caras sólidas ligeramene hacia atrás
    // para que las líneas que dibujaremos después no se corten
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);

    // Creamos el objeto cuádrico (cilindros/conos)
    GLUquadricObj *q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);
    gluQuadricDrawStyle(q, GLU_FILL); // Forzamos dibujado sólido

    // Dimensiones del diamante
    float R  = 1.65f;   // Cintura
    float h1 = 0.5f;    // Altura corona (arriba)
    float h2 = 1.20f;   // Altura pabellón (pico abajo)

    // -- DIBUJAMOS CORONA SÓLIDA --
    glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(q, R, 0.0f, h1, 24, 4); // 24 cortes para que parezca joya
    glPopMatrix();

    // -- DIBUJAMOS PABELLÓN SÓLIDO --
    glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(q, R, 0.0f, h2, 24, 4);
    glPopMatrix();

    // Limpieza de la Parte 1 (apagamos texturas y el offset)
    glDisable(GL_POLYGON_OFFSET_FILL);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_2D);


    // =========================================================
    // PARTE 2: PASADA DE ARISTAS (Líneas brillantes tipo diamante)
    // =========================================================
    
    // Guardamos estado para no afectar otras figuras
    glPushAttrib(GL_LIGHTING_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
    
    glDisable(GL_LIGHTING); // Apagamos luz para que las líneas brillen puras
    glLineWidth(2.0f);      // Hacemos las líneas un poco más gruesas
    glColor4f(0.3f, 0.6f, 1.0f, 0.20f); // Color de borde: Cyan delgado

    // Le decimos a GLU que ahora queremos que el objeto sea de LÍNEAS, no sólido
    gluQuadricDrawStyle(q, GLU_LINE);

    // -- DIBUJAMOS CORONA (Solo líneas) --
    glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(q, R, 0.0f, h1, 24, 4);
    glPopMatrix();

    // -- DIBUJAMOS PABELLÓN (Solo líneas) --
    glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(q, R, 0.0f, h2, 24, 4);
    glPopMatrix();

    glPopAttrib(); // Restauramos la luz, grosor de línea y colores originales

    // Liberamos memoria del cuádrico y apagamos transparencia
    gluDeleteQuadric(q);
    glDisable(GL_BLEND);
    
    glPopMatrix();
}

/*
 *Nececitamos dibujar las patas que sostienen al toroide para que simulen estar cargando
 * el soporte de la gema
 * */
void dibujarPatasToroide(){

// Guardamos el estado actual de la matriz de modelo-vista
    glPushMatrix();
    
    // Posicionar patas acorde con la dona (flotando a 2.0 unidades en Y)
    glTranslatef(0.0f, 2.0f, 0.0f);
    
    // Habilitamos el uso de texturas 2D
    glEnable(GL_TEXTURE_2D);
    // Aplicamos la textura de oro previamente cargada
    glBindTexture(GL_TEXTURE_2D, texturaOro);  

    // Habilitamos la generación automática de coordenadas de textura (S y T)
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    // Usamos mapeo esférico para dar un efecto de reflejo metálico
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

    // Definimos el color base para que la textura adquiera un tono metálico/dorado
    glColor3f(1.0f, 0.84f, 0.0f);  
    
    // Distancia del centro hacia donde se colocarán las patas
    float radio = 1.4f;
    // Matriz con las posiciones X y Z de las 4 patas simétricas
    float posiciones[4][2] = {
        { radio,  0.0f}, // Derecha
        {-radio,  0.0f}, // Izquierda
        { 0.0f,  radio}, // Frente
        { 0.0f, -radio}  // Atrás
    };
    
    // --- OPTIMIZACIÓN DE MEMORIA ---
    // Creamos el objeto cuádrico UNA SOLA VEZ usando memoria dinámica (*puntero)
    GLUquadricObj *pata = gluNewQuadric();
    // Le indicamos que genere normales suaves para que reaccione bien a la luz
    gluQuadricNormals(pata, GLU_SMOOTH);
    
    // Ciclo para dibujar las 4 patas reutilizando el mismo objeto cuádrico
    for (int i = 0; i < 4; i++) {
        // Guardamos la matriz para no acumular las traslaciones de las patas
        glPushMatrix();
            // Movemos el "pincel" a la posición X y Z correspondiente de la matriz
            glTranslatef(posiciones[i][0], 0.0f, posiciones[i][1]); 
            
            // gluCylinder normalmente crece en +Z. 
            // Rotamos 90 grados en X para que crezca hacia abajo (-Y), hacia el suelo
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

            // Dibujamos el cilindro usando el puntero 'pata'
            // Base = 0.12f, Punta = 0.07f, Altura = 2.0f, Segmentos = 12, Pilas = 4
            gluCylinder(pata, 0.12f, 0.07f, 2.0f, 12, 4);
        // Restauramos la matriz para la siguiente pata
        glPopMatrix();
    }
    
    // --- LIMPIEZA DE MEMORIA ---
    // Destruimos el objeto cuádrico UNA SOLA VEZ al terminar el bucle.
    // (Equivalente al ~Destructor o 'delete', previene la fuga de memoria)
    gluDeleteQuadric(pata);
    
    // Deshabilitamos la generación automática de texturas para no afectar otros objetos
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);

    // Apagamos las texturas 2D
    glDisable(GL_TEXTURE_2D);
    
    // Restauramos la matriz original para no afectar el resto de la escena
    glPopMatrix();

}

// ============================================================================
// FUNCIONES PRINCIPALES Y CALLBACKS DE INTERACCION
// ============================================================================

// ============================================================================
// FUNCIÓN: display()
// Función que se llama cada vez que GLUT necesita redibujar la pantalla
// ============================================================================
void display() {
    // 1. LIMPIEZA DE BUFFERS
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glLoadIdentity(); // Reiniciamos la matriz de transformaciones al estado inicial
   
     /*
     * Aplicamos los FLAGS de visualización ANTES de dibujar
     * para que afecten todo lo que se dibuja en este frame
     */
    if (bCull) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);

    if (bDepth) glEnable(GL_DEPTH_TEST);             
    else glDisable(GL_DEPTH_TEST);

    if (bWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (bSmooth) glShadeModel(GL_SMOOTH); 
    else glShadeModel(GL_FLAT);

    // Aquí irá la jerarquía de objetos (caja, tapa, mecanismos, etc.) en el futuro

    // ========================================================================
    // 2. CÁMARA: Posicionamiento del observador (PUNTO 3 de la rúbrica)
    // ========================================================================
    
    /* * La cámara se posiciona usando COORDENADAS ESFÉRICAS:
     * - camDist: radio (distancia del centro)
     * - camAngleX: azimut (giro horizontal)
     * - camAngleY: elevación (giro vertical)
     * * Convertimos de GRADOS a RADIANES (porque sin/cos usan radianes):
     * 1 grado = 0.01745 radianes
     */
    float rX = camAngleX * 0.01745f;  // Conversión a radianes (eje X)
    float rY = camAngleY * 0.01745f;  // Conversión a radianes (eje Y)

    /*
     * Cálculo de posición del "ojo" (cámara) usando TRIGONOMETRÍA ESFÉRICA:
     * - eyeX: movimiento en el plano XZ horizontal
     * - eyeY: movimiento vertical (arriba-abajo)
     * - eyeZ: profundidad (entrada-salida)
     */
    float eyeX = camDist * sin(rX) * cos(rY);
    float eyeY = camDist * sin(rY);
    float eyeZ = camDist * cos(rX) * cos(rY);

    /*
     * gluLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ):
     * - Primer triplete (eye): posición de la cámara en el espacio
     * - Segundo triplete (center): punto hacia donde mira la cámara
     * - Tercer triplete (up): vector que indica qué dirección es "arriba"
     * (0, 1, 0) significa que el eje Y es hacia arriba
     */
    gluLookAt(eyeX, eyeY, eyeZ,      // Posición del ojo (cámara)
              0.0f, 0.0f, 0.0f,      // Mira hacia el origen (centro del mundo)
              0.0f, 1.0f, 0.0f);     // Vector "arriba" (eje Y positivo)
                     
    //**Actualizamos la posicion de la luz cada frame**//
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos); 

     
    // ========================================================================
    // 2.5 ANIMACIÓN DE LA TAPA
    // ========================================================================
    /*
     * Si la tapa está abriendo, incrementamos su ángulo cada frame.
     * Si llegó al máximo, la detenemos.
     */
    if (tapIsOpening) {
        if (tapRotation < tapMaxRotation) {
            tapRotation += tapRotationSpeed;
            glutPostRedisplay(); // <- EL MOTOR: Pide dibujar el SIGUIENTE cuadro
        }
    } else {
        if (tapRotation > 0.0f) {
            tapRotation -= tapRotationSpeed;
            glutPostRedisplay(); // <- EL MOTOR: Pide dibujar el SIGUIENTE cuadro
        }
    }

    // ========================================================================
    // 3. RENDERIZADO DEL ESCENARIO
    // ========================================================================
    // El orden de dibujado importara
    /*
     * ORDEN IMPORTANTÍSIMO:
     * 1° Skybox  → fondo, sin escribir Z-buffer
     * 2° Foco    → sin iluminación (emisivo)
     * 3° Piso    → con iluminación
     * 4° Caja    → con iluminación
     * 5 tapaCaja -> con iluminacion
     */
    dibujarSkybox(); //dibujamos el entorno primero
    dibujarFoco(); //dibuja el foco
    dibujarPiso(); // Dibuja el plano base
		   //
    dibujarSombraCaja(); //para dibujar la sombra de la caja //->importante aqui
    dibujarCaja(); //dibujamos la caja :D
    dibujarTapa(); // dibujamos la tapita de la caja
		   
    dibujarReflejoMadera(); // <-- AQUI INYECTAS LA MAGIA

    dibujarToroide();      // ← dibujamos el toroide
    dibujarPatasToroide(); // ← dibujamos las patas de soporte del toroide


    
    dibujarGema();         // ← dubujamos la gema
    

    // ========================================================================
    // 4. INTERCAMBIO DE BUFFERS
    // ========================================================================
    glutSwapBuffers(); // Muestra la imagen renderizada en pantalla
}

// ============================================================================
// FUNCIÓN: myReshape(int w, int h)
// Se llama automáticamente cada vez que GLUT cambia el tamaño de la ventana
// 
// PROPÓSITO: Ajustar la PROYECCIÓN (cómo se transforma 3D a 2D en pantalla)
// para que nunca se vea distorsionada, sin importar el tamaño de la ventana
// ============================================================================
void myReshape(int w, int h) {
    
    /*
     * glViewport(x, y, width, height):
     * Define el ÁREA DE DIBUJO de la ventana
     * - (0, 0): esquina inferior izquierda
     * - (w, h): nuevo ancho y alto de la ventana
     * * Esto asegura que OpenGL use toda el área de la ventana
     */
    glViewport(0, 0, w, h);

    /*
     * glMatrixMode(GL_PROJECTION):
     * Cambia a la MATRIZ DE PROYECCIÓN
     * Esta matriz define cómo se transforman las coordenadas 3D del mundo
     * a las coordenadas 2D de la pantalla
     * * Tipos de matrices en OpenGL:
     * - GL_PROJECTION: proyección (perspectiva, ortográfica)
     * - GL_MODELVIEW: modelo y vista (transformaciones de objetos y cámara)
     */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity(); // Reinicia la matriz de proyección

    /*
     * gluPerspective(fov, aspect, near, far):
     * Define una PROYECCIÓN EN PERSPECTIVA (como el ojo humano)
     * * Parámetros:
     * - 60.0f: Ángulo de visión vertical en grados (field of view)
     * Un valor mayor = zoom out (ves más), menor = zoom in (ves menos)
     * - (float)w/h: Relación de aspecto (ancho/alto)
     * Si cambias tamaño ventana, esto evita distorsión
     * CRÍTICO: (float) convierte divisiones a punto flotante
     * - 1.0f: Plano CERCANO (near plane) - distancia mínima que ves
     * Objetos más cerca que esto NO se ven
     * - 300.0f: Plano LEJANO (far plane) - distancia máxima que ves
     * Objetos más lejos que esto NO se ven (se quedan negros)
     */
    gluPerspective(60.0f, (float)w / h, 1.0f, 2000.0f);

    /*
     * glMatrixMode(GL_MODELVIEW):
     * Volvemos a la MATRIZ DE MODELO-VISTA
     * Ahora podemos volver a dibujar objetos y aplicarles transformaciones
     */
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================================
// FUNCIÓN: procesarSeleccion(int x, int y)
// Implementa Color Picking para cumplir el Punto 11 de la rúbrica
// ============================================================================
void procesarSeleccion(int x, int y) {
 
    // 1. Apagamos todo lo que altere los colores puros
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);  // Por si acaso
    glDisable(GL_DITHER); // <-- FIX: Apagamos el difuminado de color (Tramado) para el color picking

    // 2. Limpiamos el buffer oculto con un fondo negro (0,0,0)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 3. Posicionamos la cámara igual que en display()
    glLoadIdentity();
    float rX = camAngleX * 0.01745f;
    float rY = camAngleY * 0.01745f;
    float eyeX = camDist * sin(rX) * cos(rY);
    float eyeY = camDist * sin(rY);
    float eyeZ = camDist * cos(rX) * cos(rY);
    gluLookAt(eyeX, eyeY, eyeZ, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    // 4. DIBUJAMOS LOS OBJETOS CON COLORES "CLAVE" (IDs de color)
    
    // --- OBJETO LIBRE (La base de la caja) -> Color VERDE PURO (0, 255, 0)
    glColor3ub(0, 255, 0); 
    glPushMatrix();
        glTranslatef(0.0f, cajaAlto / 2.0f + 0.01f, 0.0f);
        
        // ¡EL FIX GEOMÉTRICO! Dibujamos las 5 paredes huecas en lugar de un cubo sólido.
        // Esto evita que la cámara choque con una "pared invisible" de clic al estar lejos.
        glBegin(GL_QUADS);
            // Frente
            glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f( cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
            // Atrás
            glVertex3f( cajaAncho/2.0f,  cajaAlto/2.0f, cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f, cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, cajaProfundo/2.0f);
            glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f, cajaProfundo/2.0f);
            // Izquierda
            glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f,  cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);
            // Derecha
            glVertex3f(cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f(cajaAncho/2.0f,  cajaAlto/2.0f,  cajaProfundo/2.0f);
            glVertex3f(cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);
            glVertex3f(cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
            // Fondo
            glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);
        glEnd();
    glPopMatrix();

    // --- FRAGMENTO ARTICULADO (La Tapa) -> Color ROJO PURO (255, 0, 0)
    glColor3ub(255, 0, 0);
    glPushMatrix();
        glTranslatef(0.0f, cajaAlto + 0.01f, 0.0f);
        glTranslatef(cajaAncho/2.0f, 0.0f, 0.0f);
        glRotatef(-tapRotation, 0.0f, 0.0f, 1.0f);
        glTranslatef(-cajaAncho/2.0f, 0.0f, 0.0f);
        
        glScalef(cajaAncho, 0.2f, cajaProfundo); // Una tapa simulada plana
        glutSolidCube(1.0f);
    glPopMatrix();

    // --- LA GEMA -> Color AZUL PURO (0, 0, 255) ---
    glColor3ub(0, 0, 255);
    glPushMatrix();
        // Posicionamos exactamente igual que en dibujarGema()
        glTranslatef(0.0f, 2.4f, 0.0f); 
        
        // Dibujamos la geometría básica sin texturas (hitbox perfecto)
        GLUquadricObj *qPick = gluNewQuadric();
        gluQuadricDrawStyle(qPick, GLU_FILL); // Sólido puro
        
        // Corona (Cono superior)
        glPushMatrix();
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            gluCylinder(qPick, 1.65f, 0.0f, 0.5f, 12, 2);
        glPopMatrix();
        
        // Pabellón (Cono inferior)
        glPushMatrix();
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            gluCylinder(qPick, 1.65f, 0.0f, 1.20f, 12, 2);
        glPopMatrix();
        
        gluDeleteQuadric(qPick);
    glPopMatrix();

    // 5. LEER EL PÍXEL DONDE ESTÁ EL MOUSE
    unsigned char pixel[3];
    // En OpenGL, la coordenada Y=0 es ABAJO, pero en GLUT Y=0 es ARRIBA. Hay que invertir Y.
    int viewportY = glutGet(GLUT_WINDOW_HEIGHT) - y; 
    glReadPixels(x, viewportY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    // 6. LÓGICA DE LA RÚBRICA (PUNTO 11)
    // Usamos rangos de tolerancia (> 200 y < 50) en lugar de valores exactos (== 255 y == 0)
    // Esto hace que los clics en los bordes suavizados también cuenten.
    if (pixel[0] > 200 && pixel[1] < 50 && pixel[2] < 50) {
        // Clic en ROJO (La Tapa articulada)
        printf("Clic en la TAPA! Acelerando animacion.\n");
        tapRotationSpeed = velocidadRapida;
        tapIsOpening = !tapIsOpening; 
        
        if (audioListo) {
            if (tapIsOpening) {
                ma_engine_play_sound(&motorAudio, "assets/audio/cofre/abrirCofre.mp3", NULL);
            } else {
                ma_engine_play_sound(&motorAudio, "assets/audio/cofre/cerrarCofre.mp3", NULL);
            }
        }
    } 
    else if (pixel[0] < 50 && pixel[1] > 200 && pixel[2] < 50) {
        // Clic en VERDE (Cuerpo libre de la caja)
        printf("Clic en la BASE! Velocidad normal.\n");
        tapRotationSpeed = velocidadNormal;
    }
    else if (pixel[0] < 50 && pixel[1] < 50 && pixel[2] > 200) {
        // Clic en AZUL (El Diamante)
        printf("Clic en el DIAMANTE!\n");
        
        if (audioListo) {
            // Se genera un número aleatorio entre 1 y 5
            int randomNum = (rand() % 5) + 1; 
            char rutaSonido[100]; 
            // Se ensambla la ruta dinámicamente
            sprintf(rutaSonido, "assets/audio/diamante/tocarDiamante%d.mp3", randomNum);
            ma_engine_play_sound(&motorAudio, rutaSonido, NULL);
        }
    }
    else {
        // Clic en el vacío (Fondo negro u otro color mezclado)
        printf("Clic en el vacio.\n"); 
    }
    
    // 7. RESTAURAMOS EL ESTADO PARA QUE display() DIBUJE NORMAL
    glEnable(GL_LIGHTING);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Tu color de fondo original
                      
    // Restauramos el tramado (Dither) para que las texturas reales se vean bien
    glEnable(GL_DITHER); 
    
    // EL FIX: Le damos la patada de arranque al motor de animación
    glutPostRedisplay();

}

// ============================================================================
// FUNCIÓN: teclado()
//
// GLUT la llama automáticamente cada vez que el usuario presiona
// una tecla NORMAL (letras, números, espacio, escape, etc.)
// ============================================================================
void teclado(unsigned char key, int x, int y) {
    switch (key) {

        // ------ ZOOM ------
        case 'w': case 'W':
            if (camDist > 5.0f) camDist -= 1.0f;
            break;

        case 's': case 'S':
            camDist += 1.0f;
            break;

        // ------ CONTROLES DE VISUALIZACIÓN (requisito 10) ------
        case 'c': case 'C':
            bCull = !bCull; // Alternamos: si estaba activo lo apagamos y viceversa
            break;

        case 'd': case 'D':
            bDepth = !bDepth;
            break;

        case 'z': case 'Z':
            bWireframe = !bWireframe;
            break;

        case 'x': case 'X':
            bSmooth = !bSmooth;
            break;
    
        // ------ MOVER EL FOCO CON TECLADO NUMÉRICO ------
        case '8': focoZ -= 0.5f; break;
        case '2': focoZ += 0.5f; break;
        case '6': focoX += 0.5f; break;
        case '4': focoX -= 0.5f; break;
        case '1': focoY += 0.5f; break;
        case '0': if (focoY > 0.5f) focoY -= 0.5f; break;

        case ' ': //espacio
            tapIsOpening = !tapIsOpening;
            glutPostRedisplay();
            break;

        case 27:
            exit(0);
            break;
    }

    lightPos[0] = focoX;
    lightPos[1] = focoY;
    lightPos[2] = focoZ;
    
    glutPostRedisplay();
}

// ============================================================================
// FUNCIÓN: tecladoEspecial()
//
// GLUT la llama cuando el usuario presiona teclas ESPECIALES:
// ============================================================================
void tecladoEspecial(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_LEFT:
            camAngleX -= 3.0f;
            break;
        case GLUT_KEY_RIGHT:
            camAngleX += 3.0f;
            break;
        case GLUT_KEY_UP:
            if (camAngleY < 89.0f) camAngleY += 3.0f;
            break;
        case GLUT_KEY_DOWN:
            if (camAngleY > 1.0f) camAngleY -= 3.0f;
            break;
        case GLUT_KEY_F1: bCull      = !bCull;      break;
        case GLUT_KEY_F2: bDepth     = !bDepth;     break;
        case GLUT_KEY_F3: bWireframe = !bWireframe; break;
        case GLUT_KEY_F4: bSmooth    = !bSmooth;    break;
    }

    glutPostRedisplay();
}

// ============================================================================
// FUNCIÓN: mouseBoton()
//
// GLUT la llama cuando el usuario PRESIONA o SUELTA un botón del mouse
// ============================================================================
void mouseBoton(int boton, int estado, int x, int y) {

    if (boton == GLUT_LEFT_BUTTON) {
        if (estado == GLUT_DOWN) {
            mousePresionado = true;
            lastMouseX = x;
            lastMouseY = y;

            // ¡AQUI LLAMAMOS AL PICKING!
            procesarSeleccion(x, y);

        } else { // GLUT_UP
            mousePresionado = false;
        }
    }

    /*
     * SCROLL del mouse (rueda):
     */
    if (boton == 3 && estado == GLUT_DOWN) {
        if (camDist > 5.0f) camDist -= 1.0f;
        glutPostRedisplay();
    }
    if (boton == 4 && estado == GLUT_DOWN) {
        camDist += 1.0f;
        glutPostRedisplay();
    }
}

// ============================================================================
// FUNCIÓN: mouseMovimiento()
// ============================================================================
void mouseMovimiento(int x, int y) {

    // Si el botón no está presionado, ignoramos el movimiento
    if (!mousePresionado) return;

    int dx = x - lastMouseX;
    int dy = y - lastMouseY;

    camAngleX += dx * 0.4f;
    camAngleY += dy * 0.4f;

    if (camAngleY >  89.0f) camAngleY =  89.0f;
    if (camAngleY <   1.0f) camAngleY =   1.0f;

    lastMouseX = x;
    lastMouseY = y;

    // Pedimos a GLUT que redibuje con los nuevos ángulos
    glutPostRedisplay();
}

// ============================================================================
// SECCION DE DIBUJO DE OBJETOS
// ============================================================================

// ============================================================================
// FUNCION: dibujarFoco()
//
// Dibuja una esfera pequeña amarilla en la posicion de la luz.
// Esto nos permite VER donde esta la fuente de luz en el espacio.
//
// TRUCO IMPORTANTE:
// Desactivamos GL_LIGHTING antes de dibujar el foco y lo reactivamos despues.
//
// ¿Por que? Porque si la iluminacion esta activa, OpenGL intentaria
// iluminar al foco con su propia luz — lo cual causaria que se vea
// oscuro en algunos angulos (absurdo para una fuente de luz).
// Al desactivarla, el foco se pinta con su color puro siempre: amarillo brillante.
// ============================================================================
void dibujarFoco() {
    glPushMatrix();

        // Nos movemos a la posicion de la luz
        glTranslatef(focoX, focoY, focoZ);

        // Desactivamos iluminacion -> el foco se pinta con color puro
        glDisable(GL_LIGHTING);

            /*
             * Color amarillo puro para que parezca que emite luz
             * R=1.0, G=1.0, B=0.0 -> amarillo
             */
            glColor3f(1.0f, 1.0f, 0.0f);

            /*
             * glutSolidSphere(radio, slices, stacks):
             * Dibuja una esfera solida
             *
             * - radio:  tamaño de la esfera (0.3 = pequeña, discreta)
             * - slices: "rebanadas" verticales (como meridianos del globo)
             * mas = mas redonda, mas costosa
             * - stacks: "capas" horizontales (como paralelos del globo)
             * mas = mas redonda, mas costosa
             *
             * Con 12,12 se ve suficientemente redonda sin ser costosa
             */
            glutSolidSphere(0.3, 12, 12);

        // Reactivamos iluminacion para todo lo que venga despues
        glEnable(GL_LIGHTING);

    glPopMatrix();
}


// ============================================================================
// FUNCIÓN: dibujarSkybox()
//
// Dibuja un cubo gigante con las texturas del espacio en sus caras interiores.
// ============================================================================
void dibujarSkybox() {

    float s = skyboxTamanio; // Abreviación para no repetir

    glPushMatrix();

    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);

    GLboolean cullActivo = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);

    // ========== CARA DERECHA (+X) ==========
    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[0]);
    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex3f( s, -s,  s);
        glTexCoord2f(1,0); glVertex3f( s, -s, -s);
        glTexCoord2f(1,1); glVertex3f( s,  s, -s);
        glTexCoord2f(0,1); glVertex3f( s,  s,  s);
    glEnd();

    // ========== CARA IZQUIERDA (-X) ==========
    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[1]);
    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex3f(-s, -s, -s);
        glTexCoord2f(1,0); glVertex3f(-s, -s,  s);
        glTexCoord2f(1,1); glVertex3f(-s,  s,  s);
        glTexCoord2f(0,1); glVertex3f(-s,  s, -s);
    glEnd();

    // ========== CARA ARRIBA (+Y) ==========
    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[2]);
    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex3f(-s,  s,  s);
        glTexCoord2f(1,0); glVertex3f( s,  s,  s);
        glTexCoord2f(1,1); glVertex3f( s,  s, -s);
        glTexCoord2f(0,1); glVertex3f(-s,  s, -s);
    glEnd();

    // ========== CARA ABAJO (-Y) - CORREGIDA ==========
    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[3]);
    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex3f(-s, -s,  s);  // CAMBIADO
        glTexCoord2f(1,0); glVertex3f( s, -s,  s);  // CAMBIADO
        glTexCoord2f(1,1); glVertex3f( s, -s, -s);  // CAMBIADO
        glTexCoord2f(0,1); glVertex3f(-s, -s, -s);  // CAMBIADO
    glEnd();

    // ========== CARA FRENTE (-Z) - CORREGIDA ==========
    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[4]);
    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex3f(-s, -s, -s);  // CAMBIADO
        glTexCoord2f(1,0); glVertex3f( s, -s, -s);  // CAMBIADO
        glTexCoord2f(1,1); glVertex3f( s,  s, -s);  // CAMBIADO
        glTexCoord2f(0,1); glVertex3f(-s,  s, -s);  // CAMBIADO
    glEnd();

    // ========== CARA ATRÁS (+Z) ==========
    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[5]);
    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex3f(-s, -s,  s);
        glTexCoord2f(1,0); glVertex3f( s, -s,  s);
        glTexCoord2f(1,1); glVertex3f( s,  s,  s);
        glTexCoord2f(0,1); glVertex3f(-s,  s,  s);
    glEnd();

    //Restauramos el estado del culling
    if (cullActivo) glEnable(GL_CULL_FACE);

    // ------------------------------------------------------------------
    // RESTAURAMOS EL ESTADO
    // ------------------------------------------------------------------
    glDisable(GL_TEXTURE_2D);  // Apagamos texturas //<- desconosco porque :D (Explicado en glosario)
    glDepthMask(GL_TRUE);       // Reactivamos escritura al Z-buffer
    glEnable(GL_LIGHTING);      // Reactivamos iluminación

    glPopMatrix();
}

// ============================================================================
// FUNCIÓN: dibujarPiso()
// Dibuja un plano rectangular horizontal que servirá como base del escenario
// ============================================================================
void dibujarPiso() {

    glPushMatrix();
    
    //Activamos la textura del suelo usando glEnable(GL_TEXTURE_2D)
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaPisoCristal);

    //color blanco para que la textura se vea con sus colores reales
    glColor3f(1.0f , 1.0f , 1.0f);

    glBegin(GL_QUADS);

        glNormal3f(0.0f, 1.0f, 0.0f);

        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-30.0f, 0.0f, -30.0f); // Esquina posterior izquierda

        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-30.0f, 0.0f,  30.0f); // Esquina frontal izquierda

        glTexCoord2f(1.0f, 1.0f);
        glVertex3f( 30.0f, 0.0f,  30.0f); // Esquina frontal derecha

        glTexCoord2f(0.0f, 1.0f);
        glVertex3f( 30.0f, 0.0f, -30.0f); // Esquina posterior derecha

    glEnd(); // Fin del cuadrilátero
    
    glDisable(GL_TEXTURE_2D); //<- desconosco porque :D (Explicado en glosario)
    glPopMatrix();
}

/*
 * Funcion Dibujar caja
 * Dibuja una cara rectangular abierta por arriba (sin tapita)
 * Las dimensiones son : cajaAncho(x), cajaAlto(y) , cajaProfundo(Z)
 * */
void dibujarCaja(){

    /*La caja esta cerrada en el origen 0,0,0
     * usare push/pop para no afectar otras transformaciones
     * */
    glPushMatrix();

    //area de textura de la caja :D
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaMaderaCaja);

    //trasladamos la cajita para que no este enterrada
    glTranslatef(0.0f , cajaAlto / 2.0f + 0.01f , 0.0f);

    //definimos el color de la caja: madera oscura cafe 
    glColor3f(0.4f,0.25f,0.1f);

    // ========== CARA 1: FRENTE (Z negativo) ==========
    /*
     * Normal apunta hacia AFUERA (hacia -Z)
     * Vértices en orden antihorario (CCW) visto desde afuera
     */
    glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, -1.0f);

        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);  // Arriba izquierda
                                      
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f( cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);  // Arriba derecha
                                      
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);  // Abajo derecha
                                      
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);  // Abajo izquierda
    glEnd();
    
    // ========== CARA 2: ATRÁS (Z positivo) ==========
    glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, 1.0f);

        glTexCoord2f(0.0f, 1.0f);
        glVertex3f( cajaAncho/2.0f,  cajaAlto/2.0f, cajaProfundo/2.0f);   // Arriba derecha
         
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f, cajaProfundo/2.0f);   // Arriba izquierda
        
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, cajaProfundo/2.0f);   // Abajo izquierda
         
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f, cajaProfundo/2.0f);   // Abajo derecha
    glEnd();
    
    // ========== CARA 3: IZQUIERDA (X negativo) ==========
    glBegin(GL_QUADS);
        glNormal3f(-1.0f, 0.0f, 0.0f);

        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f,  cajaProfundo/2.0f);  // Arriba atrás
         
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f); // Arriba frente

        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);  // Abajo frente

        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);  // Abajo atrás
    glEnd();
    
    // ========== CARA 4: DERECHA (X positivo) ==========
    glBegin(GL_QUADS);
        glNormal3f(1.0f, 0.0f, 0.0f);

        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);   // Arriba frente
                                      
        glTexCoord2f(1.0f, 1.0f);                                  
        glVertex3f(cajaAncho/2.0f,  cajaAlto/2.0f,  cajaProfundo/2.0f);   // Arriba atrás
                                      
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);   // Abajo atrás

        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);   // Abajo frente
    glEnd();
    
    // ========== CARA 5: FONDO (Y negativo) ==========
    glBegin(GL_QUADS);
        glNormal3f(0.0f, -1.0f, 0.0f);

        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);  // Frente izquierda
                                      
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);  // Frente derecha

        glTexCoord2f(1.0f, 0.0f);
        glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);  // Atrás derecha

        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);  // Atrás izquierda
    glEnd();
    

    glDisable(GL_TEXTURE_2D); //-> no entiendo por que es necesario esto (Explicado en glosario)
    glPopMatrix();
}

// ============================================================================
// FUNCIÓN: dibujarTapa()
// ============================================================================
void dibujarTapa() {
    
    glPushMatrix();

    //cargamos la textura de la tapita
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaTapaMadera);
    
    // La tapa está en la parte superior de la caja
    // Trasladamos a la altura de la parte superior
    glTranslatef(0.0f, cajaAlto + 0.01f, 0.0f);
   
    /*
     * BISAGRA LATERAL (lado derecho en X positivo):
     */
    glTranslatef(cajaAncho/2.0f, 0.0f, 0.0f);   // Vamos a la bisagra derecha
    glRotatef(-tapRotation, 0.0f, 0.0f, 1.0f);   // Rotamos alrededor del eje Y
    glTranslatef(-cajaAncho/2.0f, 0.0f, 0.0f);  // Volvemos

    // Color: madera más clara que la caja (para diferenciarse)
    glColor3f(0.5f, 0.3f, 0.15f);

    // Dibujamos la tapa como un rectángulo plano
    glBegin(GL_QUADS);
        
        /*
         * Normal apunta hacia ARRIBA (Y positivo)
         * porque la tapa está mirando hacia el cielo
         */
        glNormal3f(0.0f, 1.0f, 0.0f);
        
    /*
         * ¡CORRECCIÓN DE WINDING ORDER (CCW)!
         * Vértices declarados en sentido antihorario para que 
         * OpenGL sepa que esta es la cara FRONTAL superior.
         */
        glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-cajaAncho/2.0f, 0.0f,  cajaProfundo/2.0f);  // 1. Atrás izquierda

        glTexCoord2f(1.0f, 0.0f);
    glVertex3f( cajaAncho/2.0f, 0.0f,  cajaProfundo/2.0f);  // 2. Atrás derecha

        glTexCoord2f(1.0f, 1.0f);
    glVertex3f( cajaAncho/2.0f, 0.0f, -cajaProfundo/2.0f);  // 3. Frente derecha
        
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-cajaAncho/2.0f, 0.0f, -cajaProfundo/2.0f);  // 4. Frente izquierda      
    glEnd();
    
    glDisable(GL_TEXTURE_2D); //<- desconosco porque :D (Explicado en glosario)
    glPopMatrix();
}


// ============================================================================
// FUNCION NUEVA: dibujarReflejoMadera()
// Dibuja el Toroide y la Gema de cabeza usando el Stencil Buffer como máscara.
// ============================================================================
void dibujarReflejoMadera() {

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    // 1. Limpiar el Stencil Buffer
    glClear(GL_STENCIL_BUFFER_BIT);

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    // Apagamos los pinceles de color y profundidad
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);

    // Dibujar SOLO el fondo interior de la caja (crea la máscara)
    glPushMatrix();
        glTranslatef(0.0f , cajaAlto / 2.0f + 0.01f , 0.0f);
        glBegin(GL_QUADS);
            glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);
        glEnd();
    glPopMatrix();

    // 2. Preparar el dibujo del reflejo
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glDisable(GL_DEPTH_TEST); 

    // Atenuamos la luz
    GLfloat luzTenue[] = { 0.2f, 0.2f, 0.25f, 1.0f }; 
    glLightfv(GL_LIGHT0, GL_DIFFUSE, luzTenue);
    glLightfv(GL_LIGHT0, GL_SPECULAR, luzTenue);

    // 3. Dibujar de cabeza (El reflejo brilloso)
    glPushMatrix();
        glTranslatef(0.0f, 0.01f, 0.0f);
        glScalef(1.0f, -1.0f, 1.0f); // Espejo en el eje Y
        glTranslatef(0.0f, -0.01f, 0.0f);

        glFrontFace(GL_CW); // Invertir caras porque escalamos en negativo

        dibujarToroide();
        dibujarPatasToroide();
        dibujarGema();
    glPopMatrix();
    
    // =========================================================
    // 4. NUEVO: CAPA DE BARNIZ OSCURECEDOR (Filtro óptico)
    // =========================================================
    // Seguimos dentro del Stencil, así que esto solo afectará la base.
    glDisable(GL_LIGHTING);   // Apagamos la luz para pintar color puro
    glDisable(GL_TEXTURE_2D); // Apagamos texturas
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Tinte café oscuro (RGB: 0.1, 0.05, 0.02)
    // El 4to valor (0.75f) es la fuerza del filtro. 
    // Si quieres que el reflejo sea aún más tenue, súbelo a 0.85f o 0.90f.
    // Si quieres que el reflejo sea más visible, bájalo a 0.60f.
    glColor4f(0.1f, 0.05f, 0.02f, 0.75f); 
    
    glPushMatrix();
        // Lo dibujamos un milímetro arriba del piso para cubrir el reflejo
        glTranslatef(0.0f , cajaAlto / 2.0f + 0.015f , 0.0f);
        glBegin(GL_QUADS);
            glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);
        glEnd();
    glPopMatrix();

    // Restaura absolutamente todo a la normalidad
    glPopAttrib();
    
}





void dibujarSombraCaja(){

    // =========================================================
    // FASE 1: STENCIL (marcar SOLO el área del piso)
    // =========================================================
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    
    // Dibujar piso sin color para marcar stencil
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    
    glBegin(GL_QUADS);
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(-30.0f, 0.0f, -30.0f);
        glVertex3f(-30.0f, 0.0f,  30.0f);
        glVertex3f( 30.0f, 0.0f,  30.0f);
        glVertex3f( 30.0f, 0.0f, -30.0f);
    glEnd();
    
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // =========================================================
    // FASE 2: SOMBRA (solo donde stencil = 1)
    // =========================================================
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

    // ¡EL FIX MAGISTRAL!
    // glPushAttrib guarda el estado de TODO (Depth, Luces, Culling, Texturas, Blend)
    // Así no importa qué modifiquemos, F1 y F2 están a salvo.
    glPushAttrib(GL_ALL_ATTRIB_BITS); 
    
    glDisable(GL_DEPTH_TEST);  // Evita z-fighting
    glDisable(GL_LIGHTING);     // Sombra plana
    glDisable(GL_CULL_FACE);    // Sombra desde cualquier ángulo
    glDisable(GL_TEXTURE_2D);   // Sombra sin texturas
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Color negro semi-transparente
    glColor4f(0.0f, 0.0f, 0.0f, 0.35f);
    
    // Calcular matriz de sombra
    GLfloat planeEquation[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
    GLfloat lightPosArray[4] = { lightPos[0], lightPos[1], lightPos[2], lightPos[3] };
    GLfloat shadowMatrix[16];
    gltMakeShadowMatrix(planeEquation, lightPosArray, shadowMatrix);

    // --- INICIO DE LA PRENSA HIDRÁULICA (MATRIZ DE SOMBRA) ---
    glPushMatrix();
        glMultMatrixf(shadowMatrix); // APLASTAMOS TODO
        
        //--- 1. Sombra del cuerpo de la caja ---
        glPushMatrix();
            glTranslatef(0.0f, cajaAlto / 2.0f + 0.01f, 0.0f);
            glBegin(GL_QUADS);
                // Frente
                glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);
                glVertex3f( cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);
                glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
                glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
                // Atrás
                glVertex3f( cajaAncho/2.0f,  cajaAlto/2.0f, cajaProfundo/2.0f);
                glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f, cajaProfundo/2.0f);
                glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, cajaProfundo/2.0f);
                glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f, cajaProfundo/2.0f);
                // Izquierda
                glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f,  cajaProfundo/2.0f);
                glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);
                glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
                glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);
                // Derecha
                glVertex3f(cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);
                glVertex3f(cajaAncho/2.0f,  cajaAlto/2.0f,  cajaProfundo/2.0f);
                glVertex3f(cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);
                glVertex3f(cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
            glEnd();
        glPopMatrix();

        // --- 2. Sombra de la tapa ---
        glPushMatrix();
            glTranslatef(0.0f, cajaAlto + 0.01f, 0.0f);
            glTranslatef(cajaAncho/2.0f, 0.0f, 0.0f);   
            glRotatef(-tapRotation, 0.0f, 0.0f, 1.0f); 
            glTranslatef(-cajaAncho/2.0f, 0.0f, 0.0f);  

            glBegin(GL_QUADS);
                glVertex3f(-cajaAncho/2.0f, 0.0f,  cajaProfundo/2.0f);  
                glVertex3f( cajaAncho/2.0f, 0.0f,  cajaProfundo/2.0f);  
                glVertex3f( cajaAncho/2.0f, 0.0f, -cajaProfundo/2.0f);  
                glVertex3f(-cajaAncho/2.0f, 0.0f, -cajaProfundo/2.0f);        
            glEnd();
        glPopMatrix();
        
    glPopMatrix(); 
    
    // RESTAURAMOS LA MÁQUINA DE ESTADOS
    // (Esto regresa el Depth Test y Culling a lo que dictan tus teclas F1-F4)
    glPopAttrib(); 
    
    // El Stencil sí lo apagamos manualmente porque ya no se necesita
    glDisable(GL_STENCIL_TEST);
}




// ============================================================================
// FUNCIONES DE CONFIGURACION E INICIALIZACION
// ============================================================================

// ============================================================================
// FUNCIÓN: cargarTexturasSkybox()
// ============================================================================
void cargarTexturasSkybox() {

    /*
     * Habilitamos el sistema de texturas 2D de OpenGL
     * Sin esto, glBindTexture no tiene efecto
     */
    glEnable(GL_TEXTURE_2D);

    // Cargamos cada cara del skybox
    // Ajusta los nombres si tus imágenes corresponden a caras diferentes
    skyboxTexturas[0] = cargarBMP("assets/1.bmp"); // derecha
    skyboxTexturas[1] = cargarBMP("assets/2.bmp"); // izquierda
    skyboxTexturas[2] = cargarBMP("assets/3.bmp"); // arriba
    skyboxTexturas[3] = cargarBMP("assets/4.bmp"); // abajo
    skyboxTexturas[4] = cargarBMP("assets/5.bmp"); // frente
    skyboxTexturas[5] = cargarBMP("assets/6.bmp"); // atrás

    // Desactivamos texturas al terminar
    // Las activaremos solo cuando dibujemos el skybox
    glDisable(GL_TEXTURE_2D);
}

// ============================================================================
// FUNCIÓN: cargarBMP()
// ============================================================================
GLuint cargarBMP(const char* ruta) {

    // ------------------------------------------------------------------
    // 1. ABRIR EL ARCHIVO
    // ------------------------------------------------------------------
    FILE* archivo = fopen(ruta, "rb"); // "rb" = read binary (leer en binario)
    if (!archivo) {
        printf("ERROR: No se pudo abrir la textura: %s\n", ruta);
        return 0; // Devolvemos 0 = textura inválida
    }

    // ------------------------------------------------------------------
    // 2. LEER EL ENCABEZADO BMP
    // ------------------------------------------------------------------
    unsigned char encabezado[54];
    if (fread(encabezado, 1, 54, archivo) != 54 || encabezado[0] != 'B' || encabezado[1] != 'M') {
        printf("ERROR: Encabezado BMP inválido en: %s\n", ruta);
        fclose(archivo);
        return 0;
    }

    int ancho  = encabezado[18] | (encabezado[19] << 8) |
                 (encabezado[20] << 16) | (encabezado[21] << 24);
    int alto   = encabezado[22] | (encabezado[23] << 8) |
                 (encabezado[24] << 16) | (encabezado[25] << 24);

    // ------------------------------------------------------------------
    // 3. LEER LOS DATOS DE PÍXELES
    // ------------------------------------------------------------------
    int tamanioDatos = ancho * alto * 3;
    unsigned char* datos = (unsigned char*)malloc(tamanioDatos);

    if (!datos) {
        printf("ERROR: Sin memoria para cargar: %s\n", ruta);
        fclose(archivo);
        return 0;
    }

    size_t bytesLeidos = fread(datos, 1, tamanioDatos, archivo);
    if(bytesLeidos !=(size_t)tamanioDatos){
    printf("ADVERTENCIA: lECTURA INCOMPLETA DE %s\n", ruta);
    }
    fclose(archivo);

    // ------------------------------------------------------------------
    // 4. CREAR LA TEXTURA EN OPENGL
    // ------------------------------------------------------------------
    GLuint idTextura;
    glGenTextures(1, &idTextura);
    glBindTexture(GL_TEXTURE_2D, idTextura);

    /*
     * ¡NUEVO!: ALINEACIÓN DE MEMORIA (El fix para la línea verde/basura)
     * Le decimos a OpenGL que los píxeles están empaquetados byte por byte (1),
     * en lugar de bloques de 4. Esto evita que los canales RGB se desfasen
     * en la orilla de la imagen creando colores irreales.
     */
   // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ancho, alto,
                 0, GL_BGR, GL_UNSIGNED_BYTE, datos);

    free(datos); // Liberamos la memoria RAM (ya está en la GPU)

    printf("Textura cargada: %s (%dx%d)\n", ruta, ancho, alto);
    return idTextura;
}

// ============================================================================
// FUNCIÓN: configurarOpenGL()
// ============================================================================
void configurarOpenGL() {

    // Cargamos las texturas del skybox UNA SOLA VEZ
    cargarTexturasSkybox();

    // Cargamos la textura de la caja
    texturaMaderaCaja = cargarBMP("assets/textures/madera/Pino.bmp");
    // Cargamos la textura de la tapa de madera
    texturaTapaMadera = cargarBMP("assets/textures/madera/Wood_Tile.bmp");
    // Cargamos la textura de ORO
    texturaOro = cargarBMP("assets/textures/metal/metal_gold.bmp");

    // Cargamos la textura del suelo donde esta apoyada la caja V:
    texturaPisoCristal = cargarBMP("assets/textures/cristal/glass_texture_pure.bmp");
    //configuramos el patron loseta para no dejar espacios en blanco
    glBindTexture(GL_TEXTURE_2D, texturaPisoCristal);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);      // Repetir en X
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);      // Repetir en Y

    /*
     * NORMALIZACIÓN:
     * Evita que las normales se deformen al rotar la cámara.
     * Crucial para que el cálculo matemático de la luz no se dispare
     * y genere brillos infinitos.
     */
    glEnable(GL_NORMALIZE);

    /*
     * MODELO DE LUZ:
     * GL_LIGHT_MODEL_LOCAL_VIEWER: Fuerza a OpenGL a calcular los vectores 
     * de luz desde la posición exacta de la cámara, suavizando el Gouraud Shading.
     * GL_LIGHT_MODEL_TWO_SIDE: Ilumina AMBAS caras (frente y reverso)
     * Perfecto para ver el interior de la caja iluminado
     */
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    // ------------------------------------------------------------------
    // 1. COMPONENTES DE LA LUZ
    // ------------------------------------------------------------------
    GLfloat luzAmbiental[] = { 0.25f, 0.25f, 0.25f, 1.0f }; //-> para mas contraste
    GLfloat luzDifusa[] = { 1.0f, 1.0f, 0.95f, 1.0f }; //-> blanco natural
    GLfloat luzEspecular[] = { 0.8f, 0.8f, 0.8f, 1.0f };

    // Aplicamos ambas componentes a GL_LIGHT0 (la primera luz de OpenGL)
    glLightfv(GL_LIGHT0, GL_AMBIENT,  luzAmbiental);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  luzDifusa);
    glLightfv(GL_LIGHT0, GL_SPECULAR, luzEspecular);

    /*
     * ATENUACIÓN POR DISTANCIA (NUEVO)
     * Le dice a OpenGL cómo debe decaer la luz conforme se aleja del objeto.
     * Debemos de cuidar esto, dado que la atenuacion es cuadratica, recortara 
     * la luz de forma muy brusca. es por ello que desactive 
     * GL_QUADRATIC_ATENUATION a 0.0f para no tener caida cuadratica
     * esta es la formula (c + l·d + q·d²), d² aplasta la luz muy fuertemente 
     */
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.8f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.005f); // caida de luz suave
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0f); // anulamos la atenuacion

    // ========================================================================
    // 2. CONFIGURACIÓN DE MATERIAL (NUEVA SECCIÓN)
    // ========================================================================
    GLfloat materialEspecular[] = { 0.3f, 0.3f, 0.3f, 1.0f };  // Más mate (evita sobresaturación)
    GLfloat materialBrillo[]    = { 100.0f };                  // Concentración máxima del reflejo

    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  materialEspecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, materialBrillo);

    // ------------------------------------------------------------------
    // 3. HABILITAR COLOR DE VÉRTICES COMO MATERIAL
    // ------------------------------------------------------------------
    glEnable(GL_COLOR_MATERIAL);
    
    /*
     * GL_FRONT_AND_BACK → el color glColor3f() se aplica a AMBAS caras
     * Esto es necesario porque activamos GL_LIGHT_MODEL_TWO_SIDE
     * Si solo aplicamos color al frente, el reverso usa material blanco por defecto
     */
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // ------------------------------------------------------------------
    // 4. ENCENDER LA LUZ "sistema en general"
    // ------------------------------------------------------------------
    glEnable(GL_LIGHTING); // Interruptor maestro: activa el sistema de luces
    glEnable(GL_LIGHT0);   // Enciende la primera luz (la que configuramos arriba)

    // ------------------------------------------------------------------
    // 5. DEPTH TEST (Prueba de profundidad / Z-buffer)
    // ------------------------------------------------------------------
    glEnable(GL_DEPTH_TEST);

    // ------------------------------------------------------------------
    // 6. ORDEN DE VÉRTICES (Winding)
    // ------------------------------------------------------------------
    glFrontFace(GL_CCW);


    //---------------------------------------------------------------------
    //7.- Efetos de transparencia Blending para mesclar colores :D
    //---------------------------------------------------------------------
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // no necesarios ahora, ya que los metodos dibujarGema y dibujarSombraCaja lo implementan :D
}

// ============================================================================
// MAIN
// ============================================================================
int main(int argc, char** argv) {
     
    srand(time(NULL));

    // Inicializar GLUT con los argumentos de la línea de comandos
    glutInit(&argc, argv);

    /*
     * glutInitDisplayMode(mode):
     * Configura el modo de visualización inicial
     * * Flags usados:
     * - GLUT_DOUBLE: Doble buffer (dibuja fuera de pantalla, luego copia)
     * → Evita parpadeo (flickering)
     * - GLUT_RGB: Modo de color RGB (rojo, verde, azul)
     * - GLUT_DEPTH: Buffer de profundidad (Z-buffer)
     * → Para saber qué objeto está adelante/atrás
     * - GLUT_STENCIL: Buffer de stencil
     * → Para efectos avanzados (reflejos, sombras)
     */
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);

    // Tamaño inicial de la ventana
    glutInitWindowSize(800, 600);

    // Crea la ventana con nombre
    glutCreateWindow("ProyectoFinal: caja enigma");

    /*
     * glClearColor(r, g, b, a):
     * Define el color de fondo cuando se "limpia" la pantalla
     * Valores: 0.0 (oscuro) a 1.0 (claro)
     * - (0.1f, 0.1f, 0.1f): Gris muy oscuro (casi negro)
     * - (1.0f): Alpha = completamente opaco
     */
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    /*
     * Registrar CALLBACKS: funciones que GLUT llamará automáticamente en eventos
     */
    glutDisplayFunc(display);       // Llamada cada vez que se redibuja
    glutReshapeFunc(myReshape);     // Llamada cuando la ventana cambia de tamaño

     /*
     * Registramos los CALLBACKS de controles:
     *
     * glutKeyboardFunc   → teclas normales (letras, números, escape)
     * glutSpecialFunc    → teclas especiales (flechas, F1-F12)
     * glutMouseFunc      → clic/soltar botones del mouse
     * glutMotionFunc     → movimiento del mouse CON botón presionado
     */
    glutKeyboardFunc(teclado);
    glutSpecialFunc(tecladoEspecial);
    glutMouseFunc(mouseBoton);
    glutMotionFunc(mouseMovimiento);

                
    configurarOpenGL(); //-> preparamos luces y materiales antes del loop


    //inicializaremos el audio
    if (ma_engine_init(NULL, &motorAudio) == MA_SUCCESS) {
        audioListo = true;
        printf("Audio inicializado correctamente.\n");
    } else {
        printf("ERROR: No se pudo inicializar el motor de audio.\n");
    }



    /*
     * glutMainLoop():
     * Inicia el CICLO INFINITO DE EVENTOS
     * El programa se queda aquí hasta que cierres la ventana
     * Continuamente:
     * 1. Dibuja
     * 2. Procesa eventos del mouse/teclado
     * 3. Llama a funciones registradas (callbacks)
     * 4. Repite
     */
    glutMainLoop();

    return 0;
}

// ============================================================================
// GLOSARIO: SISTEMA DE COORDENADAS EN OPENGL
// ============================================================================
/*
 * Eje X: Horizontal (paralelo a la pantalla)
 * - Negativo (-X): izquierda
 * - Positivo (+X): derecha
 *
 * Eje Y: Vertical (paralelo a la pantalla)
 * - Negativo (-Y): abajo
 * - Positivo (+Y): arriba
 *
 * Eje Z: Profundidad (perpendicular a la pantalla)
 * - Negativo (-Z): hacia afuera (hacia ti)
 * - Positivo (+Z): hacia adentro (lejos de ti)
 *
 * ORIGEN (0, 0, 0): Centro del mundo
 */

// ============================================================================
// NOTAS SOBRE EL CÓDIGO
// ============================================================================
/*
 * Este código es el ESQUELETO del proyecto:
 * - PASO 1: Dibujar un piso con textura (HECHO) -> Punto 4.
 * - PASO 2: Sistema de iluminacion (HECHO) -> Punto 5.
 * - PASO 3: Dibujar caja con texturas (HECHO) -> Puntos 6 y 8.
 * - PASO 4: Tapa articulada con animacion (HECHO) -> Punto 9.
 * - PASO 5: Flags de visualizacion F1-F4 (HECHO) -> Punto 10.
 * - PASO 6: Seleccion de objetos con clic (HECHO) -> Punto 11.
 * * ¿Donde y como se cumple la rubrica?:
 *
 * 1. Coordenadas Esfericas (Punto 3): Implementado en display() usando eyeX/Y/Z 
 * calculados con sin/cos para permitir una rotacion polar fluida.
 * 2. Iluminacion Phong (Punto 5): Configurado en configurarOpenGL() definiendo 
 * componentes ambiental, difusa y especular sobre GL_LIGHT0.
 * 3. Materiales y Normales (Punto 7): Cada cara de la caja y tapa tiene su 
 * glNormal3f() definida para interactuar correctamente con la luz.
 * 4. Picking/Seleccion (Punto 11): Implementado en procesarSeleccion() usando 
 * la tecnica de Color Picking (lectura de pixeles en el buffer trasero).
 * * IMPORTANTE: Cada paso es independiente. Verifica que cada uno funcione
 * antes de pasar al siguiente.
 *
 *
 * ### Resumen de controles:
 * Mouse izquierdo + arrastrar  →  rotar cámara
 * Scroll arriba/abajo          →  zoom in/out
 * W / S                        →  zoom in/out (alternativo)
 * Flechas ←→↑↓                →  rotar cámara (alternativo)
 * C                            →  culling on/off
 * D                            →  depth test on/off
 * Z                             →  wireframe on/off
 * X                            →  smooth/flat shading
 * Escape                       →  cerrar
 * F1-F4                        →  mismos que C,D,Z,X
 *
 *
 *
 * glDisable(GL_TEXTURE_2D):
 * 
 * Resulta que OpenGL trabaja como una maquina de estados.
 * Si prendes la textura (glEnable) para dibujar una madera, la textura se
 * quedara pegada en la brocha de dibujo. Si no la apago despues de dibujar,
 * todo lo que dibuje a continuacion (como el foquito amarillo o las lineas de
 * ayuda) va a intentar dibujarse con textura de madera. Apagarla garantiza
 * que los colores puros vuelvan a funcionar.

 * glPushMatrix() y glPopMatrix():
 * Son como un "punto de guardado" para tus coordenadas del universo.
 * 
 * glPushMatrix() guarda como estan todos los ejes antes de que los toques.
 * Despues de hacer glTranslate o glRotate (por ejemplo, para mover la tapa).
 * 
 * Al terminar, llamo a glPopMatrix() para regresar el universo a su estado
 * original. Si no lo uso, al rotar la tapa terminare rotando tambien el
 * piso entero, el foco y cualquier otra cosa que se dibuje despues.
 * 
 * glutPostRedisplay():
 * Es la magia para que las cosas se muevan. Cuando cambio una variable (como
 * el angulo de la camara con el mouse o la animacion de la tapa), esa variable
 * por si sola no hace que la pantalla se actualice. glutPostRedisplay le avisa
 * a OpenGL: "Oye, cambie un valor, por favor vuelve a llamar a la funcion
 * display() lo antes posible para pintar el nuevo fotograma". Si no lo pongo,
 * la logica avanza pero la ventana se queda congelada mostrando lo mismo.
 *
 *
 *
 *
 * ¿Por que usamos glDisable(GL_TEXTURE_2D) al final de cada dibujo?
 * OpenGL funciona como un "pincel" que guarda memoria. Si habilitas una textura 
 * para la madera y no la deshabilitas, el siguiente objeto que dibujes 
 * (como el foco amarillo) intentara usar los colores de la madera. Al apagarla, 
 * garantizamos que los objetos sin textura recuperen su color solido.
 *
 * ¿Para que sirve glEnable(GL_NORMALIZE)?
 * Cuando rotamos o escalamos objetos (como la tapa), los vectores normales pueden 
 * estirarse matematicamente. Si una normal mide mas de 1.0, la luz brilla con 
 * una intensidad infinita (fogonazos). Esta funcion obliga a OpenGL a que todas 
 * las normales midan siempre 1.0, manteniendo la luz estable.
 *
 * ¿Que es GL_LIGHT_MODEL_LOCAL_VIEWER?
 * Por defecto, OpenGL asume que la camara esta infinitamente lejos. Esto causa 
 * que el brillo especular sea brusco. Al activarlo, OpenGL calcula la luz 
 * basandose en la posicion exacta de tu ojo, logrando que el brillo en la madera 
 * se mueva de forma realista cuando mueves el mouse.
 *
 * ¿Por que invertimos el orden de vertices en la tapa? (Winding Order)
 * OpenGL usa la "regla de la mano derecha". Si declaras los vertices en sentido 
 * horario, la cara apunta hacia abajo. Al cambiarlos a sentido antihorario (CCW), 
 * la cara apunta hacia arriba, lo que permite que la luz pegue en el lado 
 * correcto de la tapa y no por debajo.
 *
 *
 * glDisable(GL_TEXTURE_2D); en la sombra:
 * La textura funciona como una "estampa" que se pega sobre los polígonos. Si no apagamos las texturas
 * al momento de pintar la sombra, OpenGL intentará multiplicar el color negro semitransparente por la
 * textura de madera de la tapa. El resultado es que tu sombra se vería como una mancha de madera deformada 
 * sobre el cristal. Al apagarla, garantizamos que sea un color negro puro.
 * 
 * glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);:
 * Esta es la fórmula matemática de la transparencia.

 * GL_SRC_ALPHA: Toma el porcentaje Alfa (opacidad) del color que vas a pintar
 * (en nuestro caso 0.35f o 35% de negro).
 * 
 * GL_ONE_MINUS_SRC_ALPHA: Toma lo que sobra (1.0 - 0.35 = 0.65f o 65%) del color del píxel 
 * que ya estaba en el fondo (el piso de cristal).
 *
 * OpenGL los suma, creando el efecto de que el cristal se oscurece un 35% pero sigues viendo a través de él.
 *
 *
 *
 *
 *
 *
 *
 *  CONCEPTO:
 * miniaudio.h es una maravilla de la ingeniería en C. Es una librería single-header, lo que significa 
 * que todo su código fuente (declaraciones e implementaciones) vive dentro de un único archivo .h. Para
 * decirle al compilador g++ que no solo lea las definiciones, sino que también compile el código interno 
 * del motor de audio, debemos definir la macro MINIAUDIO_IMPLEMENTATION justo antes de incluirla, y solo en 
 * un archivo .cpp de tu proyecto.
 *
 *
*/


