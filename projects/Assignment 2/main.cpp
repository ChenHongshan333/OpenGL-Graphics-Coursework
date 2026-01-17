// CS3241 Assignmeny 2
// Name: Chen Hongshan
// Student ID: A0311136W

#include <cmath>
#include <iostream>
#include <ctime>
#include <thread>
#include <chrono>
#include <deque>
#define GL_SILENCE_DEPRECATION
#define NUM_STARS 200

#ifdef _WIN32
#include <Windows.h>
#include "GL/glut.h"
#define M_PI 3.141592654
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <GLUT/GLUT.h>
#endif

using namespace std;

#define numPlanets 9

class planet {
public:
    float distFromRef;
    float angularSpeed;
    GLfloat color[3];
    float size;
    float angle;
    float alpha;

    planet() {
        distFromRef = 0;
        angularSpeed = 0;
        color[0] = color[1] = color[2] = 0;
        size = 0;
        angle = 0;
        alpha = 1.0;
    }
};

struct Star {
    float x, y;
    float brightness;
};

struct Comet {
    float semiMajor;
    float semiMinor;
    float angularSpeed;
    float angle;
    float size;
    GLfloat color[3];
};

// globals
planet planetList[numPlanets];
Star stars[NUM_STARS];
Comet comet1, comet2;
deque<pair<float, float>> comet1Trail, comet2Trail;
const int maxTrailLength = 50;

GLfloat PI = 3.14159265;
float alpha = 0.0, k = 1;   // rotation + scaling
float tx = 0.0, ty = 0.0;   // translation
bool clockMode = false;
time_t seconds = 0;
struct tm* timeinfo;
float timer = 0.1;

// initialization
void generateStars() {
    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].x = ((float)rand() / RAND_MAX) * 30 - 15;
        stars[i].y = ((float)rand() / RAND_MAX) * 30 - 15;
        stars[i].brightness = 0.5 + ((float)rand() / RAND_MAX) * 0.5;
    }
}

void drawStars() {
    glBegin(GL_POINTS);
    for (int i = 0; i < NUM_STARS; i++) {
        glColor4f(1.0, 1.0, 1.0, stars[i].brightness);
        glVertex2f(stars[i].x, stars[i].y);
    }
    glEnd();
}

void init(void) {
    glClearColor(0.0, 0.0, 0.1, 1.0);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    generateStars();
}

void reshape(int w, int h) {
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-15, 15, -15, 15, -10, 10);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// drawing background 
void drawBackgroundGradient() {
    glBegin(GL_QUADS);
    glColor3f(0.0, 0.0, 0.05);
    glVertex2f(-15, -15);
    glVertex2f(15, -15);
    glColor3f(0.0, 0.0, 0.3);
    glVertex2f(15, 15);
    glVertex2f(-15, 15);
    glEnd();
}

