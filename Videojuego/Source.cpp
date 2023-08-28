#include <GL/freeglut.h>
#include <cmath>
#include <stdio.h>
#include<iostream>
#include <vector>
#include <sstream>
#include <thread>
#include <chrono>

//Sonido
#pragma comment(lib, "irrKlang.lib") // Esto vincula automáticamente la biblioteca irrKlang
#include "irrKlang.h"

//Texturas
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" 

using namespace std;
using namespace irrklang;

//Variables globales
#define PI 3.14159265358979323846
#define WALL_SIZE 2.0
#define MAP_SIZE 10
#define PLAYER_SIZE 0.5

//Texturas
GLuint texture[1];
unsigned char* data1;
unsigned char* data2;
unsigned char* data3;
unsigned char* data4;
int width, height, nrChannels;

////Sonido
ISoundEngine* engine; // Declaración del motor de sonido
ISound* currentBackgroundMusic; // Puntero al sonido de fondo actual
vector<ISound*> backgroundMusics;

//Jugador
int totalPoints = 0;
GLfloat playerX = 2.5;
GLfloat playerY = 0.5;
GLfloat playerZ = 2.5;

GLfloat playerAngle = 0.0;
GLfloat cameraDistance = 1.7;

//Mouse
int prevMouseX = 0;
int prevMouseY = 0;
bool isMousePressed = false;
GLfloat moveSpeed = 0.1;
bool sphereCollided = false;
int collectedCoins = 0;

//Variables de movimiento de la esfera
GLfloat sphereVerticalPosition = 0.0;
GLfloat sphereVerticalSpeed = 0.0009; // Velocidad vertical más lenta
GLfloat sphereVerticalDirection = 1.0; // Inicialmente, las esferas se mueven hacia arriba

//Variables para definir las vidas restantes
int vidasRestantes = 5;
int colisionParedes = 0;

//Sierpinski
GLfloat vertices[4][3] = { {0.0, 0.942809, -0.333333},  // Cambio de las coordenadas
                          {-0.816497, -0.471405, -0.333333},
                          {0.816497, -0.471405, -0.333333},
                          {0.0, 0.0, 1.0} };  // Coordenadas de la punta superior

//Rotacion
float rotationX = 0.0f;
float rotationY = 0.0f;
float rotationSpeed = 0.4f; // Velocidad de rotación horizontal


struct Point {
    GLfloat x, y, z;
};

struct Texture {
    GLuint id;
    int width, height, nrChannels;
};

struct GameObject {
    Point position;
    GLfloat width, height, depth;
    Texture texture;
};

struct Player {
    Point position;
    GLfloat angle, moveSpeed, cameraDistance;
    int collectedCoins, totalPoints;
};

// Estructura para representar una esfera
struct Sphere {
    Point position;  // Cambio: Agregar la posición de la esfera
    GLfloat radius;
    bool collided;

    // Constructor de Sphere
    Sphere(GLfloat _x, GLfloat _y, GLfloat _z, GLfloat _radius)
        : position({ _x, _y, _z }), radius(_radius), collided(false) {}  // Cambio: Inicializar la posición
};

// Estructura para representar un rectángulo
struct Rectangulo {
    Point position;
    GLfloat width, height, depth;

    // Constructor de Rectangulo
    Rectangulo(GLfloat _x, GLfloat _y, GLfloat _z, GLfloat _width, GLfloat _height, GLfloat _depth)
        : position({ _x, _y, _z }), width(_width), height(_height), depth(_depth) {}  // Cambio: Inicializar la posición
};

//Vectores de figuras
vector<Sphere> spheres;
vector<Rectangulo> rectangles;

//Reproducir sonido
void playSoundEffect(const char* soundFilePath) {
    ISound* soundEffect = engine->play2D(soundFilePath, false, false, true);
    if (soundEffect) {
        soundEffect->setVolume(0.1); // Ajusta el volumen del efecto de sonido si es necesario
        soundEffect->drop(); // Liberar el puntero al efecto de sonido
    }
}

void triangle(GLfloat* a, GLfloat* b, GLfloat* c) {
    glVertex3fv(a);
    glVertex3fv(b);
    glVertex3fv(c);
}

