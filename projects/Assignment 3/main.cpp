// CS3241 Assignment 2: Let there be light
// Name: Chen Hongshan
// Matric No.: A0311136W

#include <cmath>
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#include "GL/glut.h"
#ifndef M_PI
#define M_PI 3.141592654
#endif
#elif __APPLE__
#include <OpenGL/gl.h>
#include <GLUT/GLUT.h>
#ifndef M_PI
#define M_PI 3.141592654
#endif
#else
#include <GL/gl.h>
#include <GL/glut.h>
#ifndef M_PI
#define M_PI 3.141592654
#endif
#endif

using namespace std;

// global variable
bool m_Smooth = false;      // not smooth by default, which can be switched by S
bool m_Highlight = false;   // no highlight by defult, which can be switched by H
GLfloat angle = 0;   /* in degrees */
GLfloat angle2 = 0;  /* in degrees */
GLfloat zoom = 1.0;
int mouseButton = 0;
int moving, startx, starty;

#define NO_OBJECT 4
int current_object = 0;

// camera parameters
GLfloat camEyeX = 0, camEyeY = 0, camEyeZ = 6;   // camera position
GLfloat camCenterX = 0, camCenterY = 0, camCenterZ = 0; // look-at target
GLfloat camUpX = 0, camUpY = 1, camUpZ = 0;      // up vector

GLfloat camNear = 1.0f;
GLfloat camFar = 80.0f;
GLfloat camFovy = 40.0f; // field of view in degree

// camera setup
void updateCamera() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(camFovy, 1.0, camNear, camFar);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(camEyeX, camEyeY, camEyeZ, camCenterX, camCenterY, camCenterZ, camUpX, camUpY, camUpZ);
}

// lighting setup
void setupLighting()
{
    // FLAT shading by default
    glShadeModel(GL_FLAT);

    // normalize
    glEnable(GL_NORMALIZE);

    // Lights, material properties
    GLfloat ambientProperties[] = { 0.1f, 0.1f, 0.3f, 1.0f };
    GLfloat diffuseProperties[] = { 0.8f, 0.8f, 1.0f, 1.0f };
    GLfloat specularProperties[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat lightPosition[] = { -100.0f, 100.0f, 100.0f, 1.0f };

    glClearDepth(1.0);

    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientProperties);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseProperties);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularProperties);
    glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, 0.0);

    // Default : lighting enabled
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
}

// sphere with normals
void drawSphere(double r)
{
    // remove: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  

    int i, j;
    int n = 30; // resolution: increase for smoother sphere

    // We compute vertex positions on a unit sphere, then multiply by r for scaling purpose. 
    // For a sphere centered at origin, the normal at a surface point (x,y,z)
    // is simply the unit vector (x,y,z) itself. We set that as the normal
    // with glNormal3d(...) before glVertex3d(...).
    //
    // If we scale the vertex positions by r or non-uniformly (for ellipsoid),
    // GL_NORMALIZE will make sure the normal is normalized correctly.

    for (i = 0; i < 2 * n; i++) {
        for (j = 0; j < n; j++) {
            glBegin(GL_POLYGON);
            double theta1 = i * M_PI / n;
            double theta2 = (i + 1) * M_PI / n;
            double phi1 = j * M_PI / n;
            double phi2 = (j + 1) * M_PI / n;

            // vertex 1
            double x1 = sin(theta1) * sin(phi1);
            double y1 = cos(theta1) * sin(phi1);
            double z1 = cos(phi1);
            glNormal3d(x1, y1, z1);            // normal = unit position vector
            glVertex3d(r * x1, r * y1, r * z1);

            // vertex 2
            double x2 = sin(theta2) * sin(phi1);
            double y2 = cos(theta2) * sin(phi1);
            double z2 = cos(phi1);
            glNormal3d(x2, y2, z2);
            glVertex3d(r * x2, r * y2, r * z2);

            // vertex 3
            double x3 = sin(theta2) * sin(phi2);
            double y3 = cos(theta2) * sin(phi2);
            double z3 = cos(phi2);
            glNormal3d(x3, y3, z3);
            glVertex3d(r * x3, r * y3, r * z3);

            // vertex 4
            double x4 = sin(theta1) * sin(phi2);
            double y4 = cos(theta1) * sin(phi2);
            double z4 = cos(phi2);
            glNormal3d(x4, y4, z4);
            glVertex3d(r * x4, r * y4, r * z4);

            glEnd();
        }
    }
}

