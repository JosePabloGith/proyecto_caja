/*
 * Alumno : Jose pablo Perez De Anda
 * Materia: Graficas Por Computadora
 * Profesor: Georgii Khachaturov
 *
 * Proyecto : proyecto_caja (Caja Enigmática estilo The Room)
 *
 * Control de versiones:
 * 1.- Establecimos el esqueleto inicial
 * 2.- Sistema de cámara con rotación polar
 * 3.- Plano base (piso) con normales
 * 4.- Sombras
 * 5.- Limpieza de código y organización
 * 6.- Implementacion del sistema de puzle Temporal
 * 7.- [MERGE] Quadrics globales para eficiencia + restauracion de estandarte,
 * material del pergamino, guard del reflejo y animacion bajo demanda (fix CPU)
 * 8.- [FIX] Blindaje de Máquina de Estados (Picking) y Optimización Geométrica.
 * 9.- [FIX CRÍTICO] Eliminacion de GL_ALL_ATTRIB_BITS en el Render Loop para evitar
 * saturacion del driver Mesa (AMD) y prevenir freezes durante el movimiento de camara.
 * 10.- [RESTORE] Se restauraron las funciones de dibujo estáticas (Caja, Skybox, Piso, Tapa).
 */

// ============================================================================
// INCLUDES
// ============================================================================
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <GL/glut.h>

#define MINIAUDIO_IMPLEMENTATION
#include "include/miniaudio.h"


// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

ma_engine motorAudio;
bool audioListo = false;

GLuint skyboxTexturas[6];
float skyboxTamanio = 200.0f;

GLuint texturaMaderaCaja  = 0;
GLuint texturaTapaMadera  = 0;
GLuint texturaPisoCristal = 0;
GLuint texturaOro         = 0;
GLuint texturaPapel       = 0;

float velocidadNormal = 1.5f;
float velocidadRapida = 5.0f;

// ------ CÁMARA ------
float camDist   = 25.0f;
float camAngleX =  0.0f;
float camAngleY = 20.0f;

int  lastMouseX      = 0;
int  lastMouseY      = 0;
bool mousePresionado = false;

float lightPos[4] = { 5.0f, 10.0f, 5.0f, 1.0f };

// ------ PUZZLE y NURBS ------
int  combinacion[4]       = {1,1,1,1};
bool cajaDesbloqueada     = false;
GLUnurbsObj* nurbPergamino = NULL;
GLfloat ctrlPointsNurbs[4][4][3];   // Puntos de control del pergamino

// Puntos de control del estandarte (bandera)
GLfloat ctrlPointsBanner[4][4][3];

// Quadrics globales — se crean UNA sola vez
GLUquadricObj* qGema       = NULL;
GLUquadricObj* qPatas      = NULL;
GLUquadricObj* qPicking    = NULL;
GLUquadricObj* qPalo       = NULL;   // Palo del estandarte
GLUquadricObj* qBolita     = NULL;   // Bolita decorativa del estandarte

// ------ FLAGS DE VISUALIZACIÓN ------
bool bCull      = false;
bool bDepth     = true;
bool bWireframe = false;
bool bSmooth    = false;

// ------ DIMENSIONES DE LA CAJA ------
float cajaAncho    = 6.0f;
float cajaProfundo = 8.0f;
float cajaAlto     = 5.0f;

// ------ TAPA ------
float tapRotation      = 0.0f;
float tapMaxRotation   = 120.0f;
float tapRotationSpeed = 1.5f;
bool  tapIsOpening     = false;

// ------ GEMA ------
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
// UTILIDAD: Shadow Matrix
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
// FUNCIONES DE DIBUJO
// ============================================================================

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
    
    // [FIX DE RENDIMIENTO] Reducción de polígonos del toroide (de 20x72 a 12x36).
    glutSolidTorus(0.135f, 1.4f, 12, 36);

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}


