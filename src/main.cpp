/*
 * Alumno : Jose pablo Perez De Anda
 * Materia: Graficas Por Computadora
 * Profesor: Georgii Khachaturov
 *
 * Proyecto_final : proyecto_caja (
 *
 * Control de versiones:
 * 1.- Establecimos el esqueleto inicial.
 * 2.- Sistema de cámara con rotación polar.
 * 3.- Plano base (piso) con normales.
 * 4.- Sombras.
 * 5.- Limpieza de código y organización.
 * 6.- Implementación del sistema de puzle Temporal.
 * 7.- [MERGE] Quadrics globales para eficiencia + estandarte NURBS,
 * material del pergamino, guard del reflejo y animación bajo demanda.
 * 8.- [FIX] Blindaje de Máquina de Estados (Picking) y Optimización Geométrica.
 * 9.- [FIX CRÍTICO] Eliminación de GL_ALL_ATTRIB_BITS en el Render Loop para evitar
 * saturación del driver Mesa (AMD) y prevenir freezes.
 * 10.- [RESTORE] Se restauraron las funciones de dibujo estáticas.
 * 11.- [RELEASE] Documentación exhaustiva, teoría in-line y Glosario Técnico Final.
 */

// ============================================================================
// INCLUDES
// ============================================================================
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <GL/glut.h>

// Librería header-only para gestión de audio en múltiples plataformas
#define MINIAUDIO_IMPLEMENTATION
#include "include/miniaudio.h"

// ============================================================================
// VARIABLES GLOBALES Y GESTIÓN DE ESTADO
// ============================================================================

ma_engine motorAudio;
bool audioListo = false;

// --- Texturas ---
GLuint skyboxTexturas[6];
float skyboxTamanio = 200.0f;

GLuint texturaMaderaCaja  = 0;
GLuint texturaTapaMadera  = 0;
GLuint texturaPisoCristal = 0;
GLuint texturaOro         = 0;
GLuint texturaPapel       = 0;

// --- Control de Tiempo/Animaciones ---
float velocidadNormal = 1.5f;
float velocidadRapida = 5.0f;

// --- Sistema de Cámara (Coordenadas Polares) ---
float camDist   = 25.0f; // Radio
float camAngleX =  0.0f; // Azimut
float camAngleY = 20.0f; // Elevación

int  lastMouseX      = 0;
int  lastMouseY      = 0;
bool mousePresionado = false;

// --- Iluminación (Punto 6 del proyecto) ---
// Luz posicional (w=1.0f indica posición, no vector direccional)
float lightPos[4] = { 5.0f, 10.0f, 5.0f, 1.0f };

// --- Lógica del Puzzle y Objetos Especiales ---
int  combinacion[4]       = {1,1,1,1};
bool cajaDesbloqueada     = false;

// Puntos de control para las superficies NURBS (Punto 4 del proyecto)
GLUnurbsObj* nurbPergamino = NULL;
GLfloat ctrlPointsNurbs[4][4][3];   // Puntos del pergamino base
GLfloat ctrlPointsBanner[4][4][3];  // Puntos del estandarte (bandera ondulante)

// Quadrics globales (Optimización de memoria, instanciados una sola vez)
GLUquadricObj* qGema       = NULL;
GLUquadricObj* qPatas      = NULL;
GLUquadricObj* qPicking    = NULL;
GLUquadricObj* qPalo       = NULL;   
GLUquadricObj* qBolita     = NULL;   

// --- Flags de Visualización (Punto 10 del proyecto) ---
bool bCull      = false; // Culling de caras traseras
bool bDepth     = true;  // Prueba de profundidad (Z-Buffer)
bool bWireframe = false; // Modo líneas o relleno
bool bSmooth    = false; // Shading Gouraud (Smooth) o Flat

// --- Dimensiones y Estado Físico del Objeto Articulado ---
float cajaAncho    = 6.0f;
float cajaProfundo = 8.0f;
float cajaAlto     = 5.0f;

// Rotación de la articulación de la tapa (Punto 1 y 2 del proyecto)
float tapRotation      = 0.0f;
float tapMaxRotation   = 120.0f;
float tapRotationSpeed = 1.5f;
bool  tapIsOpening     = false;

float gemaRadius = 1.65f;
float gemaY      = 2.4f;

// ============================================================================
// PROTOTIPOS
// ============================================================================
void dibujarSkybox();
void dibujarFoco();
void dibujarPiso();
void dibujarCaja();
void dibujarReflejoMadera();
void dibujarTapa();
void dibujarSombraCaja();
void dibujarToroide();
void dibujarPatasToroide();
void dibujarGema();
void dibujarPanelesPuzzle();
void dibujarPergamino();
void dibujarEstandarte();

void display();
void myReshape(int w, int h);
void procesarSeleccion(int x, int y);

void teclado(unsigned char key, int x, int y);
void tecladoEspecial(int key, int x, int y);
void mouseBoton(int boton, int estado, int x, int y);
void mouseMovimiento(int x, int y);

void configurarOpenGL();
void cargarTexturasSkybox();
GLuint cargarBMP(const char* ruta);
void initNURBS();
void initQuadrics();

// ============================================================================
// FUNCIÓN: gltMakeShadowMatrix
// ----------------------------------------------------------------------------
// Descripción: Crea una matriz de proyección ortogonal para aplastar cualquier
// geometría 3D contra un plano 2D específico, desde la perspectiva de la luz.
// Teoría: Utiliza álgebra lineal (producto punto) entre el vector posición
// de la luz y la ecuación del plano (Ax + By + Cz + D = 0).
// ============================================================================
void gltMakeShadowMatrix(GLfloat vPlaneEquation[], GLfloat vLightPos[], GLfloat destMat[]) {
    GLfloat dot;
    dot = vPlaneEquation[0]*vLightPos[0] + vPlaneEquation[1]*vLightPos[1] +
          vPlaneEquation[2]*vLightPos[2] + vPlaneEquation[3]*vLightPos[3];

    destMat[0]  = dot - vLightPos[0]*vPlaneEquation[0];
    destMat[4]  =     - vLightPos[0]*vPlaneEquation[1];
    destMat[8]  =     - vLightPos[0]*vPlaneEquation[2];
    destMat[12] =     - vLightPos[0]*vPlaneEquation[3];

    destMat[1]  =     - vLightPos[1]*vPlaneEquation[0];
    destMat[5]  = dot - vLightPos[1]*vPlaneEquation[1];
    destMat[9]  =     - vLightPos[1]*vPlaneEquation[2];
    destMat[13] =     - vLightPos[1]*vPlaneEquation[3];

    destMat[2]  =     - vLightPos[2]*vPlaneEquation[0];
    destMat[6]  =     - vLightPos[2]*vPlaneEquation[1];
    destMat[10] = dot - vLightPos[2]*vPlaneEquation[2];
    destMat[14] =     - vLightPos[2]*vPlaneEquation[3];

    destMat[3]  =     - vLightPos[3]*vPlaneEquation[0];
    destMat[7]  =     - vLightPos[3]*vPlaneEquation[1];
    destMat[11] =     - vLightPos[3]*vPlaneEquation[2];
    destMat[15] = dot - vLightPos[3]*vPlaneEquation[3];
}

// ============================================================================
// FUNCIONES DE DIBUJO GEOMÉTRICO
// ============================================================================

/*
 * dibujarToroide: Dibuja el anillo dorado de la base usando Sphere Mapping.
 * Técnica: GL_SPHERE_MAP genera dinámicamente coordenadas de textura
 * basadas en el vector normal y el ángulo de visión, simulando un entorno cromado/metálico.
 */