// drawing disk
void drawGradientDisk(float radius, GLfloat r, GLfloat g, GLfloat b, float alpha) {
    int circle_points = 100;
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(r, g, b, alpha); // center
    glVertex2f(0, 0);
    glColor4f(r * 0.3, g * 0.3, b * 0.3, 0.0); // edge
    for (int i = 0; i <= circle_points; i++) {
        float angle = 2 * PI * i / circle_points;
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
}

// drawing comet trail
void drawCometTrail(deque<pair<float, float>>& trail, float c) {
    if (trail.size() < 2) return;
    glBegin(GL_LINE_STRIP);
    int i = 0;
    for (auto& p : trail) {
        float fade = (float)i / trail.size();
        glColor4f(c, c, c, fade * 0.6);
        glVertex2f(p.first, p.second);
        i++;
    }
    glEnd();
}


void generatePlanets() {
    // Sun
    planetList[0].distFromRef = 0;
    planetList[0].angularSpeed = 0;
    planetList[0].color[0] = 1.0; 
    planetList[0].color[1] = 0.7; 
    planetList[0].color[2] = 0.0;
    planetList[0].size = 2.0;

    // Mercury
    planetList[1].distFromRef = 4; 
    planetList[1].angularSpeed = 5;
    planetList[1].color[0] = 0.7; 
    planetList[1].color[1] = 0.7; 
    planetList[1].color[2] = 0.7;
    planetList[1].size = 0.6;

    // Venus
    planetList[2].distFromRef = 5; 
    planetList[2].angularSpeed = 3;
    planetList[2].color[0] = 1.0; 
    planetList[2].color[1] = 0.5; 
    planetList[2].color[2] = 0.2;
    planetList[2].size = 0.6;

    // Earth
    planetList[3].distFromRef = 7; 
    planetList[3].angularSpeed = 2;
    planetList[3].color[0] = 0.2; 
    planetList[3].color[1] = 0.6; 
    planetList[3].color[2] = 1.0;
    planetList[3].size = 0.7;

    // Moon
    planetList[4].distFromRef = 1.5; 
    planetList[4].angularSpeed = 10;
    planetList[4].color[0] = 0.8; 
    planetList[4].color[1] = 0.8; 
    planetList[4].color[2] = 0.8;
    planetList[4].size = 0.2;

    // Mars
    planetList[5].distFromRef = 9; 
    planetList[5].angularSpeed = 1.5;
    planetList[5].color[0] = 1.0; 
    planetList[5].color[1] = 0.3; 
    planetList[5].color[2] = 0.3;
    planetList[5].size = 0.6;

    // Comet1
    comet1.semiMajor = 10.0; 
    comet1.semiMinor = 4.0; 
    comet1.angularSpeed = 15.0; 
    comet1.angle = 0;
    comet1.size = 0.4; 
    comet1.color[0] = 1.0; 
    comet1.color[1] = 1.0; 
    comet1.color[2] = 1.0;

    // Comet2
    comet2.semiMajor = 3.0; 
    comet2.semiMinor = 8.0; 
    comet2.angularSpeed = 19.0; 
    comet2.angle = 45;
    comet2.size = 0.2; 
    comet2.color[0] = 0.86; 
    comet2.color[1] = 0.86; 
    comet2.color[2] = 0.86;
}


void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();

    glScalef(k, k, k);
    glTranslatef(tx, ty, 0);
    glRotatef(alpha, 0, 0, 1);

    drawBackgroundGradient();
    drawStars();

    // Sun
    glPushMatrix();
    drawGradientDisk(planetList[0].size, planetList[0].color[0], planetList[0].color[1], planetList[0].color[2], 1.0);
    glPopMatrix();

    // Mercury, Venus
    for (int i = 1; i <= 2; i++) {
        glPushMatrix();
        glRotatef(planetList[i].angle, 0, 0, 1);
        glTranslatef(0, planetList[i].distFromRef, 0);
        drawGradientDisk(planetList[i].size, planetList[i].color[0], planetList[i].color[1], planetList[i].color[2], 1.0);
        glPopMatrix();
    }

    // Earth + Moon
    glPushMatrix();
    glRotatef(planetList[3].angle, 0, 0, 1);
    glTranslatef(0, planetList[3].distFromRef, 0);
    drawGradientDisk(planetList[3].size, planetList[3].color[0], planetList[3].color[1], planetList[3].color[2], 1.0);

    glPushMatrix();
    glRotatef(planetList[4].angle, 0, 0, 1);
    glTranslatef(0, planetList[4].distFromRef, 0);
    drawGradientDisk(planetList[4].size, planetList[4].color[0], planetList[4].color[1], planetList[4].color[2], 1.0);
    glPopMatrix();
    glPopMatrix();

    // Mars
    glPushMatrix();
    glRotatef(planetList[5].angle, 0, 0, 1);
    glTranslatef(0, planetList[5].distFromRef, 0);
    drawGradientDisk(planetList[5].size, planetList[5].color[0], planetList[5].color[1], planetList[5].color[2], 1.0);
    glPopMatrix();

    // comets
    drawCometTrail(comet1Trail, 1.0f);
    glPushMatrix();
    float cx1 = comet1.semiMajor * cos(comet1.angle * PI / 180.0);
    float cy1 = comet1.semiMinor * sin(comet1.angle * PI / 180.0);
    glTranslatef(cx1, cy1, 0);
    drawGradientDisk(comet1.size, comet1.color[0], comet1.color[1], comet1.color[2], 1.0);
    glPopMatrix();

    drawCometTrail(comet2Trail, 0.86f);
    glPushMatrix();
    float cx2 = comet2.semiMajor * cos(comet2.angle * PI / 180.0);
    float cy2 = comet2.semiMinor * sin(comet2.angle * PI / 180.0);
    glTranslatef(cx2, cy2, 0);
    drawGradientDisk(comet2.size, comet2.color[0], comet2.color[1], comet2.color[2], 1.0);
    glPopMatrix();

    glPopMatrix();
    glFlush();
}


