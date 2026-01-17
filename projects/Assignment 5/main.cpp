// CS3241Lab5.cpp 
// Name: Chen Hongshan
// Student ID: A0311136W
// ************** IMPORTANT ******************
// Extra features: Shadows + Texture Mapping (Scene 1 only)

/*
README

Added features:

1) Shadow
- Shadow effect is only enabled in Scene 1.

2) Texture Mapping (Scene 1 only)
- Press 'S' to switch scenes.
- In Scene 1:
    - Press 'A' to toggle texture for sphere 0 (big blue)
    - Press 'B' to toggle texture for sphere 1 (green)
    - Press 'C' to toggle texture for sphere 2 (yellow)
    - Press 'D' to toggle texture for sphere 3 (grey)
- Put these PPM(P6) files in the same folder as the executable:
    hogwarts.ppm, slytherin.ppm, hufflepuff.ppm, gryffindor.ppm

Notes:
- PPM must be binary P6 with max value = 255.
*/

#include <cmath>
#include <iostream>
#include <chrono>
#include <fstream>
#include <string>
#include <cfloat>   // DBL_MAX
#include <cstdlib>  // exit
#include "GL/glut.h"
#include "vector3D.h"

using namespace std;

const double PI = 3.14159265358979323846;

#define WINWIDTH 600
#define WINHEIGHT 400
#define NUM_OBJECTS 4
#define MAX_RT_LEVEL 50
#define NUM_SCENE 2

float* pixelBuffer = new float[WINWIDTH * WINHEIGHT * 3];

// Per–sphere texture enable flags for scene 1
// 0: big blue sphere, 1: green, 2: yellow, 3: grey
bool texEnabled[NUM_OBJECTS] = { false, false, false, false };

// Textures (loaded from PPM)
int texW[NUM_OBJECTS] = { 0,0,0,0 };
int texH[NUM_OBJECTS] = { 0,0,0,0 };
unsigned char* texImg[NUM_OBJECTS] = { nullptr, nullptr, nullptr, nullptr };

class Ray { // a ray that start with "start" and going in the direction "dir"
public:
    Vector3 start, dir;
};

class RtObject {
public:
    virtual double intersectWithRay(Ray, Vector3& pos, Vector3& normal) = 0;
    // return a -ve if there is no intersection. Otherwise, return the smallest postive value of t

    // Materials Properties
    double ambiantReflection[3];
    double diffusetReflection[3];
    double specularReflection[3];
    double speN = 300;
};

class Sphere : public RtObject {
    Vector3 center_;
    double r_;
public:
    Sphere(Vector3 c, double r) { center_ = c; r_ = r; };
    Sphere() {};
    void set(Vector3 c, double r) { center_ = c; r_ = r; };

    Vector3 getCenter() const { return center_; }
    double getRadius() const { return r_; }

    double intersectWithRay(Ray, Vector3& pos, Vector3& normal);
};

RtObject** objList; // The list of all objects in the scene

// Global Variables
Vector3 cameraPos(0, 0, -500);

// assume the the following two vectors are normalised
Vector3 lookAtDir(0, 0, 1);
Vector3 upVector(0, 1, 0);
Vector3 leftVector(1, 0, 0);
float focalLen = 500;

// Light Settings
Vector3 lightPos(900, 1000, -1500);
double ambiantLight[3] = { 0.4,0.4,0.4 };
double diffusetLight[3] = { 0.7,0.7, 0.7 };
double specularLight[3] = { 0.5,0.5, 0.5 };

double bgColor[3] = { 0.1,0.1,0.4 };

int sceneNo = 0;

double Sphere::intersectWithRay(Ray r, Vector3& intersection, Vector3& normal)
{
    Vector3 O = r.start;
    Vector3 D = r.dir;
    Vector3 C = center_;

    Vector3 L = O - C;

    double a = dot_prod(D, D);
    double b = 2.0 * dot_prod(D, L);
    double c = dot_prod(L, L) - r_ * r_;

    double discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0.0)
        return -1.0;

    double sqrtDisc = sqrt(discriminant);

    double t1 = (-b - sqrtDisc) / (2.0 * a);
    double t2 = (-b + sqrtDisc) / (2.0 * a);

    const double EPS = 1e-4;
    double t = -1.0;

    if (t1 > EPS && t2 > EPS)
        t = (t1 < t2) ? t1 : t2;
    else if (t1 > EPS)
        t = t1;
    else if (t2 > EPS)
        t = t2;
    else
        return -1.0;

    intersection = O + (D * t);
    normal = intersection - C;
    normal.normalize();

    return t;
}