void dibujarGema() {
    glPushMatrix();
    glTranslatef(0.0f, gemaY, 0.0f);

    // --- PARTE 1: Sólido texturizado ---
    GLfloat gemaEspecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat gemaBrillo[]    = { 128.0f };
    GLfloat gemaAmbiente[]  = { 0.1f, 0.2f, 0.6f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   gemaAmbiente);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  gemaEspecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, gemaBrillo);

    glColor4f(0.2f, 0.5f, 1.0f, 0.85f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaMaderaCaja);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
    GLfloat blendColor[] = { 0.2f, 0.5f, 1.0f, 1.0f };
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, blendColor);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);

    // [FIX DE ESTADO] Aseguramos que la gema sólida siempre se dibuje con FILL
    gluQuadricDrawStyle(qGema, GLU_FILL);

    float R  = gemaRadius;
    float h1 = 0.5f;
    float h2 = 1.20f;

    // [FIX DE RENDIMIENTO] Reducción de segmentos a 16
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

    // --- PARTE 2: Aristas ---
    // Protegemos el estado de línea y luces para el resto del programa
    glPushAttrib(GL_LIGHTING_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);
    glColor4f(0.3f, 0.6f, 1.0f, 0.20f);
    
    // Cambiamos a líneas
    gluQuadricDrawStyle(qGema, GLU_LINE);

    glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(qGema, R, 0.0f, h1, 16, 4);
    glPopMatrix();
    glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(qGema, R, 0.0f, h2, 16, 4);
    glPopMatrix();

    // [FIX DE ESTADO] Restauramos el quadric global a FILL para otros usos
    gluQuadricDrawStyle(qGema, GLU_FILL);

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
    float posiciones[4][2] = {
        { radio,  0.0f},
        {-radio,  0.0f},
        { 0.0f,  radio},
        { 0.0f, -radio}
    };

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


void dibujarPanelesPuzzle() {
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
    glDisable(GL_LIGHTING);
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

    glPushMatrix();
        glTranslatef(cajaAncho/2.0f + offsetZ, alturaCentro - 0.08f, 0.0f);
        glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
        glScalef(-0.01f, 0.01f, 0.01f);
        for (int i = 0; i < 2; i++) {
            glPushMatrix();
                glTranslatef((i - 0.5f) * 185.0f, 0.0f, 0.0f);
                char numStr[2];
                sprintf(numStr, "%d", combinacion[i]);
                for (char* c = numStr; *c != '\0'; c++)
                    glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
            glPopMatrix();
        }
    glPopMatrix();

    glPushMatrix();
        glTranslatef(-cajaAncho/2.0f - offsetZ, alturaCentro - 0.08f, 0.0f);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        glScalef(-0.01f, 0.01f, 0.01f);
        for (int i = 2; i < 4; i++) {
            glPushMatrix();
                glTranslatef(((i-2) - 0.5f) * 185.0f, 0.0f, 0.0f);
                char numStr[2];
                sprintf(numStr, "%d", combinacion[i]);
                for (char* c = numStr; *c != '\0'; c++)
                    glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
            glPopMatrix();
        }
    glPopMatrix();

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_2D);
    glPopAttrib();
}


void dibujarPergamino() {
    if (!cajaDesbloqueada) return;

    glPushMatrix();
    glTranslatef(0.0f, 6.0f, 0.0f);

    glColor3f(1.0f, 1.0f, 1.0f);
    GLfloat matAmbiente[]  = { 0.8f, 0.7f, 0.5f, 1.0f };
    GLfloat matDifuso[]    = { 1.0f, 0.9f, 0.7f, 1.0f };
    GLfloat matEspecular[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat matBrillo[]    = { 10.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   matAmbiente);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   matDifuso);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  matEspecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matBrillo);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaPapel);
    glEnable(GL_AUTO_NORMAL);

    GLfloat nudos[8]        = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};
    GLfloat nudosTex[4]     = {0.0, 0.0, 1.0, 1.0};
    GLfloat texPts[2][2][2] = {{{0.0, 0.0}, {0.0, 1.0}}, {{1.0, 0.0}, {1.0, 1.0}}};

    gluBeginSurface(nurbPergamino);
        gluNurbsSurface(nurbPergamino, 4, nudosTex, 4, nudosTex,
                        2*2, 2, &texPts[0][0][0], 2, 2, GL_MAP2_TEXTURE_COORD_2);
        gluNurbsSurface(nurbPergamino, 8, nudos, 8, nudos,
                        4*3, 3, &ctrlPointsNurbs[0][0][0], 4, 4, GL_MAP2_VERTEX_3);
    gluEndSurface(nurbPergamino);

    glDisable(GL_AUTO_NORMAL);
    if (bCull) glEnable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
}