void divide_triangle(GLfloat* a, GLfloat* b, GLfloat* c, int m) {
    GLfloat v1[3], v2[3], v3[3];
    int j;
    if (m > 0) {
        for (j = 0; j < 3; j++) {
            v1[j] = (a[j] + b[j]) / 2;
            v2[j] = (a[j] + c[j]) / 2;
            v3[j] = (b[j] + c[j]) / 2;
        }
        divide_triangle(a, v1, v2, m - 1);
        divide_triangle(c, v2, v3, m - 1);
        divide_triangle(b, v3, v1, m - 1);
    }
    else {
        triangle(a, b, c);
    }
}

void tetrahedron(int m) {
    glColor3f(1.0, 0.0, 0.0);
    divide_triangle(vertices[0], vertices[1], vertices[2], m);
    glColor3f(0.0, 1.0, 0.0);
    divide_triangle(vertices[3], vertices[2], vertices[1], m);
    glColor3f(0.0, 0.0, 1.0);
    divide_triangle(vertices[0], vertices[3], vertices[1], m);
    glColor3f(1.0, 1.0, 0.0);
    divide_triangle(vertices[0], vertices[2], vertices[3], m);
}

//Cargar textura
void cargarTextura(unsigned char* archivo, GLuint& texture, int textureWidth, int textureHeight) {
    
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, archivo);
    stbi_image_free(archivo);
}

//Dibujo del rectangulo cara a cara
void drawRectangle(GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height, GLfloat depth) {
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture[3]); // Usamos la primera textura

    glPushMatrix();
    glTranslatef(x, y, z);

    // Configuración del material para el rectángulo
    GLfloat rectangleMaterialDiffuse[] = { 1.0, 1.0, 1.0, 1.0 }; // Color difuso blanco
    glMaterialfv(GL_FRONT, GL_DIFFUSE, rectangleMaterialDiffuse);

    // Front face
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex3f(-width / 2, -height / 2, depth / 2);
    glTexCoord2f(1.0, 0.0); glVertex3f(width / 2, -height / 2, depth / 2);
    glTexCoord2f(1.0, 1.0); glVertex3f(width / 2, height / 2, depth / 2);
    glTexCoord2f(0.0, 1.0); glVertex3f(-width / 2, height / 2, depth / 2);
    glEnd();

    // Back face
    glBegin(GL_QUADS);
    glTexCoord2f(1.0, 0.0); glVertex3f(-width / 2, -height / 2, -depth / 2);
    glTexCoord2f(1.0, 1.0); glVertex3f(-width / 2, height / 2, -depth / 2);
    glTexCoord2f(0.0, 1.0); glVertex3f(width / 2, height / 2, -depth / 2);
    glTexCoord2f(0.0, 0.0); glVertex3f(width / 2, -height / 2, -depth / 2);
    glEnd();

    // Left face
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 1.0); glVertex3f(-width / 2, height / 2, depth / 2);
    glTexCoord2f(0.0, 0.0); glVertex3f(-width / 2, -height / 2, depth / 2);
    glTexCoord2f(1.0, 0.0); glVertex3f(-width / 2, -height / 2, -depth / 2);
    glTexCoord2f(1.0, 1.0); glVertex3f(-width / 2, height / 2, -depth / 2);
    glEnd();

    // Right face
    glBegin(GL_QUADS);
    glTexCoord2f(1.0, 1.0); glVertex3f(width / 2, height / 2, depth / 2);
    glTexCoord2f(0.0, 1.0); glVertex3f(width / 2, -height / 2, depth / 2);
    glTexCoord2f(0.0, 0.0); glVertex3f(width / 2, -height / 2, -depth / 2);
    glTexCoord2f(1.0, 0.0); glVertex3f(width / 2, height / 2, -depth / 2);
    glEnd();

    // Top face
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex3f(-width / 2, height / 2, depth / 2);
    glTexCoord2f(1.0, 0.0); glVertex3f(width / 2, height / 2, depth / 2);
    glTexCoord2f(1.0, 1.0); glVertex3f(width / 2, height / 2, -depth / 2);
    glTexCoord2f(0.0, 1.0); glVertex3f(-width / 2, height / 2, -depth / 2);
    glEnd();

    // Bottom face
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 1.0); glVertex3f(-width / 2, -height / 2, depth / 2);
    glTexCoord2f(0.0, 0.0); glVertex3f(-width / 2, -height / 2, -depth / 2);
    glTexCoord2f(1.0, 0.0); glVertex3f(width / 2, -height / 2, -depth / 2);
    glTexCoord2f(1.0, 1.0); glVertex3f(width / 2, -height / 2, depth / 2);
    glEnd();

    glPopMatrix();
    glFlush();
    glDisable(GL_TEXTURE_2D); // Deshabilitar mapeo de texturas después de usarlo
}