static void sampleSphereTexture(
    int objIndex,
    Vector3 hitPoint,   // <- 传值：不再是 const 左操作数
    double outColor[3]
) {
    if (objIndex < 0 || objIndex >= NUM_OBJECTS) return;
    if (!texImg[objIndex] || texW[objIndex] <= 0 || texH[objIndex] <= 0) return;

    Sphere* sph = (Sphere*)objList[objIndex];
    Vector3 center = sph->getCenter();

    Vector3 p = hitPoint - center;
    p.normalize();

    double x = p.x[0];
    double y = p.x[1];
    double z = p.x[2];

    double u = atan2(z, x) / (2.0 * PI) + 0.5;
    double v = 0.5 - asin(y) / PI;

    if (u < 0.0) u += 1.0;
    if (u > 1.0) u -= 1.0;

    if (v < 0.0) v = 0.0;
    if (v > 1.0) v = 1.0;

    int iu = (int)(u * (texW[objIndex] - 1));
    int iv = (int)(v * (texH[objIndex] - 1));
    int idx = (iv * texW[objIndex] + iu) * 3;

    outColor[0] = texImg[objIndex][idx] / 255.0;
    outColor[1] = texImg[objIndex][idx + 1] / 255.0;
    outColor[2] = texImg[objIndex][idx + 2] / 255.0;
}

void rayTrace(Ray ray, double& r, double& g, double& b, int fromObj, int level)
{
    bool goBackground = true;

    Vector3 intersection, normal;
    Vector3 lightV, viewV, lightReflectionV, rayReflectionV;

    Ray newRay;
    double mint = DBL_MAX, t;

    int hitObj = -1;
    Vector3 bestIntersection;
    Vector3 bestNormal;

    // 1) closest hit
    for (int i = 0; i < NUM_OBJECTS; i++)
    {
        if (i == fromObj) continue;

        if ((t = objList[i]->intersectWithRay(ray, intersection, normal)) > 0.0)
        {
            if (t < mint)
            {
                mint = t;
                hitObj = i;
                bestIntersection = intersection;
                bestNormal = normal;
            }
        }
    }

    if (hitObj != -1)
    {
        // 2) lighting vectors
        Vector3 L = lightPos - bestIntersection;
        double lightDist = sqrt(dot_prod(L, L));
        lightV = L / lightDist;

        viewV = cameraPos - bestIntersection;
        viewV.normalize();

        bestNormal.normalize();

        // 3) texture mapping (Scene 1 only)
        bool useTexture = false;
        double texColor[3] = { 1.0, 1.0, 1.0 };

        if (sceneNo == 1 && texEnabled[hitObj] && texImg[hitObj] != nullptr)
        {
            useTexture = true;
            sampleSphereTexture(hitObj, bestIntersection, texColor);
        }

        // 4) shadow (Scene 1 only)
        bool inShadow = false;
        if (sceneNo == 1)
        {
            Ray shadowRay;
            shadowRay.start = bestIntersection + lightV * 1e-3;
            shadowRay.dir = lightV;

            for (int j = 0; j < NUM_OBJECTS; j++)
            {
                if (j == hitObj) continue;

                Vector3 shInter, shNorm;
                double tShadow = objList[j]->intersectWithRay(shadowRay, shInter, shNorm);
                if (tShadow > 0.0 && tShadow < lightDist)
                {
                    inShadow = true;
                    break;
                }
            }
        }

        // 5) Phong illumination
        double NdotL = dot_prod(bestNormal, lightV);
        if (NdotL < 0.0) NdotL = 0.0;

        lightReflectionV = (bestNormal * (2.0 * dot_prod(bestNormal, lightV))) - lightV;
        lightReflectionV.normalize();

        double RdotV = dot_prod(lightReflectionV, viewV);
        double specFactor = (RdotV > 0.0) ? pow(RdotV, objList[hitObj]->speN) : 0.0;

        if (inShadow)
        {
            NdotL = 0.0;
            specFactor = 0.0;
        }

        double color[3];

        for (int c = 0; c < 3; c++)
        {
            double ambient = ambiantLight[c] * objList[hitObj]->ambiantReflection[c];

            double kd = objList[hitObj]->diffusetReflection[c];
            if (useTexture) kd = texColor[c];

            double diffuse = diffusetLight[c] * kd * NdotL;
            double specular = specularLight[c] * objList[hitObj]->specularReflection[c] * specFactor;

            color[c] = ambient + diffuse + specular;

            if (color[c] < 0.0) color[c] = 0.0;
            if (color[c] > 1.0) color[c] = 1.0;
        }

        r = color[0];
        g = color[1];
        b = color[2];

        // 6) Reflection (recursive)
        if (level < MAX_RT_LEVEL &&
            (objList[hitObj]->specularReflection[0] > 0.0 ||
                objList[hitObj]->specularReflection[1] > 0.0 ||
                objList[hitObj]->specularReflection[2] > 0.0))
        {
            double DdotN = dot_prod(ray.dir, bestNormal);
            rayReflectionV = ray.dir - bestNormal * (2.0 * DdotN);
            rayReflectionV.normalize();

            newRay.start = bestIntersection + rayReflectionV * 1e-3; // avoid acne
            newRay.dir = rayReflectionV;

            double rr, rg, rb;
            rayTrace(newRay, rr, rg, rb, hitObj, level + 1);

            double reflectivity = useTexture ? 0.15 : 0.3;

            r = (1.0 - reflectivity) * r + reflectivity * rr;
            g = (1.0 - reflectivity) * g + reflectivity * rg;
            b = (1.0 - reflectivity) * b + reflectivity * rb;
        }

        goBackground = false;
    }

    // 7) background
    if (goBackground)
    {
        r = bgColor[0];
        g = bgColor[1];
        b = bgColor[2];
    }
}