void dibujarToroide() {
    glPushMatrix();
    glTranslatef(0.0f, 2.0f, 0.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaOro);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

    glColor3f(1.0f, 0.84f, 0.0f);
    
    // Reducción geométrica: de 1440 quads a solo 432 quads para optimizar el bus.
    glutSolidTorus(0.135f, 1.4f, 12, 36);

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

/*
 * dibujarGema: Dibuja la joya central.
 * Técnica (Punto 7): Define propiedades de material (Especular, Brillantez, Ambiente)
 * según el modelo de iluminación de Phong.
 * Usa GL_BLEND para darle una estética translúcida mágica mezclada con la textura de la madera.
 */
void dibujarGema() {
    glPushMatrix();
    glTranslatef(0.0f, gemaY, 0.0f);

    // Configuración del modelo de iluminación de Phong (Materiales)
    GLfloat gemaEspecular[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Reflejos blancos intensos
    GLfloat gemaBrillo[]    = { 128.0f }; // Shininess altísima (cristal/vidrio)
    GLfloat gemaAmbiente[]  = { 0.1f, 0.2f, 0.6f, 1.0f }; // Tono base azulado
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   gemaAmbiente);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  gemaEspecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, gemaBrillo);

    glColor4f(0.2f, 0.5f, 1.0f, 0.85f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Mapeo ambiental para darle un tono iridiscente
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaMaderaCaja);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
    GLfloat blendColor[] = { 0.2f, 0.5f, 1.0f, 1.0f };
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, blendColor);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

    // Polygon Offset previene el Z-fighting entre el sólido y el wireframe
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);

    gluQuadricDrawStyle(qGema, GLU_FILL);

    float R  = gemaRadius;
    float h1 = 0.5f;
    float h2 = 1.20f;

    // Geometría del cristal
    glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(qGema, R, 0.0f, h1, 16, 4);
    glPopMatrix();
    glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(qGema, R, 0.0f, h2, 16, 4);
    glPopMatrix();

    glDisable(GL_POLYGON_OFFSET_FILL);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_2D);

    // --- Dibujo de las aristas (Wireframe) superpuestas ---
    // Guardamos estado para no contaminar la luz del resto de los objetos
    glPushAttrib(GL_LIGHTING_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);
    glColor4f(0.3f, 0.6f, 1.0f, 0.20f);
    
    gluQuadricDrawStyle(qGema, GLU_LINE);

    glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(qGema, R, 0.0f, h1, 16, 4);
    glPopMatrix();
    glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(qGema, R, 0.0f, h2, 16, 4);
    glPopMatrix();

    gluQuadricDrawStyle(qGema, GLU_FILL); // Restauramos
    glPopAttrib();
    
    glDisable(GL_BLEND);
    glPopMatrix();
}

void dibujarPatasToroide() {
    glPushMatrix();
    glTranslatef(0.0f, 2.0f, 0.0f);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaOro);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glColor3f(1.0f, 0.84f, 0.0f);

    float radio = 1.4f;
    float posiciones[4][2] = { { radio, 0.0f}, {-radio, 0.0f}, { 0.0f, radio}, { 0.0f, -radio} };

    gluQuadricDrawStyle(qPatas, GLU_FILL);
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
            glTranslatef(posiciones[i][0], 0.0f, posiciones[i][1]);
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            gluCylinder(qPatas, 0.12f, 0.07f, 2.0f, 12, 4);
        glPopMatrix();
    }

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

/*
 * dibujarPanelesPuzzle: Muestra los números actuales del candado usando fuentes Stroke de GLUT.
 */
void dibujarPanelesPuzzle() {
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
    glDisable(GL_LIGHTING); // Desactivar luces para un color brillante puro
    glLineWidth(2.5f);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaOro);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glColor3f(1.0f, 0.84f, 0.0f);

    float alturaCentro = cajaAlto / 2.0f + 0.01f; 
    float offsetZ = 0.01f; 

    // Números de la Cara Derecha
    glPushMatrix();
        glTranslatef(cajaAncho/2.0f + offsetZ, alturaCentro - 0.08f, 0.0f);
        glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
        glScalef(-0.01f, 0.01f, 0.01f); // Escalar los caracteres stroke
        for (int i = 0; i < 2; i++) {
            glPushMatrix();
                glTranslatef((i - 0.5f) * 185.0f, 0.0f, 0.0f);
                char numStr[2];
                sprintf(numStr, "%d", combinacion[i]);
                for (char* c = numStr; *c != '\0'; c++) glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
            glPopMatrix();
        }
    glPopMatrix();

    // Números de la Cara Izquierda
    glPushMatrix();
        glTranslatef(-cajaAncho/2.0f - offsetZ, alturaCentro - 0.08f, 0.0f);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        glScalef(-0.01f, 0.01f, 0.01f);
        for (int i = 2; i < 4; i++) {
            glPushMatrix();
                glTranslatef(((i-2) - 0.5f) * 185.0f, 0.0f, 0.0f);
                char numStr[2];
                sprintf(numStr, "%d", combinacion[i]);
                for (char* c = numStr; *c != '\0'; c++) glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
            glPopMatrix();
        }
    glPopMatrix();

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_2D);
    glPopAttrib();
}

/*
 * dibujarEstandarte: Estandarte glorioso final (Puntos 4 y 12).
 * Usa un renderer NURBS para evaluar un plano matemáticamente deformado, 
 * al cual se le aplican coordenadas de textura.
 */
void dibujarEstandarte() {
    if (!cajaDesbloqueada) return;

    float polZ      = cajaProfundo / 2.0f + 3.5f;
    float polAltura = 4.5f;
    float polRadio  = 0.09f;

    glPushMatrix();
    glTranslatef(0.0f, 0.01f, polZ);

    // Mástil
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaOro);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glColor3f(1.0f, 0.84f, 0.0f);

    gluQuadricDrawStyle(qPalo, GLU_FILL);
    gluQuadricNormals(qPalo, GLU_SMOOTH);

    glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(qPalo, polRadio, polRadio * 0.6f, polAltura, 12, 4);
    glPopMatrix();

    glTranslatef(0.0f, polAltura, 0.0f);
    gluQuadricDrawStyle(qBolita, GLU_FILL);
    gluQuadricNormals(qBolita, GLU_SMOOTH);
    gluSphere(qBolita, polRadio * 2.2f, 10, 10);

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_2D);

    // --- Bandera de Papel (Superficie NURBS) ---
    GLfloat matAmb[]  = { 0.75f, 0.65f, 0.45f, 1.0f };
    GLfloat matDif[]  = { 1.0f,  0.90f, 0.70f, 1.0f };
    GLfloat matSpec[] = { 0.15f, 0.15f, 0.15f, 1.0f };
    GLfloat matShin[] = { 8.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   matAmb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   matDif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  matSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShin);
    glColor3f(1.0f, 1.0f, 1.0f);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaPapel);
    glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
    glDisable(GL_CULL_FACE); // Requerido para ver el estandarte por detrás

    GLfloat nudos[8]        = {0,0,0,0, 1,1,1,1}; // 4 puntos de control, orden 4 = 8 nudos (B-Spline)
    GLfloat nudosTex[4]     = {0,0,1,1};
    GLfloat texPts[2][2][2] = {{{0,0},{0,1}}, {{1,0},{1,1}}}; // Coordenadas paramétricas de textura (S, T)

    gluBeginSurface(nurbPergamino);
        // Función generadora de coordenadas de textura sobre el espacio paramétrico de la curva
        gluNurbsSurface(nurbPergamino,
                        4, nudosTex, 4, nudosTex,
                        2*2, 2, &texPts[0][0][0],
                        2, 2, GL_MAP2_TEXTURE_COORD_2);
        // Función generadora de la geometría 3D
        gluNurbsSurface(nurbPergamino,
                        8, nudos, 8, nudos,
                        4*3, 3, &ctrlPointsBanner[0][0][0],
                        4, 4, GL_MAP2_VERTEX_3);
    gluEndSurface(nurbPergamino);

    if (bCull) glEnable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
}

