
// CS3241 Assignment 1: Doodle
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
float alpha = 0.0, k = 1;
float tx = 0.0, ty = 0.0;


//void display(void)
//{
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//	
//	glPushMatrix();
//
//	//controls transformation
//	glScalef(k, k, k);	
//	glTranslatef(tx, ty, 0);	
//	glRotatef(alpha, 0, 0, 1);
//	
//	//draw your stuff here (Erase the triangle code)
//	glBegin(GL_POLYGON);
//		glColor3f(0.5, 0, 0);
//		glVertex2f(-3,-3);
//		glVertex2f(3,-3);
//		glVertex2f(0,3);
//	glEnd();
//
//	glPopMatrix();
//	glFlush ();
//}

// 8x8 "character" bitmap
int pixelArt[8][8] = {
	{0,0,1,1,1,1,0,0},
	{0,1,1,1,1,1,1,0},
	{1,1,0,1,1,0,1,1},
	{1,1,1,1,1,1,1,1},
	{1,2,2,1,1,2,2,1},
	{1,2,2,1,1,2,2,1},
	{0,1,1,2,2,1,1,0},
	{0,0,1,1,1,1,0,0}
};

void drawPixelArt() {
	float size = 1.0f;   // 每个像素的大小（正方形边长）
	for (int row = 0; row < 8; row++) {
		for (int col = 0; col < 8; col++) {
			int val = pixelArt[row][col];
			if (val == 0) continue; // 0 表示背景，不画

			// 设置颜色
			if (val == 1) glColor3f(1.0, 0.8, 0.6);   // 肤色
			if (val == 2) glColor3f(0.2, 0.2, 1.0);   // 蓝色衣服

			float x = col * size;
			float y = (7 - row) * size; // 翻转一下 Y，让第一行在上方

			// 画一个小方块（quad）
			glBegin(GL_QUADS);
			glVertex2f(x, y);
			glVertex2f(x + size, y);
			glVertex2f(x + size, y + size);
			glVertex2f(x, y + size);
			glEnd();
		}
	}
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();

	// controls transformation
	glScalef(k, k, k);
	glTranslatef(tx, ty, 0);
	glRotatef(alpha, 0, 0, 1);
	drawPixelArt();

	glPopMatrix();
	glFlush();
}

void reshape(int w, int h)
{
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-10, 10, -10, 10, -10, 10);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void init(void)
{
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glShadeModel(GL_SMOOTH);
}



void keyboard(unsigned char key, int x, int y)
{
	//keys to control scaling - k
	//keys to control rotation - alpha
	//keys to control translation - tx, ty
	switch (key) {

		// anticlockwise
	case 'z':
		alpha += 10;
		glutPostRedisplay();
		break;

		// clockwise
	case 'c':
		alpha -= 10;
		glutPostRedisplay();
		break;

		// zoom in
	case 'q':
		k += 0.1;
		glutPostRedisplay();
		break;

		// zoom out
	case 'e':
		if (k > 0.1)
			k -= 0.1;
		glutPostRedisplay();
		break;

		// left
	case 'a':
		tx -= 0.1;
		glutPostRedisplay();
		break;

		// right
	case 'd':
		tx += 0.1;
		glutPostRedisplay();
		break;

		// down
	case 's':
		ty -= 0.1;
		glutPostRedisplay();
		break;

		// up
	case 'w':
		ty += 0.1;
		glutPostRedisplay();
		break;

	default:
		break;
	}
}

int main(int argc, char** argv)
{
	cout << "CS3241 Lab 1\n\n";
	cout << "+++++CONTROL BUTTONS+++++++\n\n";
	cout << "Scale Up/Down: Q/E\n";
	cout << "Rotate Clockwise/Counter-clockwise: A/D\n";
	cout << "Move Up/Down: W/S\n";
	cout << "Move Left/Right: Z/C\n";

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(600, 600);
	glutInitWindowPosition(50, 50);
	glutCreateWindow(argv[0]);
	init();
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	//glutMouseFunc(mouse);
	glutKeyboardFunc(keyboard);
	glutMainLoop();

	return 0;
}
