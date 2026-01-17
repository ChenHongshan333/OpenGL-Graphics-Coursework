// CS3241Lab4: Bezier curves with multi-segment I/O, tangents (T), objects (O), undo (Z)
// Name: Chen Hongshan
// Student ID: A0311136W
// - Multi-segment input uses "-1 -1" as a "pen-up" separator in savefile.txt
// - Undo stack snapshots before mutating operations (mouse add, read, clear)
// - Rendering still honors your previous toggles and cute cat object
//
// Build targets GLUT & OpenGL (Windows/macOS stubs included)

#include "math.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstring>  // memcpy
#include <sstream>   // istringstream
#include <limits>    // numeric_limits

#ifdef _WIN32
#include <Windows.h>
#include "GL/glut.h"
#ifndef M_PI
#define M_PI 3.141592654
#endif
#elif __APPLE__
#include <OpenGL/gl.h>
#include <GLUT/GLUT.h>
#endif

#define MAXPTNO 120000
#define NLINESEGMENT 32
#define NOBJECTONCURVE 8

using namespace std;

// ------------------------------
// Basic data
// ------------------------------
struct Point { int x, y; };
struct FPoint { float x, y; };

// Global point store (flat)
int nPt = 0;
Point ptList[MAXPTNO];

// Display toggles
bool displayControlPoints = true;   // 'P'
bool displayControlLines = true;   // 'L'
bool displayTangentVectors = false;  // 'T'
bool displayObjects = false;  // 'O'
bool C1Continuity = false;  // 'C'

// ------------------------------
// Segments (multi-path support)
// A "segment" is a consecutive slice of ptList which forms a single Bezier chain
// points are grouped [0..3], [3..6], [6..9], ... inside a segment
// ------------------------------
struct Segment { int start; int count; };
static std::vector<Segment> segments;

// Helper: view of "active segments" for drawing.
// If file read provided segments => use them.
// Else, if user just clicked points (no segments) => treat as single segment [0..nPt).
static inline const std::vector<Segment> buildActiveSegments()
{
    static std::vector<Segment> single;
    if (!segments.empty()) return segments;
    single.clear();
    if (nPt > 0) single.push_back({ 0, nPt });
    return single;
}

// ------------------------------
// Undo stack (snapshots of ptList + nPt + segments)
// ------------------------------
struct Snapshot {
    int n;
    Point pts[MAXPTNO];
    std::vector<Segment> segs;
};
static const int MAX_UNDO = 100;
static Snapshot undoStack[MAX_UNDO];
static int undoTop = 0;    // next write position (circular)
static int undoCount = 0;  // number stored

static inline void pushUndo()
{
    undoStack[undoTop].n = nPt;
    if (nPt > 0) {
        memcpy(undoStack[undoTop].pts, ptList, sizeof(Point) * nPt);
    }
    undoStack[undoTop].segs = segments;
    undoTop = (undoTop + 1) % MAX_UNDO;
    if (undoCount < MAX_UNDO) undoCount++;
}

static inline bool popUndo()
{
    if (undoCount == 0) return false;
    undoTop = (undoTop - 1 + MAX_UNDO) % MAX_UNDO;
    const Snapshot& s = undoStack[undoTop];
    nPt = s.n;
    if (nPt > 0) memcpy(ptList, s.pts, sizeof(Point) * nPt);
    segments = s.segs;
    undoCount--;
    return true;
}

// ------------------------------
// Curve helpers
// ------------------------------

// Evaluate cubic Bezier at t in [0,1]
static inline FPoint evalBezier(const Point& P0, const Point& P1, const Point& P2, const Point& P3, float t) {
    float u = 1.0f - t;
    float b0 = u * u * u;
    float b1 = 3 * u * u * t;
    float b2 = 3 * u * t * t;
    float b3 = t * t * t;
    FPoint r;
    r.x = b0 * P0.x + b1 * P1.x + b2 * P2.x + b3 * P3.x;
    r.y = b0 * P0.y + b1 * P1.y + b2 * P2.y + b3 * P3.y;
    return r;
}