// ============================================================================
// FUNCIONES DE REFLEJO Y SOMBRAS (TÉCNICAS AVANZADAS)
// ============================================================================

/*
 * dibujarReflejoMadera: Crea la ilusión de una superficie pulida mediante "Fake Reflections" (Punto 9).
 * Teoría: Escala la matriz en Y por -1.0, dibujando los objetos debajo del piso.
 * Stencil Buffer (Punto 13): Impide que el reflejo sobresalga del cuadrado interior de la madera, 
 * actuando como un enmascaramiento de píxeles.
 */
void dibujarReflejoMadera() {
    if (tapRotation < 20.0f) return;

    // Blindamos la máquina de estados. Guardamos configuración de stencil, luces y profundidad.
    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);

    // 1. Configuramos el Stencil Buffer para ESCRIBIR la máscara
    glClear(GL_STENCIL_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 1); // Siempre pasa
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); // Reemplaza por 1 donde dibujemos

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // No pintar color en pantalla
    glDepthMask(GL_FALSE); // Tampoco escribir profundidad
    glDepthFunc(GL_LEQUAL);

    // Dibujamos el área permitida para el reflejo (el piso de la caja)
    glPushMatrix();
        glTranslatef(0.0f, cajaAlto/2.0f + 0.01f, 0.0f);
        glBegin(GL_QUADS);
            glVertex3f(-cajaAncho/2,-cajaAlto/2,-cajaProfundo/2);
            glVertex3f( cajaAncho/2,-cajaAlto/2,-cajaProfundo/2);
            glVertex3f( cajaAncho/2,-cajaAlto/2, cajaProfundo/2);
            glVertex3f(-cajaAncho/2,-cajaAlto/2, cajaProfundo/2);
        glEnd();
    glPopMatrix();

    // 2. Configuramos el Stencil Buffer para LEER (Solo dibujar donde hay '1')
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // Volvemos a pintar colores
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); // No modificamos más el stencil
    glDisable(GL_DEPTH_TEST); // El reflejo no obedece al z-buffer normal, es una ilusión 2D

    // Ajuste de luz para que el reflejo parezca opaco y no emita su propia luz
    GLfloat luzTenue[] = { 0.2f, 0.2f, 0.25f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  luzTenue);
    glLightfv(GL_LIGHT0, GL_SPECULAR, luzTenue);

    // 3. Dibujamos la escena INVERTIDA
    glPushMatrix();
        glTranslatef(0.0f,  0.01f, 0.0f);
        glScalef(1.0f, -1.0f, 1.0f); // EL TRUCO: Multiplicamos el eje Y por -1 (Efecto Espejo)
        glTranslatef(0.0f, -0.01f, 0.0f);
        glFrontFace(GL_CW); // Al invertir, las normales se voltean. Arreglamos el culling.
        
        dibujarToroide();
        dibujarPatasToroide();
        dibujarGema();
    glPopMatrix();

    // Restauramos luces originales
    GLfloat luzDifNormal[]  = { 1.0f, 1.0f, 0.95f, 1.0f };
    GLfloat luzSpecNormal[] = { 0.8f, 0.8f, 0.80f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  luzDifNormal);
    glLightfv(GL_LIGHT0, GL_SPECULAR, luzSpecNormal);

    // 4. Capa de "Barniz" mezclada con el reflejo (GL_BLEND)
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaMaderaCaja);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLfloat barnizEspecular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    GLfloat barnizBrillo[]    = { 80.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  barnizEspecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, barnizBrillo);
    glColor4f(1.0f, 1.0f, 1.0f, 0.80f); // 80% opaco, 20% translúcido

    glPushMatrix();
        glTranslatef(0.0f, cajaAlto/2.0f + 0.015f, 0.0f);
        glBegin(GL_QUADS);
            glNormal3f(0,1,0);
            glTexCoord2f(0,1); glVertex3f(-cajaAncho/2,-cajaAlto/2,-cajaProfundo/2);
            glTexCoord2f(1,1); glVertex3f( cajaAncho/2,-cajaAlto/2,-cajaProfundo/2);
            glTexCoord2f(1,0); glVertex3f( cajaAncho/2,-cajaAlto/2, cajaProfundo/2);
            glTexCoord2f(0,0); glVertex3f(-cajaAncho/2,-cajaAlto/2, cajaProfundo/2);
        glEnd();
    glPopMatrix();

    glPopAttrib(); // Fin del bloque protegido
}

/*
 * dibujarSombraCaja: Sombra proyectada al piso (Punto 8 y 13).
 * Multiplica la ModelView actual por una matriz de transformación geométrica especial.
 */
void dibujarSombraCaja() {
    // 1. Crear máscara en el Stencil Buffer donde exista piso
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glBegin(GL_QUADS);
        glNormal3f(0,1,0);
        glVertex3f(-30.0f, 0.0f,-30.0f);
        glVertex3f(-30.0f, 0.0f, 30.0f);
        glVertex3f( 30.0f, 0.0f, 30.0f);
        glVertex3f( 30.0f, 0.0f,-30.0f);
    glEnd();
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // 2. Configurar Stencil para pintar sombra solo 1 VEZ por píxel.
    // Previene el "Double Blending" cuando dos mallas aplastadas se superponen (ej: tapa y caja).
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO); // Tras pintar sombra, escribimos 0 para no volver a pintar ahí
    
    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);
    
    glDisable(GL_DEPTH_TEST); // Dibujamos como "calcomanía" sobre el piso
    glDisable(GL_LIGHTING);   // Las sombras no se iluminan
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.35f); // Tono negro semi-transparente

    GLfloat planeEquation[4] = { 0.0f, 1.0f, 0.0f, 0.0f }; // Ecuación del plano del piso (Y=0)
    GLfloat lightPosArray[4] = { lightPos[0], lightPos[1], lightPos[2], lightPos[3] };
    GLfloat shadowMatrix[16];
    gltMakeShadowMatrix(planeEquation, lightPosArray, shadowMatrix);

    glPushMatrix();
        glMultMatrixf(shadowMatrix); // APLASTAR geometría a partir de aquí
        glPushMatrix();
            glTranslatef(0.0f, cajaAlto/2.0f + 0.01f, 0.0f);
            glBegin(GL_QUADS);
                // Vértices de la Caja (aplastados matemáticamente por la matriz anterior)
                glVertex3f(-cajaAncho/2, cajaAlto/2,-cajaProfundo/2);
                glVertex3f( cajaAncho/2, cajaAlto/2,-cajaProfundo/2);
                glVertex3f( cajaAncho/2,-cajaAlto/2,-cajaProfundo/2);
                glVertex3f(-cajaAncho/2,-cajaAlto/2,-cajaProfundo/2);
                glVertex3f( cajaAncho/2, cajaAlto/2, cajaProfundo/2);
                glVertex3f(-cajaAncho/2, cajaAlto/2, cajaProfundo/2);
                glVertex3f(-cajaAncho/2,-cajaAlto/2, cajaProfundo/2);
                glVertex3f( cajaAncho/2,-cajaAlto/2, cajaProfundo/2);
                glVertex3f(-cajaAncho/2, cajaAlto/2, cajaProfundo/2);
                glVertex3f(-cajaAncho/2, cajaAlto/2,-cajaProfundo/2);
                glVertex3f(-cajaAncho/2,-cajaAlto/2,-cajaProfundo/2);
                glVertex3f(-cajaAncho/2,-cajaAlto/2, cajaProfundo/2);
                glVertex3f(cajaAncho/2, cajaAlto/2,-cajaProfundo/2);
                glVertex3f(cajaAncho/2, cajaAlto/2, cajaProfundo/2);
                glVertex3f(cajaAncho/2,-cajaAlto/2, cajaProfundo/2);
                glVertex3f(cajaAncho/2,-cajaAlto/2,-cajaProfundo/2);
            glEnd();
        glPopMatrix();
        glPushMatrix();
            glTranslatef(0.0f, cajaAlto + 0.01f, 0.0f);
            glTranslatef(cajaAncho/2.0f, 0.0f, 0.0f);
            glRotatef(-tapRotation, 0.0f, 0.0f, 1.0f);
            glTranslatef(-cajaAncho/2.0f, 0.0f, 0.0f);
            glBegin(GL_QUADS);
                // Vértices de la Tapa (las sombras también se animarán automáticamente)
                glVertex3f(-cajaAncho/2, 0, cajaProfundo/2);
                glVertex3f( cajaAncho/2, 0, cajaProfundo/2);
                glVertex3f( cajaAncho/2, 0,-cajaProfundo/2);
                glVertex3f(-cajaAncho/2, 0,-cajaProfundo/2);
            glEnd();
        glPopMatrix();
    glPopMatrix();

    glPopAttrib();
    glDisable(GL_STENCIL_TEST);
}