// Dibujar el suelo con textura
void drawGround() {
    glEnable(GL_TEXTURE_2D); // Habilitar mapeo de texturas
    glBindTexture(GL_TEXTURE_2D, texture[2]); // Usar la textura en el índice 0

    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex3f(0.0, -0.1, 0.0);
    glTexCoord2f(1.0, 0.0); glVertex3f(MAP_SIZE * WALL_SIZE, -0.1, 0.0);
    glTexCoord2f(1.0, 1.0); glVertex3f(MAP_SIZE * WALL_SIZE, -0.1, MAP_SIZE * WALL_SIZE);
    glTexCoord2f(0.0, 1.0); glVertex3f(0.0, -0.1, MAP_SIZE * WALL_SIZE);
    glEnd();

    glDisable(GL_TEXTURE_2D); // Deshabilitar mapeo de texturas después de usarlo
}

//Dibujar el jugador en este caso un cuadrado
void drawPlayer() {

    GLfloat playerLightPosition[] = { playerX + 0.2, playerY, playerZ, 1.0 };
    GLfloat playerLightAmbiental[] = { 0.0, 0.0, 0.0, 1.0 }; // Mayor intensidad de luz ambiental
    GLfloat playerLightDiffuse[] = { 0.8, 0.8, 0.8, 1.0 }; // Mayor intensidad de luz
    GLfloat playerLightSpecular[] = { 1.0, 1.0, 1.0, 1.0 }; // Mayor intensidad de luz especular

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glLightfv(GL_LIGHT0, GL_POSITION, playerLightPosition);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, playerLightDiffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT, playerLightAmbiental);
    glLightfv(GL_LIGHT0, GL_SPECULAR, playerLightSpecular);

    glPushMatrix();
    glTranslatef(playerX, PLAYER_SIZE / 4.0, playerZ);
    glRotatef(playerAngle, 0.0, 1.0, 0.0);

    glDisable(GL_COLOR_MATERIAL);

    GLfloat playerMaterialDiffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat playerMaterialSpecularFront[] = { 1.0, 1.0, 1.0, 1.0 }; // Brillante en la cara frontal
    GLfloat playerMaterialSpecular[] = { 0.9, 0.9, 0.9, 1.0 }; // Menos especular en otras caras
    GLfloat playerMaterialShininess[] = { 100.0 };

    glMaterialfv(GL_FRONT, GL_DIFFUSE, playerMaterialDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, playerMaterialSpecular);
    glMaterialfv(GL_FRONT, GL_SHININESS, playerMaterialShininess);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture[3]);

    // Dibujar cara frontal
    glBegin(GL_QUADS);
    glNormal3f(0.0, 0.0, 1.0);
    glTexCoord2f(0.0, 0.0); glVertex3f(-PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0);
    glTexCoord2f(1.0, 0.0); glVertex3f(PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0);
    glTexCoord2f(1.0, 1.0); glVertex3f(PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0);
    glTexCoord2f(0.0, 1.0); glVertex3f(-PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0);
    glEnd();

    // Dibujar cara posterior
    glBegin(GL_QUADS);
    glNormal3f(0.0, 0.0, -1.0);
    glTexCoord2f(0.0, 0.0); glVertex3f(PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0);
    glTexCoord2f(1.0, 0.0); glVertex3f(-PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0);
    glTexCoord2f(1.0, 1.0); glVertex3f(-PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0);
    glTexCoord2f(0.0, 1.0); glVertex3f(PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0);
    glEnd();

    // Dibujar cara izquierda
    glBegin(GL_QUADS);
    glNormal3f(-1.0, 0.0, 0.0);
    glTexCoord2f(0.0, 0.0); glVertex3f(-PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0);
    glTexCoord2f(1.0, 0.0); glVertex3f(-PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0);
    glTexCoord2f(1.0, 1.0); glVertex3f(-PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0);
    glTexCoord2f(0.0, 1.0); glVertex3f(-PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0);
    glEnd();

    // Dibujar cara derecha
    glBegin(GL_QUADS);
    glNormal3f(1.0, 0.0, 0.0);
    glTexCoord2f(0.0, 0.0); glVertex3f(PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0);
    glTexCoord2f(1.0, 0.0); glVertex3f(PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0);
    glTexCoord2f(1.0, 1.0); glVertex3f(PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0);
    glTexCoord2f(0.0, 1.0); glVertex3f(PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0);
    glEnd();

    // Dibujar cara superior
    glBegin(GL_QUADS);
    glNormal3f(0.0, 1.0, 0.0);
    glTexCoord2f(0.0, 0.0); glVertex3f(-PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0);
    glTexCoord2f(1.0, 0.0); glVertex3f(PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0);
    glTexCoord2f(1.0, 1.0); glVertex3f(PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0);
    glTexCoord2f(0.0, 1.0); glVertex3f(-PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0);
    glEnd();

    // Dibujar cara inferior
    glBegin(GL_QUADS);
    glNormal3f(0.0, -1.0, 0.0);
    glTexCoord2f(0.0, 0.0); glVertex3f(-PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0);
    glTexCoord2f(1.0, 0.0); glVertex3f(PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0);
    glTexCoord2f(1.0, 1.0); glVertex3f(PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0);
    glTexCoord2f(0.0, 1.0); glVertex3f(-PLAYER_SIZE / 2.0, -PLAYER_SIZE / 2.0, PLAYER_SIZE / 2.0);
    glEnd();

    glPopMatrix();
    glDisable(GL_TEXTURE_2D); // Deshabilitar mapeo de texturas después de usarlo
}