void dibujarEstandarte() {
    if (!cajaDesbloqueada) return;

    float polZ      = cajaProfundo / 2.0f + 3.5f;
    float polAltura = 4.5f;
    float polRadio  = 0.09f;

    glPushMatrix();
    glTranslatef(0.0f, 0.01f, polZ);

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
    glDisable(GL_CULL_FACE);

    GLfloat nudos[8]        = {0,0,0,0, 1,1,1,1};
    GLfloat nudosTex[4]     = {0,0,1,1};
    GLfloat texPts[2][2][2] = {{{0,0},{0,1}}, {{1,0},{1,1}}};

    gluBeginSurface(nurbPergamino);
        gluNurbsSurface(nurbPergamino,
                        4, nudosTex, 4, nudosTex,
                        2*2, 2, &texPts[0][0][0],
                        2, 2, GL_MAP2_TEXTURE_COORD_2);
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
// FUNCIONES CON SOLUCIÓN DEL BUG (REFLEJO Y SOMBRAS)
// ============================================================================
void dibujarReflejoMadera() {
    if (tapRotation < 20.0f) return;

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);

    glClear(GL_STENCIL_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);

    glPushMatrix();
        glTranslatef(0.0f, cajaAlto/2.0f + 0.01f, 0.0f);
        glBegin(GL_QUADS);
            glVertex3f(-cajaAncho/2,-cajaAlto/2,-cajaProfundo/2);
            glVertex3f( cajaAncho/2,-cajaAlto/2,-cajaProfundo/2);
            glVertex3f( cajaAncho/2,-cajaAlto/2, cajaProfundo/2);
            glVertex3f(-cajaAncho/2,-cajaAlto/2, cajaProfundo/2);
        glEnd();
    glPopMatrix();

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glDisable(GL_DEPTH_TEST);

    GLfloat luzTenue[] = { 0.2f, 0.2f, 0.25f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  luzTenue);
    glLightfv(GL_LIGHT0, GL_SPECULAR, luzTenue);

    glPushMatrix();
        glTranslatef(0.0f,  0.01f, 0.0f);
        glScalef(1.0f, -1.0f, 1.0f);
        glTranslatef(0.0f, -0.01f, 0.0f);
        glFrontFace(GL_CW);
        dibujarToroide();
        dibujarPatasToroide();
        dibujarGema();
    glPopMatrix();

    GLfloat luzDifNormal[]  = { 1.0f, 1.0f, 0.95f, 1.0f };
    GLfloat luzSpecNormal[] = { 0.8f, 0.8f, 0.80f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  luzDifNormal);
    glLightfv(GL_LIGHT0, GL_SPECULAR, luzSpecNormal);

    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaMaderaCaja);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLfloat barnizEspecular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    GLfloat barnizBrillo[]    = { 80.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  barnizEspecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, barnizBrillo);
    glColor4f(1.0f, 1.0f, 1.0f, 0.80f);

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

    glPopAttrib();
}