// ============================================================================
// OBJETOS BASE DE LA ESCENA 
// ============================================================================
void dibujarFoco() {
    glPushMatrix();
        glTranslatef(lightPos[0], lightPos[1], lightPos[2]);
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 1.0f, 0.0f);
        glutSolidSphere(0.3, 12, 12);
        glEnable(GL_LIGHTING);
    glPopMatrix();
}

void dibujarSkybox() {
    float s = skyboxTamanio;
    glPushMatrix();
    glDepthMask(GL_FALSE); // Skybox siempre se dibuja al fondo infinito
    glDisable(GL_LIGHTING);
    GLboolean cullActivo = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);

    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[0]);
    glBegin(GL_QUADS); glTexCoord2f(0,0); glVertex3f( s,-s, s); glTexCoord2f(1,0); glVertex3f( s,-s,-s); glTexCoord2f(1,1); glVertex3f( s, s,-s); glTexCoord2f(0,1); glVertex3f( s, s, s); glEnd();
    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[1]);
    glBegin(GL_QUADS); glTexCoord2f(0,0); glVertex3f(-s,-s,-s); glTexCoord2f(1,0); glVertex3f(-s,-s, s); glTexCoord2f(1,1); glVertex3f(-s, s, s); glTexCoord2f(0,1); glVertex3f(-s, s,-s); glEnd();
    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[2]);
    glBegin(GL_QUADS); glTexCoord2f(0,0); glVertex3f(-s, s, s); glTexCoord2f(1,0); glVertex3f( s, s, s); glTexCoord2f(1,1); glVertex3f( s, s,-s); glTexCoord2f(0,1); glVertex3f(-s, s,-s); glEnd();
    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[3]);
    glBegin(GL_QUADS); glTexCoord2f(0,0); glVertex3f(-s,-s, s); glTexCoord2f(1,0); glVertex3f( s,-s, s); glTexCoord2f(1,1); glVertex3f( s,-s,-s); glTexCoord2f(0,1); glVertex3f(-s,-s,-s); glEnd();
    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[4]);
    glBegin(GL_QUADS); glTexCoord2f(0,0); glVertex3f(-s,-s,-s); glTexCoord2f(1,0); glVertex3f( s,-s,-s); glTexCoord2f(1,1); glVertex3f( s, s,-s); glTexCoord2f(0,1); glVertex3f(-s, s,-s); glEnd();
    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[5]);
    glBegin(GL_QUADS); glTexCoord2f(0,0); glVertex3f(-s,-s, s); glTexCoord2f(1,0); glVertex3f( s,-s, s); glTexCoord2f(1,1); glVertex3f( s, s, s); glTexCoord2f(0,1); glVertex3f(-s, s, s); glEnd();

    if (cullActivo) glEnable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void dibujarPiso() {
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaPisoCristal);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
        // Punto 5: Cálculo y aplicación de normales. 
        // Imprescindible para reaccionar a la luz de GL_LIGHT0.
        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0,0); glVertex3f(-30.0f, 0.0f,-30.0f);
        glTexCoord2f(1,0); glVertex3f(-30.0f, 0.0f, 30.0f);
        glTexCoord2f(1,1); glVertex3f( 30.0f, 0.0f, 30.0f);
        glTexCoord2f(0,1); glVertex3f( 30.0f, 0.0f,-30.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void dibujarCaja() {
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaMaderaCaja);
    glTranslatef(0.0f, cajaAlto/2.0f + 0.01f, 0.0f);
    glColor3f(0.4f, 0.25f, 0.1f);

    // Punto 1: Modelo articulado construido por caras tipo GL_QUADS
    // Frente
    glBegin(GL_QUADS); glNormal3f(0,0,-1);
        glTexCoord2f(0,1); glVertex3f(-cajaAncho/2, cajaAlto/2,-cajaProfundo/2);
        glTexCoord2f(1,1); glVertex3f( cajaAncho/2, cajaAlto/2,-cajaProfundo/2);
        glTexCoord2f(1,0); glVertex3f( cajaAncho/2,-cajaAlto/2,-cajaProfundo/2);
        glTexCoord2f(0,0); glVertex3f(-cajaAncho/2,-cajaAlto/2,-cajaProfundo/2);
    glEnd();
    // Atrás
    glBegin(GL_QUADS); glNormal3f(0,0,1);
        glTexCoord2f(0,1); glVertex3f( cajaAncho/2, cajaAlto/2, cajaProfundo/2);
        glTexCoord2f(1,1); glVertex3f(-cajaAncho/2, cajaAlto/2, cajaProfundo/2);
        glTexCoord2f(1,0); glVertex3f(-cajaAncho/2,-cajaAlto/2, cajaProfundo/2);
        glTexCoord2f(0,0); glVertex3f( cajaAncho/2,-cajaAlto/2, cajaProfundo/2);
    glEnd();
    // Izquierda
    glBegin(GL_QUADS); glNormal3f(-1,0,0);
        glTexCoord2f(0,1); glVertex3f(-cajaAncho/2, cajaAlto/2, cajaProfundo/2);
        glTexCoord2f(1,1); glVertex3f(-cajaAncho/2, cajaAlto/2,-cajaProfundo/2);
        glTexCoord2f(1,0); glVertex3f(-cajaAncho/2,-cajaAlto/2,-cajaProfundo/2);
        glTexCoord2f(0,0); glVertex3f(-cajaAncho/2,-cajaAlto/2, cajaProfundo/2);
    glEnd();
    // Derecha
    glBegin(GL_QUADS); glNormal3f(1,0,0);
        glTexCoord2f(0,1); glVertex3f(cajaAncho/2, cajaAlto/2,-cajaProfundo/2);
        glTexCoord2f(1,1); glVertex3f(cajaAncho/2, cajaAlto/2, cajaProfundo/2);
        glTexCoord2f(1,0); glVertex3f(cajaAncho/2,-cajaAlto/2, cajaProfundo/2);
        glTexCoord2f(0,0); glVertex3f(cajaAncho/2,-cajaAlto/2,-cajaProfundo/2);
    glEnd();
    // Fondo
    glBegin(GL_QUADS); glNormal3f(0,-1,0);
        glTexCoord2f(0,1); glVertex3f(-cajaAncho/2,-cajaAlto/2,-cajaProfundo/2);
        glTexCoord2f(1,1); glVertex3f( cajaAncho/2,-cajaAlto/2,-cajaProfundo/2);
        glTexCoord2f(1,0); glVertex3f( cajaAncho/2,-cajaAlto/2, cajaProfundo/2);
        glTexCoord2f(0,0); glVertex3f(-cajaAncho/2,-cajaAlto/2, cajaProfundo/2);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void dibujarTapa() {
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaTapaMadera);
    
    // Punto 2: Eje de articulación desplazado usando un pivote.
    glTranslatef(0.0f, cajaAlto + 0.01f, 0.0f);
    glTranslatef(cajaAncho/2.0f, 0.0f, 0.0f);
    glRotatef(-tapRotation, 0.0f, 0.0f, 1.0f); // Pivotamos sobre Z en la arista derecha
    glTranslatef(-cajaAncho/2.0f, 0.0f, 0.0f);
    
    glColor3f(0.5f, 0.3f, 0.15f);
    glBegin(GL_QUADS);
        glNormal3f(0,1,0);
        glTexCoord2f(0,0); glVertex3f(-cajaAncho/2, 0,  cajaProfundo/2);
        glTexCoord2f(1,0); glVertex3f( cajaAncho/2, 0,  cajaProfundo/2);
        glTexCoord2f(1,1); glVertex3f( cajaAncho/2, 0, -cajaProfundo/2);
        glTexCoord2f(0,1); glVertex3f(-cajaAncho/2, 0, -cajaProfundo/2);
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}