//Dibujo de la esfera
void drawSphere(GLfloat x, GLfloat y, GLfloat z, GLfloat radius) {
    glPushMatrix();
    glTranslatef(x, y + sphereVerticalPosition, z);

    glEnable(GL_TEXTURE_2D); // Habilitar mapeo de texturas
    glBindTexture(GL_TEXTURE_2D, texture[0]); // Usar la textura en el índice 0

    // Configurar coordenadas de textura para cada vértice de la esfera
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);

    // Configurar materiales y luces para efecto brillante
    GLfloat sphereMaterialSpecular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat sphereMaterialShininess[] = { 100.0 };
    GLfloat sphereLightPosition[] = { x, y + sphereVerticalPosition, z, 1.0 };
    GLfloat sphereLightDiffuse[] = { 1.0, 1.0, 1.0, 1.0 };

    glMaterialfv(GL_FRONT, GL_SPECULAR, sphereMaterialSpecular);
    glMaterialfv(GL_FRONT, GL_SHININESS, sphereMaterialShininess);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT1);

    glLightfv(GL_LIGHT1, GL_POSITION, sphereLightPosition);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, sphereLightDiffuse);

    glutSolidSphere(radius, 20, 20); // Dibujar la esfera

    // Deshabilitar configuración de textura y luces
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHT1);
    glDisable(GL_LIGHTING);

    glPopMatrix();
}

//FUNCIONES DE COLISIONES CON ESFERA Y RECTANGULOS
void handleSphereCollision(Sphere& sphere) {
    GLfloat distanceX = playerX - sphere.position.x;
    GLfloat distanceY = playerY - sphere.position.y;
    GLfloat distanceZ = playerZ - sphere.position.z;
    GLfloat distance = sqrt(distanceX * distanceX + distanceY * distanceY + distanceZ * distanceZ);

    if (distance <= sphere.radius + PLAYER_SIZE / 2) {
        totalPoints++;
        collectedCoins++; // Incrementa el contador de monedas recolectadas
        playSoundEffect("moneda.mp3");
        cout << "El total de puntos es: " << totalPoints << endl;
        sphere.collided = true;
    }
}


//Reiniciar juego
void resetGame() {
    playerX = 2.5;
    playerY = 0.5;
    playerZ = 2.5;
    collectedCoins = 0; // Reinicia el contador de monedas recolectadas
    for (Sphere& sphere : spheres) {
        sphere.collided = false;
    }
}