void drawInPixelBuffer(int x, int y, double r, double g, double b)
{
    pixelBuffer[(y * WINWIDTH + x) * 3] = (float)r;
    pixelBuffer[(y * WINWIDTH + x) * 3 + 1] = (float)g;
    pixelBuffer[(y * WINWIDTH + x) * 3 + 2] = (float)b;
}

void renderScene()
{
    int x, y;
    Ray ray;
    double r, g, b;

    cout << "Rendering Scene " << sceneNo << " with resolution "
        << WINWIDTH << "x" << WINHEIGHT << "........... ";

    long long time1 = chrono::duration_cast<chrono::milliseconds>(
        chrono::steady_clock::now().time_since_epoch()).count();

    ray.start = cameraPos;

    Vector3 vpCenter = cameraPos + lookAtDir * focalLen;
    Vector3 startingPt = vpCenter + leftVector * (-WINWIDTH / 2.0) + upVector * (-WINHEIGHT / 2.0);
    Vector3 currPt;

    for (x = 0; x < WINWIDTH; x++)
        for (y = 0; y < WINHEIGHT; y++)
        {
            currPt = startingPt + leftVector * x + upVector * y;
            ray.dir = currPt - cameraPos;
            ray.dir.normalize();
            rayTrace(ray, r, g, b, -1, 0);
            drawInPixelBuffer(x, y, r, g, b);
        }

    long long time2 = chrono::duration_cast<chrono::milliseconds>(
        chrono::steady_clock::now().time_since_epoch()).count();

    cout << "Done! \nRendering time = " << (time2 - time1) << "ms" << endl << endl;
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Make sure the full image is visible even if the window is resized
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glRasterPos2i(0, 0);
    glPixelZoom((float)w / (float)WINWIDTH, (float)h / (float)WINHEIGHT);

    glDrawPixels(WINWIDTH, WINHEIGHT, GL_RGB, GL_FLOAT, pixelBuffer);

    glPixelZoom(1.0f, 1.0f);

    glutSwapBuffers();
    glFlush();
}