// Evaluate derivative B'(t) (for tangents)
static inline FPoint evalBezierDeriv(const Point& P0, const Point& P1, const Point& P2, const Point& P3, float t) {
    // B'(t) = 3[(1-t)^2 (P1-P0) + 2(1-t)t (P2-P1) + t^2 (P3-P2)]
    float u = 1.0f - t;
    float d0 = 3.0f * u * u;
    float d1 = 6.0f * u * t;
    float d2 = 3.0f * t * t;
    FPoint r;
    r.x = d0 * (P1.x - P0.x) + d1 * (P2.x - P1.x) + d2 * (P3.x - P2.x);
    r.y = d0 * (P1.y - P0.y) + d1 * (P2.y - P1.y) + d2 * (P3.y - P2.y);
    return r;
}

// Normalize 2D vector; also return length
static inline FPoint normalize2D(const FPoint& v, float& lenOut) {
    float len = sqrtf(v.x * v.x + v.y * v.y);
    lenOut = len;
    FPoint u = v;
    if (len > 1e-6f) { u.x /= len; u.y /= len; }
    else { u.x = 1.0f; u.y = 0.0f; } // default axis if degenerate
    return u;
}

// Fetch control points of curve c inside a segment that starts at segStart
// Optionally use virtual C1-adjusted P1 (does NOT mutate ptList).
static inline void getCurveCPInSegment(
    int segStart, int curveIdx,
    Point& P0, Point& P1, Point& P2, Point& P3,
    bool useC1Adjusted,
    Point& origP1Out,
    bool& hasAdjusted)
{
    int i = segStart + curveIdx * 3;
    P0 = ptList[i];
    P1 = ptList[i + 1];
    P2 = ptList[i + 2];
    P3 = ptList[i + 3];

    origP1Out = P1;
    hasAdjusted = false;

    // C1 between previous and current within the SAME segment
    if (useC1Adjusted && curveIdx > 0) {
        Point Q2 = ptList[i - 1];
        Point newP1{ 2 * P0.x - Q2.x, 2 * P0.y - Q2.y };
        P1 = newP1;
        hasAdjusted = true;
    }
}

// ------------------------------
// Arrow (for tangent vectors)
// ------------------------------
static void drawRightArrow()
{
    glColor3f(0, 1, 0);
    glBegin(GL_LINE_STRIP);
    glVertex2f(0, 0);
    glVertex2f(100, 0);
    glVertex2f(95, 5);
    glVertex2f(100, 0);
    glVertex2f(95, -5);
    glEnd();
}

// Place arrow at world position p, aligned to "dir"
static void drawArrowAt(const FPoint& p, const FPoint& dir, float scale)
{
    float len;
    FPoint t = normalize2D(dir, len);
    float angleDeg = atan2f(t.y, t.x) * 180.0f / (float)M_PI;

    glPushMatrix();
    glTranslatef(p.x, p.y, 0.0f);
    glRotatef(angleDeg, 0.0f, 0.0f, 1.0f);
    glScalef(scale, scale, 1.0f);
    drawRightArrow();
    glPopMatrix();
}

// ------------------------------
// "Cute Cat" (from my Assign 1, ~1 unit size)
// ------------------------------
static void cc_drawCircle(float r, int num_segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.0f, 0.0f);
    for (int i = 0; i <= num_segments; i++) {
        float theta = 2.0f * (float)M_PI * i / num_segments;
        float x = r * cosf(theta);
        float y = r * sinf(theta);
        glVertex2f(x, y);
    }
    glEnd();
}

static void cc_drawTriangle() {
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.2f, 0.0f);
    glVertex2f(0.2f, 0.0f);
    glVertex2f(0.0f, 0.35f);
    glEnd();
}