void dibujarSombraCaja() {
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

    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
    
    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);
    
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.35f);

    GLfloat planeEquation[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
    GLfloat lightPosArray[4] = { lightPos[0], lightPos[1], lightPos[2], lightPos[3] };
    GLfloat shadowMatrix[16];
    gltMakeShadowMatrix(planeEquation, lightPosArray, shadowMatrix);

    glPushMatrix();
        glMultMatrixf(shadowMatrix);
        glPushMatrix();
            glTranslatef(0.0f, cajaAlto/2.0f + 0.01f, 0.0f);
            glBegin(GL_QUADS);
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
// OBJETOS DE LA ESCENA (RESTAURADOS)
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
    glDepthMask(GL_FALSE);
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
    glTranslatef(0.0f, cajaAlto + 0.01f, 0.0f);
    glTranslatef(cajaAncho/2.0f, 0.0f, 0.0f);
    glRotatef(-tapRotation, 0.0f, 0.0f, 1.0f);
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
// FUNCIÓN: display()
// ============================================================================
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glLoadIdentity();

    if (bCull)      glEnable(GL_CULL_FACE);      else glDisable(GL_CULL_FACE);
    if (bDepth)     glEnable(GL_DEPTH_TEST);      else glDisable(GL_DEPTH_TEST);
    if (bWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (bSmooth)    glShadeModel(GL_SMOOTH);      else glShadeModel(GL_FLAT);

    // ------ CÁMARA ------
    float rX = camAngleX * 0.01745f;
    float rY = camAngleY * 0.01745f;
    float eyeX = camDist * sin(rX) * cos(rY);
    float eyeY = camDist * sin(rY);
    float eyeZ = camDist * cos(rX) * cos(rY);
    gluLookAt(eyeX, eyeY, eyeZ,
              0.0f, 0.0f, 0.0f,
              0.0f, 1.0f, 0.0f);

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // ------ ANIMACIÓN DE TAPA ------
    if (tapIsOpening) {
        if (tapRotation < tapMaxRotation) {
            tapRotation += tapRotationSpeed;
            glutPostRedisplay();
        }
    } else {
        if (tapRotation > 0.0f) {
            tapRotation -= tapRotationSpeed;
            glutPostRedisplay();
        }
    }

    // ------ RENDERIZADO ------
    dibujarSkybox();
    dibujarFoco();
    dibujarPiso();
    dibujarSombraCaja();
    dibujarCaja();
    dibujarTapa();
    dibujarPanelesPuzzle();
    dibujarReflejoMadera();
    dibujarToroide();
    dibujarPatasToroide();
    dibujarGema();
    dibujarEstandarte();

    glutSwapBuffers();
}


// ============================================================================
// FUNCIÓN: myReshape()
// ============================================================================
void myReshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0f, (float)w / h, 1.0f, 2000.0f);
    glMatrixMode(GL_MODELVIEW);
}