void reshape(int w, int h)
{
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, 0, h, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void setScene(int i = 0)
{
    if (i < 0 || i >= NUM_SCENE)
    {
        cout << "Warning: Invalid Scene Number" << endl;
        return;
    }

    if (i == 0)
    {
        ((Sphere*)objList[0])->set(Vector3(-130, 80, 120), 100);
        ((Sphere*)objList[1])->set(Vector3(130, -80, -80), 100);
        ((Sphere*)objList[2])->set(Vector3(-130, -80, -80), 100);
        ((Sphere*)objList[3])->set(Vector3(130, 80, 120), 100);

        objList[0]->ambiantReflection[0] = 0.1;
        objList[0]->ambiantReflection[1] = 0.4;
        objList[0]->ambiantReflection[2] = 0.4;

        objList[0]->diffusetReflection[0] = 0;
        objList[0]->diffusetReflection[1] = 1;
        objList[0]->diffusetReflection[2] = 1;

        objList[0]->specularReflection[0] = 0.2;
        objList[0]->specularReflection[1] = 0.4;
        objList[0]->specularReflection[2] = 0.4;

        objList[0]->speN = 300;

        objList[1]->ambiantReflection[0] = 0.6;
        objList[1]->ambiantReflection[1] = 0.6;
        objList[1]->ambiantReflection[2] = 0.2;

        objList[1]->diffusetReflection[0] = 1;
        objList[1]->diffusetReflection[1] = 1;
        objList[1]->diffusetReflection[2] = 0;

        objList[1]->specularReflection[0] = 0.0;
        objList[1]->specularReflection[1] = 0.0;
        objList[1]->specularReflection[2] = 0.0;

        objList[1]->speN = 50;

        objList[2]->ambiantReflection[0] = 0.1;
        objList[2]->ambiantReflection[1] = 0.6;
        objList[2]->ambiantReflection[2] = 0.1;

        objList[2]->diffusetReflection[0] = 0.1;
        objList[2]->diffusetReflection[1] = 1;
        objList[2]->diffusetReflection[2] = 0.1;

        objList[2]->specularReflection[0] = 0.3;
        objList[2]->specularReflection[1] = 0.7;
        objList[2]->specularReflection[2] = 0.3;

        objList[2]->speN = 650;

        objList[3]->ambiantReflection[0] = 0.3;
        objList[3]->ambiantReflection[1] = 0.3;
        objList[3]->ambiantReflection[2] = 0.3;

        objList[3]->diffusetReflection[0] = 0.7;
        objList[3]->diffusetReflection[1] = 0.7;
        objList[3]->diffusetReflection[2] = 0.7;

        objList[3]->specularReflection[0] = 0.6;
        objList[3]->specularReflection[1] = 0.6;
        objList[3]->specularReflection[2] = 0.6;

        objList[3]->speN = 650;
    }

    if (i == 1)
    {
        ((Sphere*)objList[0])->set(Vector3(-80, -20, 260), 160);
        ((Sphere*)objList[1])->set(Vector3(160, -90, 100), 90);
        ((Sphere*)objList[2])->set(Vector3(-40, 20, 40), 60);
        ((Sphere*)objList[3])->set(Vector3(140, 80, 230), 80);

        objList[0]->ambiantReflection[0] = 0.25;
        objList[0]->ambiantReflection[1] = 0.45;
        objList[0]->ambiantReflection[2] = 0.95;

        objList[0]->diffusetReflection[0] = 0.4;
        objList[0]->diffusetReflection[1] = 0.7;
        objList[0]->diffusetReflection[2] = 1.3;

        objList[0]->specularReflection[0] = 1.0;
        objList[0]->specularReflection[1] = 1.0;
        objList[0]->specularReflection[2] = 1.0;

        objList[0]->speN = 140;

        objList[1]->ambiantReflection[0] = 0.1;
        objList[1]->ambiantReflection[1] = 0.6;
        objList[1]->ambiantReflection[2] = 0.1;

        objList[1]->diffusetReflection[0] = 0.1;
        objList[1]->diffusetReflection[1] = 1.0;
        objList[1]->diffusetReflection[2] = 0.1;

        objList[1]->specularReflection[0] = 0.3;
        objList[1]->specularReflection[1] = 0.7;
        objList[1]->specularReflection[2] = 0.3;

        objList[1]->speN = 650;

        objList[2]->ambiantReflection[0] = 0.70;
        objList[2]->ambiantReflection[1] = 0.70;
        objList[2]->ambiantReflection[2] = 0.10;

        objList[2]->diffusetReflection[0] = 1.2;
        objList[2]->diffusetReflection[1] = 1.2;
        objList[2]->diffusetReflection[2] = 0.0;

        objList[2]->specularReflection[0] = 0.4;
        objList[2]->specularReflection[1] = 0.4;
        objList[2]->specularReflection[2] = 0.2;

        objList[2]->speN = 90;

        objList[3]->ambiantReflection[0] = 0.50;
        objList[3]->ambiantReflection[1] = 0.50;
        objList[3]->ambiantReflection[2] = 0.40;

        objList[3]->diffusetReflection[0] = 1.0;
        objList[3]->diffusetReflection[1] = 1.0;
        objList[3]->diffusetReflection[2] = 0.8;

        objList[3]->specularReflection[0] = 0.4;
        objList[3]->specularReflection[1] = 0.4;
        objList[3]->specularReflection[2] = 0.4;

        objList[3]->speN = 60;
    }
}

// load ppm file
bool loadPPMTexture(const char* filename, int& width, int& height, unsigned char*& data)
{
    ofstream log("texture_log.txt", ios::app);
    log << "Trying to load texture: " << filename << endl;

    ifstream in(filename, ios::binary);
    if (!in.is_open())
    {
        log << "Cannot open texture file: " << filename << endl;
        return false;
    }

    string magic;
    in >> magic;
    log << "Magic: " << magic << endl;
    if (magic != "P6")
    {
        log << "Not a binary PPM (P6) file." << endl;
        return false;
    }

    char ch;
    in.get(ch);

    while (in.peek() == '#')
    {
        string comment;
        getline(in, comment);
        log << "Comment: " << comment << endl;
    }

    int w, h, maxv;
    in >> w >> h >> maxv;
    in.get(ch); // eat newline

    log << "Size read from header: " << w << " x " << h
        << ", maxv = " << maxv << endl;

    if (maxv != 255)
    {
        log << "Unsupported max value in PPM: " << maxv << endl;
        return false;
    }

    width = w;
    height = h;
    data = new unsigned char[width * height * 3];

    in.read((char*)data, width * height * 3);

    if (!in)
    {
        log << "Error reading PPM pixel data." << endl;
        delete[] data;
        data = nullptr;
        return false;
    }

    log << "Texture loaded successfully: " << filename
        << " (" << width << " x " << height << ")" << endl;
    return true;
}

static void cleanupAndExit()
{
    for (int i = 0; i < NUM_OBJECTS; i++)
    {
        if (texImg[i]) { delete[] texImg[i]; texImg[i] = nullptr; }
    }

    if (objList)
    {
        for (int i = 0; i < NUM_OBJECTS; i++) delete objList[i];
        delete[] objList;
        objList = nullptr;
    }

    if (pixelBuffer)
    {
        delete[] pixelBuffer;
        pixelBuffer = nullptr;
    }

    exit(0);
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 's':
    case 'S':
        sceneNo = (sceneNo + 1) % NUM_SCENE;
        setScene(sceneNo);
        renderScene();
        glutPostRedisplay();
        break;

    case 'a':
    case 'A':
        texEnabled[0] = !texEnabled[0];
        cout << "Texture for sphere 0 (big blue) "
            << (texEnabled[0] ? "ON" : "OFF") << endl;
        renderScene();
        glutPostRedisplay();
        break;

    case 'b':
    case 'B':
        texEnabled[1] = !texEnabled[1];
        cout << "Texture for sphere 1 (green) "
            << (texEnabled[1] ? "ON" : "OFF") << endl;
        renderScene();
        glutPostRedisplay();
        break;

    case 'c':
    case 'C':
        texEnabled[2] = !texEnabled[2];
        cout << "Texture for sphere 2 (yellow) "
            << (texEnabled[2] ? "ON" : "OFF") << endl;
        renderScene();
        glutPostRedisplay();
        break;

    case 'd':
    case 'D':
        texEnabled[3] = !texEnabled[3];
        cout << "Texture for sphere 3 (grey) "
            << (texEnabled[3] ? "ON" : "OFF") << endl;
        renderScene();
        glutPostRedisplay();
        break;

    case 'q':
    case 'Q':
        cleanupAndExit();
        break;

    default:
        break;
    }
}