static void cc_drawArc(float r, float theta_start, float theta_end, int num_segments) {
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= num_segments; i++) {
        float theta = theta_start + (theta_end - theta_start) * i / num_segments;
        float x = r * cosf(theta);
        float y = -r * sinf(theta) - r; // preserved offset from your previous code
        glVertex2f(x, y);
    }
    glEnd();
}

static void drawCuteCatUnit()
{
    // face
    glPushMatrix();
    glColor3f(1.0f, 1.0f, 1.0f);
    glScalef(1.1f, 1.0f, 1.0f);
    cc_drawCircle(0.5f, 100);
    glPopMatrix();

    // blue patches
    glPushMatrix();
    glColor3f(0.2f, 0.6f, 0.9f);
    glTranslatef(0.25f, 0.30f, 0.0f);
    glRotatef(-37.0f, 0, 0, 1);
    glScalef(1.1f, 0.49f, 1.0f);
    cc_drawCircle(0.35f, 100);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.2f, 0.6f, 0.9f);
    glTranslatef(-0.25f, 0.30f, 0.0f);
    glRotatef(37.0f, 0, 0, 1);
    glScalef(1.1f, 0.49f, 1.0f);
    cc_drawCircle(0.35f, 100);
    glPopMatrix();

    // ears
    glPushMatrix();
    glColor3f(0.2f, 0.6f, 0.9f);
    glTranslatef(-0.25f, 0.4f, 0.0f);
    glRotatef(20.0f, 0, 0, 1);
    cc_drawTriangle();
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.2f, 0.6f, 0.9f);
    glTranslatef(0.25f, 0.4f, 0.0f);
    glRotatef(-20.0f, 0, 0, 1);
    cc_drawTriangle();
    glPopMatrix();

    // blush
    glPushMatrix();
    glColor3f(1.0f, 0.7f, 0.7f);
    glTranslatef(-0.3f, -0.05f, 0.0f);
    glScalef(1.3f, 0.7f, 1.0f);
    cc_drawCircle(0.1f, 50);
    glPopMatrix();

    glPushMatrix();
    glColor3f(1.0f, 0.7f, 0.7f);
    glTranslatef(0.3f, -0.05f, 0.0f);
    glScalef(1.3f, 0.7f, 1.0f);
    cc_drawCircle(0.1f, 50);
    glPopMatrix();

    // eyes
    glPushMatrix();
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(6.0f);
    glTranslatef(-0.23f, 0.08f, 0.0f);
    glRotatef(13.0f, 0, 0, 1);
    cc_drawArc(0.12f, 1.25f * (float)M_PI, 1.8f * (float)M_PI, 1000);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(6.0f);
    glTranslatef(0.23f, 0.08f, 0.0f);
    glRotatef(-13.0f, 0, 0, 1);
    cc_drawArc(0.12f, 1.2f * (float)M_PI, 1.75f * (float)M_PI, 1000);
    glPopMatrix();

    // mouth
    glPushMatrix();
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(4.0f);
    glTranslatef(-0.036f, 0.0f, 0.0f);
    glRotatef(12.0f, 0, 0, 1);
    cc_drawArc(0.03f, 0.0f, (float)M_PI, 100);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(4.0f);
    glTranslatef(0.036f, 0.0f, 0.0f);
    glRotatef(-12.0f, 0, 0, 1);
    cc_drawArc(0.03f, 0.0f, (float)M_PI, 100);
    glPopMatrix();

    // eyebrows
    glPushMatrix();
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(4.0f);
    glTranslatef(-0.17f, 0.22f, 0.0f);
    glRotatef(12.0f, 0, 0, 1);
    cc_drawArc(0.08f, 1.4f * (float)M_PI, 1.6f * (float)M_PI, 100);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(4.0f);
    glTranslatef(0.17f, 0.22f, 0.0f);
    glRotatef(-12.0f, 0, 0, 1);
    cc_drawArc(0.08f, 1.4f * (float)M_PI, 1.6f * (float)M_PI, 100);
    glPopMatrix();

    // jaw line
    glPushMatrix();
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(3.0f);
    glTranslatef(0.0f, 0.02f, 0.0f);
    cc_drawArc(0.07f, 0.4f * (float)M_PI, 0.6f * (float)M_PI, 100);
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
}