// Dentro de la función checkPlayerRectangleCollision
bool checkPlayerRectangleCollision(const Rectangulo& rectangle) {
    if (playerX + PLAYER_SIZE / 2 >= rectangle.position.x - rectangle.width / 2 &&
        playerX - PLAYER_SIZE / 2 <= rectangle.position.x + rectangle.width / 2 &&
        playerY + PLAYER_SIZE / 2 >= rectangle.position.y - rectangle.height / 2 &&
        playerY - PLAYER_SIZE / 2 <= rectangle.position.y + rectangle.height / 2 &&
        playerZ + PLAYER_SIZE / 2 >= rectangle.position.z - rectangle.depth / 2 &&
        playerZ - PLAYER_SIZE / 2 <= rectangle.position.z + rectangle.depth / 2) {

        vidasRestantes--;
        if (vidasRestantes == 0) {
            cout << "Haz muerto" << endl;

        }
        else {
            cout << "Chocaste, te quedan: " << vidasRestantes << " vidas." << endl;
            playSoundEffect("golpe.mp3");
            resetGame(); // Reiniciar el juego al colisionar con el rectángulo
        }
        return true;
    }
    return false;
}

void drawHUD() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    string mensaje = "Vidas: " + to_string(vidasRestantes) + "\n Puntos: " + to_string(totalPoints);

    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(10, glutGet(GLUT_WINDOW_HEIGHT) - 20); // Posición del mensaje en pantalla
    for (char c : mensaje) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c); // Fuente y tamaño del mensaje
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void display() {
    glClearColor(0.0, 0.0, 0.0, 1.0); // Fondo negro
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();


    //Configuracion de luces generales del escenario
    GLfloat ambientLight[] = { 0.2, 0.2, 0.2, 1.0 }; // Luz ambiental más brillante. Mayores valores lo vuelven mas brillante
    GLfloat diffuseLight[] = { 0.5, 0.5, 0.5, 1.0 };

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT5);

    glLightfv(GL_LIGHT5, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT5, GL_DIFFUSE, diffuseLight);


    GLfloat cameraX = playerX - cameraDistance * sin(playerAngle * PI / 180);
    GLfloat cameraY = playerY + PLAYER_SIZE / 2;
    GLfloat cameraZ = playerZ - cameraDistance * cos(playerAngle * PI / 180);

    gluLookAt(
        cameraX, cameraY, cameraZ,
        playerX, playerY + PLAYER_SIZE / 4, playerZ,
        0.0, 1.0, 0.0
    );

    //Movimiento de la esfera verticalmente
    sphereVerticalPosition += sphereVerticalSpeed * sphereVerticalDirection;

    // Invierte la dirección si la esfera alcanza ciertos límites
    //Al cambiar los valores hace que la longitud del movimiento sea menor o mayor
    if (sphereVerticalPosition >= 0.07 || sphereVerticalPosition <= -0.07) {
        sphereVerticalDirection *= -1.0;
    }

    drawGround();
    drawPlayer();

    for (Sphere& sphere : spheres) {
        if (!sphere.collided) {
            handleSphereCollision(sphere);
            drawSphere(sphere.position.x, sphere.position.y, sphere.position.z, sphere.radius);
        }
    }

    for (const Rectangulo& rectangle : rectangles) {
        if (checkPlayerRectangleCollision(rectangle)) {
            resetGame(); // Reiniciar el juego al colisionar con el rectángulo
        }
        drawRectangle(rectangle.position.x, rectangle.position.y, rectangle.position.z, rectangle.width, rectangle.height, rectangle.depth);
    }

    // Dibujar el mensaje en la parte superior izquierda
    drawHUD();

    // Renderizar el fractal de Sierpinski en 3D
    glPushMatrix();
    // Ajustar la posición y la escala del fractal según tus necesidades
    glTranslatef(9.5f, 2.0f, 8.5f); // Ajusta la posición en Z
    glScalef(2.0f, 2.0f, 2.0f);       // Ajusta la escala

    // Rotación automática horizontal
    rotationY += rotationSpeed;
    glRotatef(rotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotationY, 0.0f, 1.0f, 0.0f);

    glBegin(GL_TRIANGLES);
    tetrahedron(5);
    glEnd();
    glPopMatrix();

    glutSwapBuffers();
    glutPostRedisplay();
}

//FUNCIONES DE CONFIGURACION, MOUSE Y TECLADO
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLfloat)w / (GLfloat)h, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            isMousePressed = true;
            prevMouseX = x;
            prevMouseY = y;
        }
        else {
            isMousePressed = false;
        }
    }
}