// ============================================================================
// FUNCIÓN: procesarSeleccion() — Color Picking
// ============================================================================
void procesarSeleccion(int x, int y) {
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_DITHER);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    float rX = camAngleX * 0.01745f;
    float rY = camAngleY * 0.01745f;
    float eyeX = camDist * sin(rX) * cos(rY);
    float eyeY = camDist * sin(rY);
    float eyeZ = camDist * cos(rX) * cos(rY);
    gluLookAt(eyeX, eyeY, eyeZ, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    // --- BASE (Verde) ---
    glColor3ub(0, 255, 0);
    glPushMatrix();
        glTranslatef(0.0f, cajaAlto / 2.0f + 0.01f, 0.0f);
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

    // --- TAPA (Rojo) ---
    glColor3ub(255, 0, 0);
    glPushMatrix();
        glTranslatef(0.0f, cajaAlto + 0.01f, 0.0f);
        glTranslatef(cajaAncho/2.0f, 0.0f, 0.0f);
        glRotatef(-tapRotation, 0.0f, 0.0f, 1.0f);
        glTranslatef(-cajaAncho/2.0f, 0.0f, 0.0f);
        glScalef(cajaAncho, 0.2f, cajaProfundo);
        glutSolidCube(1.0f);
    glPopMatrix();

    // --- GEMA (Azul) ---
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

    // --- HITBOXES DE NÚMEROS ---
    float alturaCentro = cajaAlto / 2.0f + 0.01f;

    glPushMatrix();
        glTranslatef(cajaAncho/2.0f + 0.05f, alturaCentro, 0.0f);
        glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
        glScalef(-1.0f, 1.0f, 1.0f);
        for (int i = 0; i < 2; i++) {
            glPushMatrix();
                glTranslatef((i - 0.5f) * 1.85f, 0.5f, 0.0f);
                glColor3ub(102 + i, 0, 0); 
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
                glColor3ub(100 + (i-2), 0, 0); 
                glBegin(GL_QUADS);
                    glVertex3f(-0.55f, -0.6f, 0.0f);
                    glVertex3f( 0.55f, -0.6f, 0.0f);
                    glVertex3f( 0.55f,  1.0f, 0.0f);
                    glVertex3f(-0.55f,  1.0f, 0.0f);
                glEnd();
            glPopMatrix();
        }
    glPopMatrix();

    // --- HITBOX ESTANDARTE (Magenta) ---
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

    // --- LEER EL PÍXEL ---
    unsigned char pixel[3];
    int viewportY = glutGet(GLUT_WINDOW_HEIGHT) - y;
    glReadPixels(x, viewportY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    // --- LÓGICA DE SELECCIÓN ---
    if (pixel[0] >= 100 && pixel[0] <= 103 && pixel[1] < 50 && pixel[2] < 50) {
        int indice;
        if (pixel[0] <= 101) indice = pixel[0] - 100 + 2; 
        else                 indice = pixel[0] - 102;      
        combinacion[indice]++;
        if (combinacion[indice] > 9) combinacion[indice] = 1;

        printf("Numero %d ajustado a: %d\n", indice + 1, combinacion[indice]);
        if (audioListo)
            ma_engine_play_sound(&motorAudio, "assets/audio/numbers/simple_click.mp3", NULL);

        if (combinacion[0]==1 && combinacion[1]==2 &&
            combinacion[2]==3 && combinacion[3]==4) {
            cajaDesbloqueada = true;
            printf(">>> PUZZLE RESUELTO!\n");
            if (audioListo)
                ma_engine_play_sound(&motorAudio, "assets/audio/puzzle/space_advice_bassEfect.mp3", NULL);
        }
    }
    else if (pixel[0] > 200 && pixel[1] < 50 && pixel[2] < 50) {
        printf("Clic en la TAPA!\n");
        tapRotationSpeed = velocidadRapida;
        tapIsOpening = !tapIsOpening;
        if (audioListo) {
            if (tapIsOpening) ma_engine_play_sound(&motorAudio, "assets/audio/cofre/abrirCofre.mp3", NULL);
            else              ma_engine_play_sound(&motorAudio, "assets/audio/cofre/cerrarCofre.mp3", NULL);
        }
    }
    else if (pixel[0] < 50 && pixel[1] > 200 && pixel[2] < 50) {
        printf("Clic en la BASE! Velocidad normal.\n");
        tapRotationSpeed = velocidadNormal;
    }
    else if (pixel[0] < 50 && pixel[1] < 50 && pixel[2] > 200) {
        printf("Clic en el DIAMANTE!\n");
        if (audioListo) {
            int randomNum = (rand() % 5) + 1;
            char rutaSonido[100];
            sprintf(rutaSonido, "assets/audio/diamante/tocarDiamante%d.mp3", randomNum);
            ma_engine_play_sound(&motorAudio, rutaSonido, NULL);
        }
    }
    else if (pixel[0] > 150 && pixel[0] < 220 && pixel[1] < 50 && pixel[2] > 150) {
        printf("Clic en el ESTANDARTE!\n");
        if (audioListo)
            ma_engine_play_sound(&motorAudio, "assets/audio/numbers/simple_click.mp3", NULL);
    }
    else {
        printf("Clic en el vacio.\n");
    }

    glPopAttrib(); 
    glutPostRedisplay();
}


// ============================================================================
// CALLBACKS DE TECLADO Y MOUSE
// ============================================================================
void teclado(unsigned char key, int x, int y) {
    switch (key) {
        case 'w': case 'W': if (camDist > 5.0f) camDist -= 1.0f; break;
        case 's': case 'S': camDist += 1.0f; break;
        case 'c': case 'C': bCull      = !bCull;      break;
        case 'd': case 'D': bDepth     = !bDepth;     break;
        case 'z': case 'Z': bWireframe = !bWireframe; break;
        case 'x': case 'X': bSmooth    = !bSmooth;    break;
        case '8': lightPos[2] -= 0.5f; break;
        case '2': lightPos[2] += 0.5f; break;
        case '6': lightPos[0] += 0.5f; break;
        case '4': lightPos[0] -= 0.5f; break;
        case '1': lightPos[1] += 0.5f; break;
        case '0': if (lightPos[1] > 0.5f) lightPos[1] -= 0.5f; break;
        case ' ':
            tapIsOpening = !tapIsOpening;
            glutPostRedisplay();
            break;
        case 27: exit(0); break;
    }
    glutPostRedisplay();
}

void tecladoEspecial(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_LEFT:  camAngleX -= 3.0f; break;
        case GLUT_KEY_RIGHT: camAngleX += 3.0f; break;
        case GLUT_KEY_UP:    if (camAngleY < 89.0f) camAngleY += 3.0f; break;
        case GLUT_KEY_DOWN:  if (camAngleY >  1.0f) camAngleY -= 3.0f; break;
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
            procesarSeleccion(x, y);
        } else {
            mousePresionado = false;
        }
    }
    if (boton == 3 && estado == GLUT_DOWN) { if (camDist > 5.0f) camDist -= 1.0f; glutPostRedisplay(); }
    if (boton == 4 && estado == GLUT_DOWN) { camDist += 1.0f; glutPostRedisplay(); }
}