int main(int argc, char** argv)
{
    cout << "<<CS3241 Lab 5>>\n\n" << endl;
    cout << "S : go to next scene" << endl;
    cout << "Q : quit" << endl;
    cout << "A/B/C/D : toggle textures (Scene 1 only)" << endl;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINWIDTH, WINHEIGHT);

    glutCreateWindow("CS3241 Lab 5: Ray Tracing");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    objList = new RtObject * [NUM_OBJECTS];

    objList[0] = new Sphere(Vector3(-130, 80, 120), 100);
    objList[1] = new Sphere(Vector3(130, -80, -80), 100);
    objList[2] = new Sphere(Vector3(-130, -80, -80), 100);
    objList[3] = new Sphere(Vector3(130, 80, 120), 100);

    // Map textures to objects:
    // 0: hogwarts, 1: slytherin, 2: hufflepuff, 3: gryffindor
    if (!loadPPMTexture("hogwarts.ppm", texW[0], texH[0], texImg[0]))
        cout << "Texture load failed: hogwarts.ppm (sphere 0 uses solid color)\n";

    if (!loadPPMTexture("slytherin.ppm", texW[1], texH[1], texImg[1]))
        cout << "Texture load failed: slytherin.ppm (sphere 1 uses solid color)\n";

    if (!loadPPMTexture("hufflepuff.ppm", texW[2], texH[2], texImg[2]))
        cout << "Texture load failed: hufflepuff.ppm (sphere 2 uses solid color)\n";

    if (!loadPPMTexture("gryffindor.ppm", texW[3], texH[3], texImg[3]))
        cout << "Texture load failed: gryffindor.ppm (sphere 3 uses solid color)\n";

    setScene(0);
    setScene(sceneNo);
    renderScene();

    glutMainLoop();

    // never reached normally
    cleanupAndExit();
    return 0;
}
