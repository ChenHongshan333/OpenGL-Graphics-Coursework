// name: Hongshan
// Extra functions used: glPointSize(), glutSwapBuffers(), glutKeyboardFunc()

#include <cmath>
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#include "GL/glut.h"
#define M_PI 3.141592654
#elif __APPLE__
#include <OpenGL/gl.h>
#include <GLUT/GLUT.h>
#endif

using namespace std;

GLfloat PI = 3.14;
float alpha = 0.0f, k = 1.0f;
float tx = 0.0f, ty = 0.0f;

void drawCircle(float r, int num_segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.0f, 0.0f);
    for (int i = 0; i <= num_segments; i++) {
        float theta = 2.0f * M_PI * i / num_segments;
        float x = r * cos(theta);
        float y = r * sin(theta);
        glVertex2f(x, y);
    }
    glEnd();
}

void drawTriangle() {
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.4f / 2, 0.0f);
    glVertex2f(0.4f / 2, 0.0f);
    glVertex2f(0.0f, 0.35f);
    glEnd();
}

void drawArc(float r, float theta_start, float theta_end, int num_segments) {
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= num_segments; i++) {
        float theta = theta_start + (theta_end - theta_start) * i / num_segments;
        float x = r * cos(theta);
        float y = -r * sin(theta) - r;
        glVertex2f(x, y);
    }
    glEnd();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Apply global transformations
    glPushMatrix();
    glTranslatef(tx, ty, 0.0f);
    glRotatef(alpha, 0, 0, 1);
    glScalef(k, k, 1.0f);

    // face
    glPushMatrix();
    glColor3f(1.0f, 1.0f, 1.0f);
    glScalef(1.1f, 1.0f, 1.0f);
    drawCircle(0.5f, 100);
    glPopMatrix();

    // blue areas on head
    glPushMatrix();
    glColor3f(0.2f, 0.6f, 0.9f);
    glTranslatef(0.25f, 0.30f, 0.0f);
    glRotatef(-37, 0, 0, 1);
    glScalef(1.1f, 0.49f, 1.0f);
    drawCircle(0.35f, 100);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.2f, 0.6f, 0.9f);
    glTranslatef(-0.25f, 0.30f, 0.0f);
    glRotatef(37, 0, 0, 1);
    glScalef(1.1f, 0.49f, 1.0f);
    drawCircle(0.35f, 100);
    glPopMatrix();

    // ears
    glPushMatrix();
    glColor3f(0.2f, 0.6f, 0.9f);
    glTranslatef(-0.25f, 0.4f, 0.0f);
    glRotatef(20, 0, 0, 1);
    drawTriangle();
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.2f, 0.6f, 0.9f);
    glTranslatef(0.25f, 0.4f, 0.0f);
    glRotatef(-20, 0, 0, 1);
    drawTriangle();
    glPopMatrix();

    // blush
    glPushMatrix();
    glColor3f(1.0f, 0.7f, 0.7f);
    glTranslatef(-0.3f, -0.05f, 0.0f);
    glScalef(1.3f, 0.7f, 1.0f);
    drawCircle(0.1f, 50);
    glPopMatrix();

    glPushMatrix();
    glColor3f(1.0f, 0.7f, 0.7f);
    glTranslatef(0.3f, -0.05f, 0.0f);
    glScalef(1.3f, 0.7f, 1.0f);
    drawCircle(0.1f, 50);
    glPopMatrix();

    // eyes
    glPushMatrix();
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(6.0f);
    glTranslatef(-0.23f, 0.08f, 0.0f);
    glRotatef(13, 0, 0, 1);
    drawArc(0.12f, 1.25 * M_PI, 1.8 * M_PI, 1000);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(6.0f);
    glTranslatef(0.23f, 0.08f, 0.0f);
    glRotatef(-13, 0, 0, 1);
    drawArc(0.12f, 1.2 * M_PI, 1.75 * M_PI, 1000);
    glPopMatrix();

    // mouth
    glPushMatrix();
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(4.0f);
    glTranslatef(-0.036f, 0.0f, 0.0f);
    glRotatef(12, 0, 0, 1);
    drawArc(0.03f, 0, M_PI, 100);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(4.0f);
    glTranslatef(0.036f, 0.0f, 0.0f);
    glRotatef(-12, 0, 0, 1);
    drawArc(0.03f, 0, M_PI, 100);
    glPopMatrix();

    // eyebrows
    glPushMatrix();
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(4.0f);
    glTranslatef(-0.17f, 0.22f, 0.0f);
    glRotatef(12, 0, 0, 1);
    drawArc(0.08f, 1.4 * M_PI, 1.6 * M_PI, 100);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(4.0f);
    glTranslatef(0.17f, 0.22f, 0.0f);
    glRotatef(-12, 0, 0, 1);
    drawArc(0.08f, 1.4 * M_PI, 1.6 * M_PI, 100);
    glPopMatrix();

    // jaw
    glPushMatrix();
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(3.0f);
    glTranslatef(0.0f, 0.02f, 0.0f);
    drawArc(0.07f, 0.4 * M_PI, 0.6 * M_PI, 100);
    glPopMatrix();

    // whisker dots
    glPushMatrix();
    glColor3f(0.0f, 0.0f, 0.0f);
    glPointSize(6.0f);
    glBegin(GL_POINTS);
    glVertex2f(-0.4f, -0.02f);
    glVertex2f(0.4f, -0.02f);
    glEnd();
    glPopMatrix();

    glPopMatrix(); // end global transformations

    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 'z': alpha += 10; break;           // rotate anticlockwise
    case 'c': alpha -= 10; break;           // rotate clockwise
    case 'q': k += 0.1; break;              // zoom in
    case 'e': if (k > 0.1) k -= 0.1; break; // zoom out
    case 'a': tx -= 0.1; break;             // left
    case 'd': tx += 0.1; break;             // right
    case 's': ty -= 0.1; break;             // down
    case 'w': ty += 0.1; break;             // up
    default: break;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(600, 600);
    glutCreateWindow("Cute Cat (OpenGL with Keyboard Control)");
    glClearColor(0.5f, 0.6f, 0.7f, 1.0f);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);  // register keyboard callback
    glutMainLoop();
    return 0;
}