// ============================================================================
// FUNCIÓN PRINCIPAL DE RENDER (PIPELINE)
// ============================================================================
void display() {
    // Limpieza total del buffer de color, profundidad y esténcil.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glLoadIdentity();

    // Punto 10: Aplicación dinámica de los "Flags" de estado visual
    if (bCull)      glEnable(GL_CULL_FACE);      else glDisable(GL_CULL_FACE);
    if (bDepth)     glEnable(GL_DEPTH_TEST);      else glDisable(GL_DEPTH_TEST);
    if (bWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (bSmooth)    glShadeModel(GL_SMOOTH);      else glShadeModel(GL_FLAT);

    // --- CÁMARA (Punto 3 del Proyecto) ---
    // Trigonometría esférica para mover el Eye Point (Posición de Cámara) en una órbita
    float rX = camAngleX * 0.01745f; // Convertir a radianes
    float rY = camAngleY * 0.01745f;
    float eyeX = camDist * sin(rX) * cos(rY);
    float eyeY = camDist * sin(rY);
    float eyeZ = camDist * cos(rX) * cos(rY);
    gluLookAt(eyeX, eyeY, eyeZ,       // Posición Ojo
              0.0f, 0.0f, 0.0f,       // Punto Central (At)
              0.0f, 1.0f, 0.0f);      // Vector Arriba (Up)

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // ------ LÓGICA DE ANIMACIÓN BASADA EN ESTADO ------
    if (tapIsOpening) {
        if (tapRotation < tapMaxRotation) {
            tapRotation += tapRotationSpeed;
            glutPostRedisplay(); // Solicita un nuevo frame a OpenGL inmediatamente
        }
    } else {
        if (tapRotation > 0.0f) {
            tapRotation -= tapRotationSpeed;
            glutPostRedisplay();
        }
    }

    // ------ RENDERIZADO EN CAPAS ------
    dibujarSkybox();
    dibujarFoco();
    dibujarPiso();
    
    // El orden de dibujado de estos efectos (Sombra y Reflejo) es vital para
    // aprovechar el Depth Buffer y evitar transparencias anómalas.
    dibujarSombraCaja();
    dibujarCaja();
    dibujarTapa();
    dibujarPanelesPuzzle();
    dibujarReflejoMadera();
    dibujarToroide();
    dibujarPatasToroide();
    dibujarGema();
    dibujarEstandarte();

    glutSwapBuffers(); // Manda el frame completo ("Back buffer") a la pantalla ("Front buffer")
}

void myReshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0f, (float)w / h, 1.0f, 2000.0f); // 60° de Field of View
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================================
// FUNCIÓN: procesarSeleccion() — Color Picking (Punto 11 del Proyecto)
// ----------------------------------------------------------------------------
// Teoría: Se pinta la pantalla (Back Buffer) usando colores puros únicos,
// apagando luces y texturas para que no modifiquen el RGB. 
// Luego se lee un solo pixel en las coordenadas X,Y del mouse. 
// Dependiendo del ID (Color), ejecutamos la lógica. El frame nunca se muestra.
// ============================================================================
void procesarSeleccion(int x, int y) {
    // Almacena ABSOLUTAMENTE todo el contexto de OpenGL. Seguro de usar solo aquí (clic único).
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_DITHER); // Evitar difuminado de píxeles

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    float rX = camAngleX * 0.01745f;
    float rY = camAngleY * 0.01745f;
    float eyeX = camDist * sin(rX) * cos(rY);
    float eyeY = camDist * sin(rY);
    float eyeZ = camDist * cos(rX) * cos(rY);
    gluLookAt(eyeX, eyeY, eyeZ, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    // --- ID 1: BASE (Verde RGB: 0, 255, 0) ---
    glColor3ub(0, 255, 0);
    glPushMatrix();
        glTranslatef(0.0f, cajaAlto / 2.0f + 0.01f, 0.0f);
        // Simplificación extrema de la caja (Hitbox)
        glBegin(GL_QUADS);
            glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f( cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f( cajaAncho/2.0f,  cajaAlto/2.0f, cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f, cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, cajaProfundo/2.0f);
            glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f, cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f,  cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);
            glVertex3f(cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f(cajaAncho/2.0f,  cajaAlto/2.0f,  cajaProfundo/2.0f);
            glVertex3f(cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);
            glVertex3f(cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);
            glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);
            glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);
        glEnd();
    glPopMatrix();

    // --- ID 2: TAPA (Rojo RGB: 255, 0, 0) ---
    glColor3ub(255, 0, 0);
    glPushMatrix();
        glTranslatef(0.0f, cajaAlto + 0.01f, 0.0f);
        glTranslatef(cajaAncho/2.0f, 0.0f, 0.0f);
        glRotatef(-tapRotation, 0.0f, 0.0f, 1.0f);
        glTranslatef(-cajaAncho/2.0f, 0.0f, 0.0f);
        glScalef(cajaAncho, 0.2f, cajaProfundo);
        glutSolidCube(1.0f);
    glPopMatrix();

    // --- ID 3: GEMA (Azul RGB: 0, 0, 255) ---
    glColor3ub(0, 0, 255);
    glPushMatrix();
        glTranslatef(0.0f, 2.4f, 0.0f);
        gluQuadricDrawStyle(qPicking, GLU_FILL);
        glPushMatrix();
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            gluCylinder(qPicking, 1.65f, 0.0f, 0.5f, 12, 2);
        glPopMatrix();
        glPushMatrix();
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            gluCylinder(qPicking, 1.65f, 0.0f, 1.20f, 12, 2);
        glPopMatrix();
    glPopMatrix();

    // --- ID MULTIPLE (Puzzle Numérico RGB: 100-103, 0, 0) ---
    float alturaCentro = cajaAlto / 2.0f + 0.01f;

    glPushMatrix();
        glTranslatef(cajaAncho/2.0f + 0.05f, alturaCentro, 0.0f);
        glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
        glScalef(-1.0f, 1.0f, 1.0f);
        for (int i = 0; i < 2; i++) {
            glPushMatrix();
                glTranslatef((i - 0.5f) * 1.85f, 0.5f, 0.0f);
                glColor3ub(102 + i, 0, 0); // Válvulas derecha: IDs 102 y 103
                glBegin(GL_QUADS);
                    glVertex3f(-0.55f, -0.6f, 0.0f);
                    glVertex3f( 0.55f, -0.6f, 0.0f);
                    glVertex3f( 0.55f,  1.0f, 0.0f);
                    glVertex3f(-0.55f,  1.0f, 0.0f);
                glEnd();
            glPopMatrix();
        }
    glPopMatrix();

    glPushMatrix();
        glTranslatef(-cajaAncho/2.0f - 0.05f, alturaCentro, 0.0f);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        glScalef(-1.0f, 1.0f, 1.0f);
        for (int i = 2; i < 4; i++) {
            glPushMatrix();
                glTranslatef(((i-2) - 0.5f) * 1.85f, 0.5f, 0.0f);
                glColor3ub(100 + (i-2), 0, 0); // Válvulas izq: IDs 100 y 101
                glBegin(GL_QUADS);
                    glVertex3f(-0.55f, -0.6f, 0.0f);
                    glVertex3f( 0.55f, -0.6f, 0.0f);
                    glVertex3f( 0.55f,  1.0f, 0.0f);
                    glVertex3f(-0.55f,  1.0f, 0.0f);
                glEnd();
            glPopMatrix();
        }
    glPopMatrix();

    // --- ID 4: ESTANDARTE FINAL (Magenta RGB: 200, 0, 200) ---
    if (cajaDesbloqueada) {
        glColor3ub(200, 0, 200);
        float polZ      = cajaProfundo / 2.0f + 3.5f;
        float polAltura = 4.5f;
        glPushMatrix();
            glTranslatef(0.0f, polAltura - 1.25f, polZ - 1.5f);
            glScalef(0.5f, 2.5f, 3.5f);
            glutSolidCube(1.0f);
        glPopMatrix();
    }

    // LEER EL PÍXEL DEL BACKBUFFER
    unsigned char pixel[3];
    // Invertir Y (El viewport origen de OGL es abajo-izquierda, GLUT reporta desde arriba-izquierda)
    int viewportY = glutGet(GLUT_WINDOW_HEIGHT) - y;
    glReadPixels(x, viewportY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    // --- PROCESAMIENTO MATEMÁTICO DE SELECCIÓN Y AUDIO (Interacción) ---
    if (pixel[0] >= 100 && pixel[0] <= 103 && pixel[1] < 50 && pixel[2] < 50) {
        int indice;
        if (pixel[0] <= 101) indice = pixel[0] - 100 + 2; 
        else                 indice = pixel[0] - 102;      
        combinacion[indice]++;
        if (combinacion[indice] > 9) combinacion[indice] = 1;

        printf("Numero %d ajustado a: %d\n", indice + 1, combinacion[indice]);
        if (audioListo) ma_engine_play_sound(&motorAudio, "assets/audio/numbers/simple_click.mp3", NULL);

        // Si la combinación es 1,2,3,4, se abre el candado
        if (combinacion[0]==1 && combinacion[1]==2 &&
            combinacion[2]==3 && combinacion[3]==4) {
            cajaDesbloqueada = true;
            printf(">>> PUZZLE RESUELTO!\n");
            if (audioListo)
                ma_engine_play_sound(&motorAudio, "assets/audio/puzzle/space_advice_bassEfect.mp3", NULL);
        }
    }
    // Clic en la Tapa articulada
    else if (pixel[0] > 200 && pixel[1] < 50 && pixel[2] < 50) {
        printf("Clic en la TAPA!\n");
        tapRotationSpeed = velocidadRapida; // Altera velocidad (Punto 11a)
        tapIsOpening = !tapIsOpening;
        if (audioListo) {
            if (tapIsOpening) ma_engine_play_sound(&motorAudio, "assets/audio/cofre/abrirCofre.mp3", NULL);
            else              ma_engine_play_sound(&motorAudio, "assets/audio/cofre/cerrarCofre.mp3", NULL);
        }
    }
    // Clic en el cuerpo fijo (Base)
    else if (pixel[0] < 50 && pixel[1] > 200 && pixel[2] < 50) {
        printf("Clic en la BASE! Velocidad normal.\n");
        tapRotationSpeed = velocidadNormal; // Restaurar velocidad (Punto 11b)
    }
    // Clic decorativo Gema
    else if (pixel[0] < 50 && pixel[1] < 50 && pixel[2] > 200) {
        printf("Clic en el DIAMANTE!\n");
        if (audioListo) {
            int randomNum = (rand() % 5) + 1;
            char rutaSonido[100];
            sprintf(rutaSonido, "assets/audio/diamante/tocarDiamante%d.mp3", randomNum);
            ma_engine_play_sound(&motorAudio, rutaSonido, NULL);
        }
    }
    // Clic en el Estandarte Final
    else if (pixel[0] > 150 && pixel[0] < 220 && pixel[1] < 50 && pixel[2] > 150) {
        printf("Clic en el ESTANDARTE!\n");
        if (audioListo) ma_engine_play_sound(&motorAudio, "assets/audio/numbers/simple_click.mp3", NULL);
    }
    else {
        printf("Clic en el vacio.\n");
    }

    // IMPORTANTE: Liberamos la máquina de estados mágicamente, recuperando texturas y luces.
    glPopAttrib(); 
    glutPostRedisplay(); // Forzar el repintado, ahora sí mostrando lo real.
}