// Place cat at world position p, aligned to tangent direction
static void drawObjectAt(const FPoint& p, const FPoint& dir, float scalePixels)
{
    float len;
    FPoint t = normalize2D(dir, len);
    float angleDeg = atan2f(t.y, t.x) * 180.0f / (float)M_PI;

    float s = scalePixels; // world units are pixels (see gluOrtho2D), unit cat ~1.0

    glPushMatrix();
    glTranslatef(p.x, p.y, 0.0f);
    glRotatef(angleDeg, 0.0f, 0.0f, 1.0f);
    glScalef(s, s, 1.0f);
    drawCuteCatUnit();
    glPopMatrix();
}

// ------------------------------
// Rendering
// ------------------------------
static inline int curvesInSegment(int segPts) {
    if (segPts < 4) return 0;
    return 1 + (segPts - 4) / 3;
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();

    const auto activeSegs = buildActiveSegments();

    // 1) Control points (black)
    if (displayControlPoints) {
        glPointSize(5);
        glBegin(GL_POINTS);
        glColor3f(0, 0, 0);
        for (const auto& seg : activeSegs) {
            for (int i = 0; i < seg.count; ++i) {
                const Point& p = ptList[seg.start + i];
                glVertex2d(p.x, p.y);
            }
        }
        glEnd();
        glPointSize(1);
    }

    // 2) Control polygons (green) with optional C1-adjusted visual P1
    if (displayControlLines) {
        glColor3f(0, 1, 0);
        for (const auto& seg : activeSegs) {
            int segPts = seg.count;
            int nCurves = curvesInSegment(segPts);

            for (int c = 0; c < nCurves; ++c) {
                Point P0, P1, P2, P3, origP1; bool adjusted = false;
                getCurveCPInSegment(seg.start, c, P0, P1, P2, P3, C1Continuity, origP1, adjusted);

                glBegin(GL_LINE_STRIP);
                glVertex2d(P0.x, P0.y);
                glVertex2d(P1.x, P1.y);
                glVertex2d(P2.x, P2.y);
                glVertex2d(P3.x, P3.y);
                glEnd();

                if (adjusted) {
                    glPointSize(6);
                    glBegin(GL_POINTS);
                    glColor3f(1, 0, 0);           // adjusted P1
                    glVertex2d(P1.x, P1.y);
                    glColor3f(0.7f, 0.7f, 0.7f); // original P1
                    glVertex2d(origP1.x, origP1.y);
                    glEnd();
                    glPointSize(1);
                    glColor3f(0, 1, 0);
                }
            }

            // If segment has >=2 points but <4, draw simple polyline as helper
            if (nCurves == 0 && segPts >= 2) {
                glBegin(GL_LINE_STRIP);
                for (int i = 0; i < segPts; ++i) {
                    const Point& p = ptList[seg.start + i];
                    glVertex2d(p.x, p.y);
                }
                glEnd();
            }
        }
    }

    // 3) Bezier curves (blue)
    {
        glColor3f(0, 0, 1);
        // glLineWidth(2.0f); // Optional thickness
        for (const auto& seg : activeSegs) {
            int segPts = seg.count;
            int nCurves = curvesInSegment(segPts);
            for (int c = 0; c < nCurves; ++c) {
                Point P0, P1, P2, P3, origP1; bool adjusted = false;
                getCurveCPInSegment(seg.start, c, P0, P1, P2, P3, C1Continuity, origP1, adjusted);

                glBegin(GL_LINE_STRIP);
                for (int k = 0; k <= NLINESEGMENT; ++k) {
                    float t = (float)k / (float)NLINESEGMENT;
                    FPoint q = evalBezier(P0, P1, P2, P3, t);
                    glVertex2f(q.x, q.y);
                }
                glEnd();
            }
        }
        // glLineWidth(1.0f);
    }

    // 4) Tangent vectors (green arrows)
    if (displayTangentVectors) {
        for (const auto& seg : activeSegs) {
            int segPts = seg.count;
            int nCurves = curvesInSegment(segPts);
            for (int c = 0; c < nCurves; ++c) {
                Point P0, P1, P2, P3, origP1; bool adjusted = false;
                getCurveCPInSegment(seg.start, c, P0, P1, P2, P3, C1Continuity, origP1, adjusted);

                for (int m = 0; m < NOBJECTONCURVE; ++m) {
                    float t = (NOBJECTONCURVE == 1) ? 0.5f : (float)m / (float)(NOBJECTONCURVE - 1);
                    FPoint pos = evalBezier(P0, P1, P2, P3, t);
                    FPoint der = evalBezierDeriv(P0, P1, P2, P3, t);
                    drawArrowAt(pos, der, 0.4f);
                }
            }
        }
    }

    // 5) Objects along curve (cute cat instances)
    if (displayObjects) {
        for (const auto& seg : activeSegs) {
            int segPts = seg.count;
            int nCurves = curvesInSegment(segPts);
            for (int c = 0; c < nCurves; ++c) {
                Point P0, P1, P2, P3, origP1; bool adjusted = false;
                getCurveCPInSegment(seg.start, c, P0, P1, P2, P3, C1Continuity, origP1, adjusted);

                for (int m = 0; m < NOBJECTONCURVE; ++m) {
                    float t = (NOBJECTONCURVE == 1) ? 0.5f : (float)m / (float)(NOBJECTONCURVE - 1);
                    FPoint pos = evalBezier(P0, P1, P2, P3, t);
                    FPoint der = evalBezierDeriv(P0, P1, P2, P3, t);
                    drawObjectAt(pos, der, 60.0f); // ~60 px cat
                }
            }
        }
    }

    glPopMatrix();
    glutSwapBuffers();
}