void mouseMovimiento(int x, int y) {
    if (!mousePresionado) return;
    camAngleX += (x - lastMouseX) * 0.4f;
    camAngleY += (y - lastMouseY) * 0.4f;
    if (camAngleY >  89.0f) camAngleY =  89.0f;
    if (camAngleY <   1.0f) camAngleY =   1.0f;
    lastMouseX = x;
    lastMouseY = y;
    glutPostRedisplay();
}

// ============================================================================
// INICIALIZACIÓN
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
    gluNurbsProperty(nurbPergamino, GLU_SAMPLING_TOLERANCE, 25.0);
    gluNurbsProperty(nurbPergamino, GLU_DISPLAY_MODE, GLU_FILL);

    // --- Puntos de control del PERGAMINO (plano curvo) ---
    for (int u = 0; u < 4; u++) {
        for (int v = 0; v < 4; v++) {
            ctrlPointsNurbs[u][v][0] = 3.0f * ((GLfloat)u - 1.5f);
            ctrlPointsNurbs[u][v][1] = (u == 1 || u == 2) ? 1.0f : 0.0f;
            ctrlPointsNurbs[u][v][2] = 3.0f * ((GLfloat)v - 1.5f);
        }
    }

    float anchoP = 3.0f;
    float altoP  = 2.5f;
    for (int u = 0; u < 4; u++) {
        for (int v = 0; v < 4; v++) {
            ctrlPointsBanner[u][v][0] = (anchoP / 3.0f) * u;  // X: 0 → anchoP (sale del palo)
            ctrlPointsBanner[u][v][1] = -(altoP / 3.0f) * v;  // Y: cuelga hacia abajo
            // Onda suave en Z para simular tela flameando
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

    glBindTexture(GL_TEXTURE_2D, texturaPisoCristal);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glEnable(GL_NORMALIZE);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    GLfloat luzAmbiental[] = { 0.25f, 0.25f, 0.25f, 1.0f };
    GLfloat luzDifusa[]    = { 1.0f,  1.0f,  0.95f, 1.0f };
    GLfloat luzEspecular[] = { 0.8f,  0.8f,  0.8f,  1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT,  luzAmbiental);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  luzDifusa);
    glLightfv(GL_LIGHT0, GL_SPECULAR, luzEspecular);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION,  0.8f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION,    0.005f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0f);

    GLfloat materialEspecular[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat materialBrillo[]    = { 100.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  materialEspecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, materialBrillo);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);

    initNURBS();
    initQuadrics();
}


// ============================================================================
// MAIN
// ============================================================================
int main(int argc, char** argv) {
    srand(time(NULL));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowSize(800, 600);
    glutCreateWindow("ProyectoFinal: caja enigma");
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    glutDisplayFunc(display);
    glutReshapeFunc(myReshape);
    glutKeyboardFunc(teclado);
    glutSpecialFunc(tecladoEspecial);
    glutMouseFunc(mouseBoton);
    glutMotionFunc(mouseMovimiento);

    configurarOpenGL();

    if (ma_engine_init(NULL, &motorAudio) == MA_SUCCESS) {
        audioListo = true;
        printf("Audio inicializado correctamente.\n");
    } else {
        printf("ERROR: No se pudo inicializar el motor de audio.\n");
    }

    glutMainLoop();
    return 0;
}