void mouseMotion(int x, int y) {
    if (isMousePressed) {
        GLfloat deltaX = x - prevMouseX;
        playerAngle += deltaX * 0.1;
        prevMouseX = x;
        prevMouseY = y;
        glutPostRedisplay();
    }
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 'w':
        playerX += moveSpeed * sin(playerAngle * PI / 180);
        playerZ += moveSpeed * cos(playerAngle * PI / 180);
        break;
    case 's':
        playerX -= moveSpeed * sin(playerAngle * PI / 180);
        playerZ -= moveSpeed * cos(playerAngle * PI / 180);
        break;
    case 'a':
        playerAngle += 5.0;
        break;
    case 'd':
        playerAngle -= 5.0;
        break;
    case 13: // Código ASCII para la tecla Enter
        cout << "Posicion del cubo X: " << playerX << endl;
        cout << "Posicion del cubo Y: " << playerY << endl;
        cout << "Posicion del cubo Z: " << playerZ << endl;
        break;

    case 27:
        exit(0);
        break;
    }
    glutPostRedisplay();
}

int initTextures() {
    data1 = stbi_load("images/anime.jpg", &width, &height, &nrChannels, 0);
    if (!data1) {
        cout << "Error al carga la textura 1" << endl;
        return 1;
    }

    data2 = stbi_load("images/009.png", &width, &height, &nrChannels, 0);
    if (!data2) {
        cout << "Error al carga la textura 2" << endl;
        return 1;
    }

    data3 = stbi_load("images/suelo.jpg", &width, &height, &nrChannels, 0);
    if (!data3) {
        cout << "Error al carga la textura 3" << endl;
        return 1;
    }

    data4 = stbi_load("images/jugador.png", &width, &height, &nrChannels, 0);
    if (!data4) {
        cout << "Error al carga la textura 4" << endl;
        return 1;
    }
}

int main(int argc, char** argv) {
    
    initTextures();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Videojuego");
    glEnable(GL_DEPTH_TEST);

    engine = createIrrKlangDevice(); // Inicializar el motor de sonido
    // Reproducir la música de fondo (ajustar la ruta según la ubicación de tu archivo)
    ISound* music = engine->play2D("ambiente.mp3", true, false, true);
    // Ajustar el volumen de la música 
    if (music) {
        music->setVolume(0.2);
    }

    // Cargar las texturas
    cargarTextura(data1, texture[0], width, height);
    cargarTextura(data2, texture[1], width, height);
    cargarTextura(data3, texture[2], width, height);
    cargarTextura(data4, texture[3], width, height);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotion);
    glutKeyboardFunc(keyboard);

    spheres.push_back(Sphere(3.0, 0.2, 8.0, 0.15));
    spheres.push_back(Sphere(7.0, 0.2, 7.0, 0.2));
    spheres.push_back(Sphere(12.0, 0.2, 7.0, 0.2));
    spheres.push_back(Sphere(15.0, 0.2, 12.0, 0.2));

    rectangles.push_back(Rectangulo(MAP_SIZE * WALL_SIZE / 2, 0.5, 0.0, MAP_SIZE * WALL_SIZE, WALL_SIZE, 0.1));
    rectangles.push_back(Rectangulo(MAP_SIZE * WALL_SIZE / 2, 0.5, MAP_SIZE * WALL_SIZE, MAP_SIZE * WALL_SIZE, WALL_SIZE, 0.1));
    rectangles.push_back(Rectangulo(0.0, 0.5, MAP_SIZE * WALL_SIZE / 2, 0.1, WALL_SIZE, MAP_SIZE * WALL_SIZE));
    rectangles.push_back(Rectangulo(MAP_SIZE * WALL_SIZE, 0.5, MAP_SIZE * WALL_SIZE / 2, 0.1, WALL_SIZE, MAP_SIZE * WALL_SIZE));

    // Agregar obstáculos en el suelo
    rectangles.push_back(Rectangulo(7.0, 0.5, 3.0, 1.0, 1.0, 1.0));
    rectangles.push_back(Rectangulo(3.0, 0.5, 7.0, 1.0, 1.0, 1.0));
    rectangles.push_back(Rectangulo(11.0, 0.5, 5.5, 1.0, 1.0, 1.0));
    rectangles.push_back(Rectangulo(13.0, 0.5, 11.0, 1.0, 1.0, 1.0));

    glutMainLoop();

    if (music) {
        music->drop();
        music = 0;
    }

    engine->drop(); // Liberar el motor de sonido

    //Liberar las texturas
    stbi_image_free(data1);
    stbi_image_free(data2);
    stbi_image_free(data3);
    stbi_image_free(data4);

    return 0;
}