// ============================================================================
// CALLBACKS DE TECLADO Y MOUSE
// ============================================================================
void teclado(unsigned char key, int x, int y) {
    switch (key) {
        case 'w': case 'W': if (camDist > 5.0f) camDist -= 1.0f; break; // Zoom in
        case 's': case 'S': camDist += 1.0f; break; // Zoom out
        // Puntos 10a a 10d (Controles Visuales)
        case 'c': case 'C': bCull      = !bCull;      break;
        case 'd': case 'D': bDepth     = !bDepth;     break;
        case 'z': case 'Z': bWireframe = !bWireframe; break;
        case 'x': case 'X': bSmooth    = !bSmooth;    break;
        // Puntos 6a (Posición de luz)
        case '8': lightPos[2] -= 0.5f; break; // Luz Atrás
        case '2': lightPos[2] += 0.5f; break; // Luz Adelante
        case '6': lightPos[0] += 0.5f; break; // Luz Derecha
        case '4': lightPos[0] -= 0.5f; break; // Luz Izquierda
        case '1': lightPos[1] += 0.5f; break; // Luz Arriba
        case '0': if (lightPos[1] > 0.5f) lightPos[1] -= 0.5f; break; // Luz Abajo
        case ' ':
            tapIsOpening = !tapIsOpening;
            glutPostRedisplay();
            break;
        case 27: exit(0); break; // ESC para salir
    }
    glutPostRedisplay();
}

void tecladoEspecial(int key, int x, int y) {
    switch (key) {
        // Movimiento de órbita de cámara alternativo
        case GLUT_KEY_LEFT:  camAngleX -= 3.0f; break;
        case GLUT_KEY_RIGHT: camAngleX += 3.0f; break;
        case GLUT_KEY_UP:    if (camAngleY < 89.0f) camAngleY += 3.0f; break;
        case GLUT_KEY_DOWN:  if (camAngleY >  1.0f) camAngleY -= 3.0f; break;
        // Mapeo extra de opciones F1-F4
        case GLUT_KEY_F1: bCull      = !bCull;      break;
        case GLUT_KEY_F2: bDepth     = !bDepth;     break;
        case GLUT_KEY_F3: bWireframe = !bWireframe; break;
        case GLUT_KEY_F4: bSmooth    = !bSmooth;    break;
    }
    glutPostRedisplay();
}