void reshape(int w, int h)
{
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // world units == pixels
    gluOrtho2D(0, w, h, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void init(void)
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

// ------------------------------
// File I/O
// Multi-segment format (new):
//   Each line: "x y"
//   A line "-1 -1" means "end of current segment / pen up"
//   No header count is required.
// Backward-compatible old format (first token is an integer N): read N points into one segment.
// ------------------------------

static inline bool allDigits(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) if (!std::isdigit((unsigned char)c)) return false;
    return true;
}

void readFile()
{
    // reading mutates state -> snapshot for Undo
    pushUndo();

    segments.clear();
    nPt = 0;

    std::ifstream file("savefile.txt");
    if (!file.is_open()) {
        std::cout << "Error: Cannot open savefile.txt for reading.\n";
        return;
    }

    auto flush_segment = [&]() {
        if (nPt == 0) return;
        if (segments.empty()) {
            segments.push_back({ 0, nPt });
        }
        else {
            int prevEnd = segments.back().start + segments.back().count;
            if (prevEnd < nPt) segments.push_back({ prevEnd, nPt - prevEnd });
        }
        };

    std::string line;
    while (std::getline(file, line)) {
        // 1) skip blank lines
        // find first non-space
        size_t i = line.find_first_not_of(" \t\r\n");
        if (i == std::string::npos) continue;

        // 2) skip full-line comments starting with '#'
        if (line[i] == '#') continue;

        // 3) parse two integers from the line (ignore trailing garbage)
        std::istringstream iss(line);
        int x, y;
        if (!(iss >> x >> y)) {
            // not a valid "x y" line -> try next
            continue;
        }

        // 4) separator "-1 -1" => close current segment
        if (x == -1 && y == -1) {
            flush_segment();
            continue;
        }

        // 5) normal coordinate
        if (nPt >= MAXPTNO) {
            std::cout << "Error: Exceeded MAXPTNO.\n";
            break;
        }
        ptList[nPt++] = { x, y };
    }
    file.close();

    // finalize last segment if file didn't end with -1 -1
    if (nPt > 0) {
        if (segments.empty()) segments.push_back({ 0, nPt });
        else {
            int prevEnd = segments.back().start + segments.back().count;
            if (prevEnd < nPt) segments.push_back({ prevEnd, nPt - prevEnd });
        }
    }

    glutPostRedisplay();
}



void writeFile()
{
    std::ofstream file("savefile.txt");
    if (!file.is_open()) {
        cout << "Error: Cannot open savefile.txt for writing." << endl;
        return;
    }

    // Prefer new multi-segment format
    const auto activeSegs = buildActiveSegments();
    for (size_t s = 0; s < activeSegs.size(); ++s) {
        int st = activeSegs[s].start, cnt = activeSegs[s].count;
        for (int i = 0; i < cnt; ++i) {
            const Point& p = ptList[st + i];
            file << p.x << " " << p.y << "\n";
        }
        if (s + 1 < activeSegs.size()) file << "-1 -1\n";
    }
    file.close();
}

// ------------------------------
// Interaction
// ------------------------------
void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 'r': case 'R':
        readFile(); break; // readFile() already snapshots
    case 'w': case 'W':
        writeFile(); break;
    case 'T': case 't':
        displayTangentVectors = !displayTangentVectors; break;
    case 'O': case 'o':
        displayObjects = !displayObjects; break;
    case 'P': case 'p':
        displayControlPoints = !displayControlPoints; break;
    case 'L': case 'l':
        displayControlLines = !displayControlLines; break;
    case 'C': case 'c':
        C1Continuity = !C1Continuity; break;
    case 'E': case 'e':
        // Clearing is a mutation => snapshot
        pushUndo();
        nPt = 0;
        segments.clear();
        break;
    case 'Z': case 'z':
        if (!popUndo()) cout << "Nothing to undo." << endl;
        break;
    case 'Q': case 'q':
        exit(0); break;
    default: break;
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{
    enum { MOUSE_LEFT_BUTTON = 0 };
    if ((button == MOUSE_LEFT_BUTTON) && (state == GLUT_UP))
    {
        if (nPt == MAXPTNO) {
            cout << "Error: Exceeded the maximum number of points." << endl;
            return;
        }
        // Mouse add is a mutation => snapshot
        pushUndo();

        // If the user is drawing manually, we treat it as one single segment (until file load).
        // So keep segments empty; buildActiveSegments() will treat [0..nPt) as a single segment.
        ptList[nPt++] = { x, y };
    }
    glutPostRedisplay();
}

// ------------------------------
// Main
// ------------------------------
int main(int argc, char** argv)
{
    cout << "CS3241 Lab 4 (Multi-segment + Undo)" << endl << endl;
    cout << "Left mouse click: Add a control point" << endl;
    cout << "Q: Quit" << endl;
    cout << "P: Toggle displaying control points" << endl;
    cout << "L: Toggle displaying control lines" << endl;
    cout << "E: Erase all points (Clear)" << endl;
    cout << "C: Toggle C1 continuity (within a segment)" << endl;
    cout << "T: Toggle displaying tangent vectors" << endl;
    cout << "O: Toggle displaying objects (Cute Cat)" << endl;
    cout << "Z: Undo last operation" << endl;
    cout << "R: Read control points from \"savefile.txt\" (multi-segment with -1 -1)" << endl;
    cout << "W: Write control points to \"savefile.txt\" (multi-segment with -1 -1)" << endl;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(600, 600);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("CS3241 Assignment 4 (Multi-segment + Undo)");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
