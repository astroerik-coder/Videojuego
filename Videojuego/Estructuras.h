#include <GL/glut.h>


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