void mouseBoton(int boton, int estado, int x, int y) {
    if (boton == GLUT_LEFT_BUTTON) {
        if (estado == GLUT_DOWN) {
            mousePresionado = true;
            lastMouseX = x;
            lastMouseY = y;
            procesarSeleccion(x, y); // Detonar algoritmo Color Picking
        } else {
            mousePresionado = false;
        }
    }
    // Rueda del ratón para Zoom In/Out (Eventos de botón en Linux)
    if (boton == 3 && estado == GLUT_DOWN) { if (camDist > 5.0f) camDist -= 1.0f; glutPostRedisplay(); }
    if (boton == 4 && estado == GLUT_DOWN) { camDist += 1.0f; glutPostRedisplay(); }
}

void mouseMovimiento(int x, int y) {
    if (!mousePresionado) return;
    // Multiplicador 0.4f para suavizar sensibilidad
    camAngleX += (x - lastMouseX) * 0.4f;
    camAngleY += (y - lastMouseY) * 0.4f;
    // Bloqueo de Gimbal lock (Zenit y Nadir)
    if (camAngleY >  89.0f) camAngleY =  89.0f;
    if (camAngleY <   1.0f) camAngleY =   1.0f;
    lastMouseX = x;
    lastMouseY = y;
    glutPostRedisplay();
}

// ============================================================================
// CONFIGURACIÓN E INICIALIZACIÓN
// ============================================================================
void cargarTexturasSkybox() {
    glEnable(GL_TEXTURE_2D);
    skyboxTexturas[0] = cargarBMP("assets/1.bmp");
    skyboxTexturas[1] = cargarBMP("assets/2.bmp");
    skyboxTexturas[2] = cargarBMP("assets/3.bmp");
    skyboxTexturas[3] = cargarBMP("assets/4.bmp");
    skyboxTexturas[4] = cargarBMP("assets/5.bmp");
    skyboxTexturas[5] = cargarBMP("assets/6.bmp");
    glDisable(GL_TEXTURE_2D);
}

GLuint cargarBMP(const char* ruta) {
    FILE* archivo = fopen(ruta, "rb");
    if (!archivo) { printf("ERROR: No se pudo abrir: %s\n", ruta); return 0; }

    unsigned char encabezado[54];
    if (fread(encabezado, 1, 54, archivo) != 54 || encabezado[0] != 'B' || encabezado[1] != 'M') {
        printf("ERROR: BMP inválido: %s\n", ruta);
        fclose(archivo); return 0;
    }
    int ancho = encabezado[18] | (encabezado[19]<<8) | (encabezado[20]<<16) | (encabezado[21]<<24);
    int alto  = encabezado[22] | (encabezado[23]<<8) | (encabezado[24]<<16) | (encabezado[25]<<24);

    int tam = ancho * alto * 3;
    unsigned char* datos = (unsigned char*)malloc(tam);
    if (!datos) { printf("ERROR: Sin memoria para: %s\n", ruta); fclose(archivo); return 0; }

    size_t leidos = fread(datos, 1, tam, archivo);
    if (leidos != (size_t)tam) printf("ADVERTENCIA: Lectura incompleta de %s\n", ruta);
    fclose(archivo);

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    // Filtros de Mipmapping básicos
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ancho, alto, 0, GL_BGR, GL_UNSIGNED_BYTE, datos);
    free(datos);
    printf("Textura cargada: %s (%dx%d)\n", ruta, ancho, alto);
    return id;
}

void initNURBS() {
    nurbPergamino = gluNewNurbsRenderer();
    gluNurbsProperty(nurbPergamino, GLU_SAMPLING_TOLERANCE, 25.0); // Tolerancia de suavizado
    gluNurbsProperty(nurbPergamino, GLU_DISPLAY_MODE, GLU_FILL);

    // Inicializamos los puntos de control del pergamino de recompensa
    for (int u = 0; u < 4; u++) {
        for (int v = 0; v < 4; v++) {
            ctrlPointsNurbs[u][v][0] = 3.0f * ((GLfloat)u - 1.5f);
            ctrlPointsNurbs[u][v][1] = (u == 1 || u == 2) ? 1.0f : 0.0f; // Bulto en el centro
            ctrlPointsNurbs[u][v][2] = 3.0f * ((GLfloat)v - 1.5f);
        }
    }

    // Inicializamos Puntos de control del Estandarte que ondea (Bandera lateral)
    float anchoP = 3.0f;
    float altoP  = 2.5f;
    for (int u = 0; u < 4; u++) {
        for (int v = 0; v < 4; v++) {
            ctrlPointsBanner[u][v][0] = (anchoP / 3.0f) * u;  // Crecimiento en X
            ctrlPointsBanner[u][v][1] = -(altoP / 3.0f) * v;  // Caída en Y (gravedad)
            
            // Simulación matemática de tela ondulante en el eje Z
            float onda = 0.0f;
            if (u == 1) onda =  0.2f;
            if (u == 2) onda = -0.15f;
            ctrlPointsBanner[u][v][2] = onda;
        }
    }
}

void initQuadrics() {
    qGema   = gluNewQuadric(); gluQuadricNormals(qGema,    GLU_SMOOTH);
    qPatas  = gluNewQuadric(); gluQuadricNormals(qPatas,   GLU_SMOOTH);
    qPicking= gluNewQuadric(); gluQuadricNormals(qPicking, GLU_NONE);
    qPalo   = gluNewQuadric(); gluQuadricNormals(qPalo,    GLU_SMOOTH);
    qBolita = gluNewQuadric(); gluQuadricNormals(qBolita,  GLU_SMOOTH);
}

void configurarOpenGL() {
    cargarTexturasSkybox();
    texturaMaderaCaja  = cargarBMP("assets/textures/madera/Pino.bmp");
    texturaTapaMadera  = cargarBMP("assets/textures/madera/Wood_Tile.bmp");
    texturaOro         = cargarBMP("assets/textures/metal/metal_gold.bmp");
    texturaPapel       = cargarBMP("assets/textures/papel/texture_paper_old.bmp");
    texturaPisoCristal = cargarBMP("assets/textures/cristal/glass_texture_pure.bmp");

    // Repetición para el material del piso
    glBindTexture(GL_TEXTURE_2D, texturaPisoCristal);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glEnable(GL_NORMALIZE); // Autocalcula las normales al escalar
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE); // Cálculos especulares precisos

    // Ajustes físicos de la Luz global del proyecto (Punto 6)
    GLfloat luzAmbiental[] = { 0.25f, 0.25f, 0.25f, 1.0f };
    GLfloat luzDifusa[]    = { 1.0f,  1.0f,  0.95f, 1.0f };
    GLfloat luzEspecular[] = { 0.8f,  0.8f,  0.8f,  1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT,  luzAmbiental);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  luzDifusa);
    glLightfv(GL_LIGHT0, GL_SPECULAR, luzEspecular);
    
    // Atenuación de luz
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION,  0.8f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION,    0.005f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0f);

    // Ajustes estándar del material del objeto en general (Punto 7)
    GLfloat materialEspecular[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat materialBrillo[]    = { 100.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  materialEspecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, materialBrillo);

    glEnable(GL_COLOR_MATERIAL); // Los glColor actuarán como difusos
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW); // Counter-Clock Wise define las caras frontales

    initNURBS();
    initQuadrics();
}