// simple ellipsoid using sphere
void drawEllipsoid(double rx, double ry, double rz)
{
    glPushMatrix();
    // scale a unit sphere into an ellipsoid
    glScalef(rx, ry, rz);
    // drawSphere(1.0) expects radius 1 unit
    drawSphere(1.0);
    glPopMatrix();
}

// calculate a point on the Möbius strip
void mobiusPoint(double R, double u, double v, double& x, double& y, double& z) {
    x = (R + v * cos(u / 2.0)) * cos(u);
    y = (R + v * cos(u / 2.0)) * sin(u);
    z = v * sin(u / 2.0);
}

// calculate the normal of a point on the Möbius strip：∂P/∂u × ∂P/∂v
void mobiusNormal(double R, double u, double v, double& nx, double& ny, double& nz) {
    // ∂P/∂u
    double xu = -sin(u) * (R + v * cos(u / 2.0)) - 0.5 * v * sin(u / 2.0) * cos(u);
    double yu = cos(u) * (R + v * cos(u / 2.0)) - 0.5 * v * sin(u / 2.0) * sin(u);
    double zu = 0.5 * v * cos(u / 2.0);

    // ∂P/∂v
    double xv = cos(u / 2.0) * cos(u);
    double yv = cos(u / 2.0) * sin(u);
    double zv = sin(u / 2.0);

	// cross product ∂P/∂u × ∂P/∂v
    nx = yu * zv - zu * yv;
    ny = zu * xv - xu * zv;
    nz = xu * yv - yu * xv;

    // normalize
    double len = sqrt(nx * nx + ny * ny + nz * nz);
    nx /= len; ny /= len; nz /= len;
}

// draw Möbius strip using quad strips
void drawMobius(double R, double w, int slices, int stacks) {
    for (int i = 0; i < slices; i++) {
        double u1 = (double)i / slices * 2.0 * M_PI;
        double u2 = (double)(i + 1) / slices * 2.0 * M_PI;

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= stacks; j++) {
            double v = -w + 2.0 * w * j / stacks;

			// vertex1
            double x1, y1, z1, nx1, ny1, nz1;
            mobiusPoint(R, u1, v, x1, y1, z1);
            mobiusNormal(R, u1, v, nx1, ny1, nz1);
            glNormal3d(nx1, ny1, nz1);
            glVertex3d(x1, y1, z1);

			// vertex2
            double x2, y2, z2, nx2, ny2, nz2;
            mobiusPoint(R, u2, v, x2, y2, z2);
            mobiusNormal(R, u2, v, nx2, ny2, nz2);
            glNormal3d(nx2, ny2, nz2);
            glVertex3d(x2, y2, z2);
        }
        glEnd();
    }
}



// display
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
    updateCamera();


    glRotatef(angle2, 1.0, 0.0, 0.0);
    glRotatef(angle, 0.0, 1.0, 0.0);

    glScalef(zoom, zoom, zoom);

    // Set shading model according to m_Smooth 
    if (m_Smooth)
        glShadeModel(GL_SMOOTH);
    else
        glShadeModel(GL_FLAT);

    // Set material (ambient + diffuse always) 
    GLfloat mat_ambient[] = { 0.2f, 0.2f, 0.8f, 1.0f };
    GLfloat mat_diffuse[] = { 0.3f, 0.6f, 1.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);

    // Specular / shininess only when m_Highlight is true
    if (m_Highlight) {
        GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GLfloat mat_shininess[] = { 50.0f }; // from larger to tighter highlight
        glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
        glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    }
    else {
        GLfloat mat_specular[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        GLfloat mat_shininess[] = { 0.0f };
        glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
        glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    }

    switch (current_object) {
    case 0:
        drawSphere(1.0);
        break;
    case 1:
        // example curved object — which can be seen if press '2'
        // drawEllipsoid(1.5, 1.0, 0.8);
        drawMobius(1.5, 0.3, 100, 20);
        break;
    case 2:
        // placeholder: another curved object
#ifdef GLU_TODO
        glutSolidTeapot(1.0);
#endif
        break;
    case 3:
        // another curved placeholder
#ifdef GLU_TODO
        glutSolidTorus(0.2, 0.7, 20, 20);
#endif
        break;
    default:
        break;
    };
    glPopMatrix();
    glutSwapBuffers();
}