void idle() {
    if (!clockMode) {
        for (int i = 0; i < numPlanets; i++)
            planetList[i].angle += planetList[i].angularSpeed * timer;

        // comet1
        comet1.angle += comet1.angularSpeed * timer;
        float cx1 = comet1.semiMajor * cos(comet1.angle * PI / 180.0);
        float cy1 = comet1.semiMinor * sin(comet1.angle * PI / 180.0);
        comet1Trail.push_back({ cx1,cy1 });
        if (comet1Trail.size() > maxTrailLength) comet1Trail.pop_front();

        // comet2
        comet2.angle += comet2.angularSpeed * timer;
        float cx2 = comet2.semiMajor * cos(comet2.angle * PI / 180.0);
        float cy2 = comet2.semiMinor * sin(comet2.angle * PI / 180.0);
        comet2Trail.push_back({ cx2,cy2 });
        if (comet2Trail.size() > maxTrailLength) comet2Trail.pop_front();
    }
    else {
        seconds = time(NULL);
        timeinfo = localtime(&seconds);

		// Earth + Moon -> second hand
        planetList[3].angle = -((float)timeinfo->tm_sec) * 6; // Earth
        planetList[4].angle += planetList[4].angularSpeed * timer; // Moon

		// Mars -> minute hand
        planetList[5].angle = -((float)timeinfo->tm_min + (float)timeinfo->tm_sec / 60.0) * 6.0; // Mars

		// Mercury + Venus -> hour hand
        planetList[1].angle = -((timeinfo->tm_hour % 12) + (float)timeinfo->tm_min / 60.0) * 30.0; // Mercury
        planetList[2].angle = -((timeinfo->tm_hour % 12) + (float)timeinfo->tm_min / 60.0) * 30.0; // Venus
    }

    glutPostRedisplay();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
}


void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 27: case 'q': case 'Q': exit(0);
    case 't': case 'T':
        clockMode = !clockMode;
        cout << "Current Mode: " << (clockMode ? "Clock" : "Solar") << " mode.\n";
        break;
    case '+': case '=': k *= 1.1; break;
    case '-': case '_': k /= 1.1; break;
    case 'w': ty += 0.5; break;
    case 's': ty -= 0.5; break;
    case 'a': tx -= 0.5; break;
    case 'd': tx += 0.5; break;
    default: break;
    }
    glutPostRedisplay();
}


int main(int argc, char** argv) {
    cout << "CS3241 Lab 2\n";
    cout << "Toggle Time Mode: T\nScale: +/-, Translate: W/A/S/D, Exit: ESC/q\n";

    generatePlanets();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(600, 600);
    glutInitWindowPosition(50, 50);
    glutCreateWindow(argv[0]);
    init();
    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMainLoop();

    return 0;
}