// ============================================================================
// MAIN
// ============================================================================
int main(int argc, char** argv) {
    srand(time(NULL));
    glutInit(&argc, argv);
    
    // El Contexto solicita Buffers críticos: Doble Buffering, Profundidad y STENCIL
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowSize(800, 600);
    glutCreateWindow("ProyectoFinal: caja enigma - Pablo Perez De Anda");
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Tono lúgubre de fondo

    glutDisplayFunc(display);
    glutReshapeFunc(myReshape);
    glutKeyboardFunc(teclado);
    glutSpecialFunc(tecladoEspecial);
    glutMouseFunc(mouseBoton);
    glutMotionFunc(mouseMovimiento);

    configurarOpenGL();

    // Inicialización API Miniaudio (Linux/Windows)
    if (ma_engine_init(NULL, &motorAudio) == MA_SUCCESS) {
        audioListo = true;
        printf("Audio inicializado correctamente.\n");
    } else {
        printf("ERROR: No se pudo inicializar el motor de audio.\n");
    }

    // Bucle inactivo, render solo reacciona a eventos (CPU Free)
    glutMainLoop();
    return 0;
}

/*
================================================================================
GLOSARIO TÉCNICO Y JUSTIFICACIÓN DEL PROYECTO (DEFENSA FINAL)
================================================================================

I. CUMPLIMIENTO DE LOS 13 PUNTOS DE LA RÚBRICA:
   Para satisfacer íntegramente los requerimientos del profesor Georgii, estructuré
   este código combinando ingeniería de software y matemáticas gráficas:

   1. Modelos de objetos articulados: Implementé la geometría de la caja usando 
      `GL_QUADS` primitivos, creando jerarquías mediante matrices (Pila de OpenGL).
   
   2. Demostrar movimiento: Desarrollé un eje de rotación en la arista de la caja
      usando `glRotatef(-tapRotation...)`. Esta articulación se anima dinámicamente 
      recalculando el ángulo cuando la variable booleana `tapIsOpening` cambia de estado.

   3. Cámara controlable: Apliqué una cámara paramétrica basada en coordenadas polares. 
      Empleé funciones trigonométricas `sin()` y `cos()` para orbitar un "Eye Point" 
      sobre un radio `camDist`. Funciona arrastrando el ratón libremente.

   4. Fragmentos curvos (NURBS): Diseñé un estandarte y pergamino ondulante empleando
      la librería GLU. Usé `gluNurbsSurface` para inyectar una malla bidimensional de
      puntos de control (Control Points, nudos y B-Splines).

   5. Cálculo y aplicación de normales: Establecí explícitamente el vector normal de cada 
      polígono (ej: `glNormal3f(0,-1,0)`) permitiendo a la API realizar los cálculos de
      producto punto necesarios para que la iluminación reaccione a la geometría.

   6. Iluminación (Posición, Ambiente y Especular): Configuré la luz dinámica `GL_LIGHT0`.
      Le doté de un vector `lightPos` reubicable mediante atajos de teclado numérico (1,2,4,6,8,0). 
      El ambiente simula la refracción global y el especular añade reflejos "quemados" al material.

   7. Propiedades de Material: Mejoré la gema superior empleando `glMaterialfv` para sobrescribir
      la respuesta lumínica (`GL_SPECULAR` y `GL_SHININESS`), forzando un brillo metálico.

   8. Sombra a un plano: Proyecté una sombra dinámica calculando la Matriz Shadow 
      (`gltMakeShadowMatrix`) que asimila un vector de plano y la posición de luz para "aplastar" 
      la figura en (Y=0) con un negro de Alpha 0.35 (`GL_BLEND`).

   9. Reflexión mediante Blending: Cautive al jugador con un reflejo pulido. Escribí una 
      técnica que invierte la escala del mundo en su eje Y (`glScalef(1.0f, -1.0f, 1.0f)`), 
      dibuja el objeto de cabeza bajo el piso y finalmente renderiza una capa translúcida superior.

   10. Controles (Shading, Polígonos, Culling, Depth): Asigné las teclas (Z,X,C,D y F1 a F4) 
       para modificar en caliente los estados de la máquina de OpenGL: `glShadeModel`, `glPolygonMode`, 
       `glEnable(GL_CULL_FACE)` y `glEnable(GL_DEPTH_TEST)`.

   11. Selección e interacción (Alterando movimiento): Implementé el algoritmo de Picking 
       por colores. Al cliquear la "Tapa" (Cód: Rojo) modifiqué el flag de velocidad a `velocidadRapida`. 
       Al cliquear la "Base" (Verde), restauré a la normal `velocidadNormal`, cumpliendo 
       perfectamente el sub-rubro "A" y "B".

   12. Textura en Curva: Mapeé una textura estática a mi curva NURBS. Empleé `GL_MAP2_TEXTURE_COORD_2` 
       junto con `gluNurbsSurface` para sincronizar un espacio u-v de texturas 2D sobre la malla B-Spline 3D.

   13. Stencil Test: Utilicé un Buffer de Esténcil (la memoria de máscara) para bloquear anomalías. 
       Para la Sombra: Evita que geometrías "aplastadas y solapadas" multipliquen su oscuridad (Z-fighting). 
       Para el Reflejo: Máscara que restringe renderizar el objeto invertido SOLO en la superficie de la caja.


II. TEORÍA Y TÉCNICAS APLICADAS 

   A. LA MÁQUINA DE ESTADOS DE OPENGL (El FIX de rendimiento)
   Como se describe en la literatura de gráficos, OpenGL no guarda memoria entre frames a 
   menos que se le ordene. Si apago `GL_LIGHTING` para hacer Picking y olvido encenderlo, el 
   siguiente frame se pintará en total oscuridad.
   La IA y la arquitectura sugerían el uso de `glPushAttrib(GL_ALL_ATTRIB_BITS)` para crear 
   backups, sin embargo, en un render-loop a 60fps, pedirle al kernel de video en sistemas AMD 
   almacenar decenas de miles de bits causaba un desbordamiento del ancho de banda y congelamiento 
   del driver en mi entorno Hyprland/Wayland.
   Para curar este "Freeze", actualicé mi estrategia utilizando "MÁSCARAS LIGERAS":
   `glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT...);`
   Esto aisló mis técnicas especiales de reflexión, protegiendo la salud de mi GPU. 

   B. EL ALGORITMO COLOR PICKING (Raycasting alternativo)
   Para evitar integraciones de librerías físicas complejas (AABB OBB Colission, descritas 
   por Millington), implementé "Color Picking". En lugar de lanzar un rayo y buscar la intersección 
   con polígonos, dibujé siluetas simplificadas en un Back-Buffer oculto con colores únicos 
   RGB codificados matemáticamente (ej. R=100, G=0, B=0 representa el botón #3 del candado).
   Cuando el ratón dispara, interrogo al buffer de video (`glReadPixels`) por el color de 
   esa posición específica y ejecuto acciones lógicas. Acto seguido, limpio la pantalla y 
   renderizo las texturas normales reales, eludiendo problemas de CPU.

   C. SUPERFICIES NURBS (Non-Uniform Rational B-Splines)
   Para salir de los modelos toscos articulados y lograr la geometría en tela que solicitaba 
   la UAM, introduje evaluación de Curvas Bezier generalizadas (B-Splines). Mediante los vectores 
   "nudos", ordené a la librería de render de GLU interpolar progresivamente 16 puntos de 
   control (`ctrlPointsBanner`), transformando su posición en los ejes X, Y y especialmente un Z 
   ondulado continuo. Para lograr la textura, evalué de forma entrelazada las coordenadas {0,1} 
   en un plano 2D (`texPts`).

¡Así culminé el desarrollo de mi proyecto final, listo para la auditoría!
================================================================================
*/