// keyboard & mouse
void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 'p':
    case 'P':
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    case 'w':
    case 'W':
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;
    case 'v':
    case 'V':
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        break;
    case 's':
    case 'S':
        m_Smooth = !m_Smooth;  // switch flat <-> smooth
        break;
    case 'h':
    case 'H':
        m_Highlight = !m_Highlight;  // switch highlights
        break;
    case '1':
    case '2':
    case '3':
    case '4':
        current_object = key - '1';
        break;
    case 'Q':
    case 'q':
        exit(0);
        break;
    case 'n': 
        camNear -= 0.1f; 
        if (camNear < 0.1f) camNear = 0.1f; 
        break;
    case 'N': 
        camNear += 0.1f; 
        break;
    case 'f': 
        camFar -= 1.0f; 
        if (camFar < 1.0f) camFar = 1.0f; 
        break;
    case 'F': 
        camFar += 1.0f; 
        break;
    case 'o': 
        camFovy -= 1.0f; 
        if (camFovy < 5.0f) camFovy = 5.0f; 
        break;
    case 'O': 
        camFovy += 1.0f; 
        if (camFovy > 170.0f) camFovy = 170.0f; 
        break;
    case 'r':   // reset to initial parameters
        camEyeX = 0; 
        camEyeY = 0; 
        camEyeZ = 6;
        camCenterX = 0; 
        camCenterY = 0; 
        camCenterZ = 0;
        camUpX = 0; 
        camUpY = 1; 
        camUpZ = 0;
        camNear = 1.0f; 
        camFar = 80.0f; 
        camFovy = 40.0f;
        break;
    case 'R':   // best camera position for object
        camEyeX = 4; 
        camEyeY = 4; 
        camEyeZ = 6; // example: diagonal top view
        camCenterX = 0; 
        camCenterY = 0; 
        camCenterZ = 0;
        camUpX = 0; 
        camUpY = 1; 
        camUpZ = 0;
        break;
    default:
        break;
    }

    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN) {
        mouseButton = button;
        moving = 1;
        startx = x;
        starty = y;
    }
    if (state == GLUT_UP) {
        mouseButton = button;
        moving = 0;
    }
}

void motion(int x, int y)
{
    if (moving) {
        if (mouseButton == GLUT_LEFT_BUTTON)
        {
            angle = angle + (x - startx);
            angle2 = angle2 + (y - starty);
        }
        else zoom += ((y - starty) * 0.001f);
        startx = x;
        starty = y;
        glutPostRedisplay();
    }
}

int main(int argc, char** argv)
{
    cout << "CS3241 Lab 3" << endl << endl;

    cout << "1-4: Draw different objects" << endl;
    cout << "S: Toggle Smooth Shading" << endl;
    cout << "H: Toggle Highlight" << endl;
    cout << "W: Draw Wireframe" << endl;
    cout << "P: Draw Polygon (filled)" << endl;
    cout << "V: Draw Vertices" << endl;
    cout << "Q: Quit" << endl << endl;

    cout << "Left mouse click and drag: rotate the object" << endl;
    cout << "Right mouse click and drag: zooming" << endl;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(600, 600);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("CS3241 Assignment 3");
    glClearColor(1.0, 1.0, 1.0, 1.0);

    // use GL_FILL as default
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);
    setupLighting();

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    glMatrixMode(GL_PROJECTION);
    gluPerspective( /* field of view in degree */ 40.0,
        /* aspect ratio */ 1.0,
        /* Z near */ 1.0, /* Z far */ 80.0);
    glMatrixMode(GL_MODELVIEW);
    glutMainLoop();

    return 0;
}
