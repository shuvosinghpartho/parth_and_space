#include <GL/glut.h>
#include <bits/stdc++.h>
#include <fstream>
using namespace std;

const int W = 900, H = 700;
const float PI = acos(-1.0f);

// Game object capacity limits
const int MB = 16;   // max player bullets
const int ME = 10;   // max enemies
const int MA = 8;    // max asteroids
const int NS = 240;  // number of background stars
const int NPU = 6;   // max power-up pickups
const int NEB = 12;  // number of neon rings

const float SPDS = 4.5f;                      // player movement speed
const string SAVE_FILE = "parthnspace_save.dat"; // high score save file

// Forward declarations
void initGame();
void spawnBoss();
void spawnExplosion(float x, float y, float intensity = 1.0f,
                    float cr = 1.0f, float cg = 0.5f, float cb = 0.1f);
void triggerScreenShake(float intensity, int frames);
void spawnPowerUp(float x, float y);

// Enumerations 

// Top-level game states
enum State    { SPLASH, MENU, SETTINGS, PLAY, PAUSE, OVER, TRANSITION };

// Zone types that change the background and atmosphere
enum ZoneType { ZONE_SPACE, ZONE_ASTEROID, ZONE_NEBULA, ZONE_BOSS };

// Enemy movement / targeting modes
enum EnemyAI  { AI_SINE, AI_ZIGZAG, AI_TRACKER, AI_SWARM, AI_KAMIKAZE };

// Collectible power-up categories
enum PowerUpType { PU_RAPIDFIRE, PU_SHIELD, PU_SLOWMO, PU_LASER, PU_HOMING, PU_BOMB };

// Boss fight phases (affects speed, fire pattern, visuals)
enum BossPhase { BOSS_PHASE1, BOSS_PHASE2, BOSS_PHASE3 };

// Active state variables
State    state       = SPLASH;
State    nextState   = MENU;
ZoneType currentZone = ZONE_SPACE;

// Data Structures 

// Parallax background star
struct Star {
    float x, y, spd, bright, sz;
    int   layer;
};

// Expanding neon ring decoration
struct NeonRing {
    float x, y, r, alpha, spd;
    bool  on;
};

// Node used for motion trail rendering
struct TrailNode {
    float x, y, alpha, sz;
};

// Physics-based particle (used inside explosions)
struct Particle {
    float x, y, vx, vy, life, maxLife;
    float r, g, b, a, sz, gravity, friction;
    bool  on;
};

// Explosion: a scaling flash ring plus a cloud of particles
struct Explosion {
    float x, y, scale, alpha;
    bool  on;
    vector<Particle> pts;
    float r, g, b;
};

// Player-fired projectile (supports normal, laser, and homing modes)
struct Bullet {
    float x, y;
    bool  on, isLaser, isHoming;
    int   age;
    float hx, hy; // homing target position
};

// Enemy ship with multi-mode AI
struct Enemy {
    float x, y, baseY, spd, ang, flashTimer;
    bool  on;
    int   hp, maxHp;
    EnemyAI ai;
    float aiTimer, tx, ty;
    int   swarmIdx;
};

// Rotating asteroid obstacle
struct Asteroid {
    float x, y, spd, rot, rotSpd, r;
    bool  on;
    float flashTimer;
};

// Collectible power-up pickup
struct PowerUp {
    float   x, y, vy;
    PowerUpType type;
    bool    on;
    float   animT, bob;
};

// Bullet fired by an enemy or boss
struct EnemyBullet {
    float x, y, vx, vy;
    bool  on;
    int   age;
};

// Boss: multi-phase AI, orbital shield orbs, entry animation
struct Boss {
    float x, y, targetX, targetY, vx, vy;
    int   hp, maxHp;
    bool  on;
    BossPhase phase;
    float phaseTimer, shootCd;
    float ang, angSpd, flashTimer, entryProgress;
    bool  enraged;
    int   attackPattern;
    float patternTimer;
    float orbAng[4];  // angle of each shield orb
    bool  orbOn[4];   // whether each orb is still alive
    int   orbHp[4];   // hit points per orb
};

// Brief expanding ring shown on hit
struct HitRing {
    float x, y, r, alpha;
    bool  on;
    float cr, cg, cb;
};

// Global Game State 

// Player position, tilt angle, and scale
float sx = W / 2, sy = 80, sang = 0, sscale = 1;

// Directional movement flags
bool mL = 0, mR = 0, mU = 0, mD = 0;

// Core game counters
int score = 0, highScore = 0, lives = 3, wave = 1, fc = 0, bcool = 0;

// Combo system
int   comboCount = 0, comboTimer = 0;
float comboPop = 0;

// Shield state
int   shieldOn = 0, shieldCool = 0;
float shieldPulse = 0;

// Active power-up durations (frames remaining)
int puRapidFire = 0, puSlowMo = 0, puLaser = 0, puHoming = 0;

// Timing and screen-shake
float gameSpeed = 1.0f, slowMoTarget = 1.0f;
float shakeX = 0, shakeY = 0, shakeMag = 0;
int   shakeFrames = 0;

// Transition fade
float transAlpha = 0.0f;
int   transDir   = 1;

// UI animation timers
float menuAnim = 0, waveFlash = 0, scoreAnim = 0;
float splashT = 0, settingsAnim = 0;
int   settingsSel = 0, lastScore = 0;

// Settings values
int  difficulty = 1;     // 0=Easy, 1=Normal, 2=Hard
bool showRadar  = true;
bool showTrails = true;

// Background decoration timers
float planetRot = 0.0f, blackHoleT = 0.0f;
float sunT = 0.0f, moonT = 0.0f, compactPlanetT = 0.0f;

// Game object arrays
Boss        boss;
Bullet      bul[MB];
Enemy       ene[ME];
Asteroid    ast[MA];
Star        stars[NS];
NeonRing    rings[NEB];
PowerUp     pups[NPU];
EnemyBullet ebul[24];
HitRing     hitRings[16];
int         hitRingIdx = 0;

vector<Explosion> exps;

// Trail queues
deque<TrailNode> playerTrail;
deque<TrailNode> bulletTrails[MB];

// ── File I/O 

// Load the stored high score from disk on startup
void loadHighScore() {
    ifstream f(SAVE_FILE);
    if (f.is_open()) { f >> highScore; f.close(); }
}

// Write a new high score to disk if the current score beats it
void saveHighScore() {
    if (score > highScore) {
        highScore = score;
        ofstream f(SAVE_FILE);
        if (f.is_open()) { f << highScore; f.close(); }
    }
}

// ── Math Helpers 

float frand(float lo, float hi) { return lo + (hi - lo) * (rand() / (float)RAND_MAX); }
float dist(float ax, float ay, float bx, float by) { return hypotf(ax - bx, ay - by); }
float lerp(float a, float b, float t) { return a + (b - a) * t; }
float clamp(float v, float lo, float hi) { return max(lo, min(hi, v)); }

// ── Rasterisation Primitives 

// DDA line algorithm — incremental floating-point scan
void dda(int x1, int y1, int x2, int y2) {
    int dx = x2 - x1, dy = y2 - y1;
    int steps = max(abs(dx), abs(dy));
    if (!steps) return;
    float xi = dx / (float)steps, yi = dy / (float)steps, x = x1, y = y1;
    glBegin(GL_POINTS);
    for (int i = 0; i <= steps; i++, x += xi, y += yi)
        glVertex2f(roundf(x), roundf(y));
    glEnd();
}

// Bresenham's line algorithm — integer-only, error-accumulation method
void bres(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1, err = dx - dy;
    glBegin(GL_POINTS);
    while (true) {
        glVertex2f(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 <  dx) { err += dx; y1 += sy; }
    }
    glEnd();
}

// Midpoint circle algorithm — plots 8-way symmetric points per step
void mpc(int cx, int cy, int r) {
    if (r <= 0) return;
    int x = 0, y = r, p = 1 - r;
    auto p8 = [&](int px, int py) {
        glVertex2f(cx + px, cy + py); glVertex2f(cx - px, cy + py);
        glVertex2f(cx + px, cy - py); glVertex2f(cx - px, cy - py);
        glVertex2f(cx + py, cy + px); glVertex2f(cx - py, cy + px);
        glVertex2f(cx + py, cy - px); glVertex2f(cx - py, cy - px);
    };
    glBegin(GL_POINTS);
    p8(x, y);
    while (x < y) {
        x++;
        p = (p < 0) ? p + 2 * x + 1 : p + 2 * (x - --y) + 1;
        p8(x, y);
    }
    glEnd();
}

// Solid circle fill using concentric midpoint circles
void mpcFill(int cx, int cy, int r) {
    for (int i = 1; i <= r; i++) mpc(cx, cy, i);
}

// Glow / Neon Helpers

// Renders a glowing circle: soft outer halo + solid filled core
void glowCircle(int cx, int cy, int coreR, float r, float g, float b,
                float coreAlpha, int layers = 5) {
    for (int i = layers; i >= 1; i--) {
        float t = i / (float)layers;
        glColor4f(r, g, b, coreAlpha * t * 0.35f);
        mpc(cx, cy, coreR + layers * 3 - i * 3);
    }
    glColor4f(r, g, b, coreAlpha);
    mpcFill(cx, cy, coreR);
}

// Renders a thick glowing line using offset DDA passes
void glowLine(int x1, int y1, int x2, int y2,
              float r, float g, float b, float a, int thick = 2) {
    for (int t = thick; t >= 1; t--) {
        glColor4f(r, g, b, a * t / (float)thick * 0.5f);
        for (int off = -t; off <= t; off++) {
            dda(x1, y1 + off, x2, y2 + off);
            dda(x1 + off, y1, x2 + off, y2);
        }
    }
    glColor4f(r, g, b, a);
    dda(x1, y1, x2, y2);
}

// Text Helpers 

// Render a string at position (x, y) using the given GLUT bitmap font
void txt(float x, float y, const string &s, void *f = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (char c : s) glutBitmapCharacter(f, c);
}

// Return the pixel width of a string in the given GLUT bitmap font
int strWidth(const string &s, void *f) {
    int w = 0;
    for (char c : s) w += glutBitmapWidth(f, c);
    return w;
}

// Render a horizontally centred string at (cx, y)
void txtC(float cx, float y, const string &s, void *f = GLUT_BITMAP_HELVETICA_18) {
    txt(cx - strWidth(s, f) / 2.0f, y, s, f);
}

// Render centred text with a dark drop-shadow for readability
void txtShadowC(float cx, float y, const string &s,
                float r, float g, float b, void *f = GLUT_BITMAP_HELVETICA_18) {
    float x = cx - strWidth(s, f) / 2.0f;
    glColor4f(0, 0, 0, 0.75f);
    txt(x + 2, y - 2, s, f);
    glColor3f(r, g, b);
    txt(x, y, s, f);
}

// Render centred text with a bloom halo and drop-shadow
void txtGlow(float cx, float y, const string &s,
             float r, float g, float b, float glow,
             void *f = GLUT_BITMAP_TIMES_ROMAN_24) {
    float x = cx - strWidth(s, f) / 2.0f;
    glColor4f(0, 0, 0, 0.8f);
    txt(x + 2, y - 2, s, f);
    // soft halo passes
    glColor4f(r, g, b, glow * 0.3f);
    txt(x - 1, y + 1, s, f); txt(x + 1, y + 1, s, f);
    txt(x - 1, y - 1, s, f); txt(x + 1, y - 1, s, f);
    glColor3f(r + glow * 0.2f, g + glow * 0.1f, b);
    txt(x, y, s, f);
}

// Render each character offset vertically by a sine wave (animation effect)
void txtWave(float cx, float y, const string &s,
             float r, float g, float b,
             float waveAmt, float phase, void *f = GLUT_BITMAP_HELVETICA_18) {
    float x = cx - strWidth(s, f) / 2.0f;
    for (int i = 0; i < (int)s.size(); i++) {
        glColor3f(r, g, b);
        glRasterPos2f(x, y + waveAmt * sinf(phase + i * 0.4f));
        glutBitmapCharacter(f, s[i]);
        x += glutBitmapWidth(f, s[i]);
    }
}

// Screen Shake

// Start a camera shake of given intensity lasting the given frame count
void triggerScreenShake(float intensity, int frames) {
    shakeMag    = intensity;
    shakeFrames = frames;
}

// Apply random offset each frame and decay the shake magnitude
void applyShake() {
    if (shakeFrames > 0) {
        shakeX = frand(-shakeMag, shakeMag);
        shakeY = frand(-shakeMag, shakeMag);
        shakeFrames--;
        shakeMag *= 0.88f;
    } else {
        shakeX = shakeY = 0;
    }
}

// Zone Backgrounds 
// Each background uses two quad gradient fills to simulate depth.

void drawBackgroundSpace() {
    glBegin(GL_QUADS);
    glColor3f(0.00f, 0.00f, 0.05f); glVertex2f(0, 0);  glVertex2f(W, 0);
    glColor3f(0.01f, 0.00f, 0.09f); glVertex2f(W, H/2); glVertex2f(0, H/2);
    glEnd();
    glBegin(GL_QUADS);
    glColor3f(0.01f, 0.00f, 0.09f); glVertex2f(0, H/2); glVertex2f(W, H/2);
    glColor3f(0.03f, 0.00f, 0.12f); glVertex2f(W, H);  glVertex2f(0, H);
    glEnd();
}

void drawBackgroundAsteroid() {
    glBegin(GL_QUADS);
    glColor3f(0.04f, 0.02f, 0.00f); glVertex2f(0, 0);  glVertex2f(W, 0);
    glColor3f(0.07f, 0.04f, 0.01f); glVertex2f(W, H/2); glVertex2f(0, H/2);
    glEnd();
    glBegin(GL_QUADS);
    glColor3f(0.07f, 0.04f, 0.01f); glVertex2f(0, H/2); glVertex2f(W, H/2);
    glColor3f(0.05f, 0.03f, 0.01f); glVertex2f(W, H);  glVertex2f(0, H);
    glEnd();
    // faint dust-band strips
    for (int i = 0; i < 4; i++) {
        float y0 = 100 + i * 150;
        glColor4f(0.15f, 0.10f, 0.04f, 0.04f);
        glBegin(GL_QUADS);
        glVertex2f(0, y0);     glVertex2f(W, y0 + 30);
        glVertex2f(W, y0 + 55); glVertex2f(0, y0 + 25);
        glEnd();
    }
}

void drawBackgroundNebula() {
    glBegin(GL_QUADS);
    glColor3f(0.00f, 0.03f, 0.05f); glVertex2f(0, 0);  glVertex2f(W, 0);
    glColor3f(0.00f, 0.06f, 0.10f); glVertex2f(W, H/2); glVertex2f(0, H/2);
    glEnd();
    glBegin(GL_QUADS);
    glColor3f(0.00f, 0.06f, 0.10f); glVertex2f(0, H/2); glVertex2f(W, H/2);
    glColor3f(0.02f, 0.08f, 0.10f); glVertex2f(W, H);  glVertex2f(0, H);
    glEnd();
}

void drawBackgroundBoss() {
    glBegin(GL_QUADS);
    glColor3f(0.06f, 0.00f, 0.00f); glVertex2f(0, 0);  glVertex2f(W, 0);
    glColor3f(0.10f, 0.00f, 0.01f); glVertex2f(W, H/2); glVertex2f(0, H/2);
    glEnd();
    glBegin(GL_QUADS);
    glColor3f(0.10f, 0.00f, 0.01f); glVertex2f(0, H/2); glVertex2f(W, H/2);
    glColor3f(0.08f, 0.01f, 0.02f); glVertex2f(W, H);  glVertex2f(0, H);
    glEnd();
}

// Dispatch to the correct background based on the active zone
void drawBackground() {
    switch (currentZone) {
        case ZONE_SPACE:    drawBackgroundSpace();    break;
        case ZONE_ASTEROID: drawBackgroundAsteroid(); break;
        case ZONE_NEBULA:   drawBackgroundNebula();   break;
        case ZONE_BOSS:     drawBackgroundBoss();     break;
    }
}

// Nebula Clouds
// Zone-specific soft cloud blobs drawn with concentric midpoint circles.
void drawNebula() {
    if (currentZone == ZONE_SPACE) {
        for (int r = 120; r > 0; r -= 4) {
            float t = r / 120.0f;
            glColor4f(0.28f*t, 0.05f*t, 0.55f*t, 0.018f*(1-t)); mpc(180, 440, r);
        }
        for (int r = 95; r > 0; r -= 4) {
            float t = r / 95.0f;
            glColor4f(0.0f, 0.25f*t, 0.55f*t, 0.016f*(1-t)); mpc(700, 240, r);
        }
    } else if (currentZone == ZONE_NEBULA) {
        for (int r = 150; r > 0; r -= 3) {
            float t = r / 150.0f;
            glColor4f(0.0f, 0.4f*t, 0.6f*t, 0.022f*(1-t)); mpc(200, 400, r);
        }
        for (int r = 120; r > 0; r -= 3) {
            float t = r / 120.0f;
            glColor4f(0.1f*t, 0.5f*t, 0.3f*t, 0.020f*(1-t)); mpc(680, 280, r);
        }
        for (int r = 100; r > 0; r -= 3) {
            float t = r / 100.0f;
            glColor4f(0.2f*t, 0.45f*t, 0.55f*t, 0.018f*(1-t)); mpc(450, 180, r);
        }
    } else if (currentZone == ZONE_BOSS) {
        // Pulsing red/orange clouds to emphasise danger
        for (int r = 140; r > 0; r -= 4) {
            float t = r/140.0f, pulse = 0.5f + 0.5f*sinf(fc*0.02f);
            glColor4f(0.5f*t*pulse, 0.0f, 0.05f*t, 0.018f*(1-t)); mpc(180, 350, r);
        }
        for (int r = 110; r > 0; r -= 4) {
            float t = r/110.0f, pulse = 0.5f + 0.5f*sinf(fc*0.025f+1.5f);
            glColor4f(0.4f*t*pulse, 0.05f*t, 0.0f, 0.016f*(1-t)); mpc(700, 380, r);
        }
    }
}

// Stars 

// Initialise the star field with three parallax layers
void initStars() {
    for (int i = 0; i < NS; i++) {
        int l = i % 3;
        stars[i] = { frand(0,W), frand(0,H),
                     l==0 ? 0.25f : l==1 ? 0.7f  : 1.6f,
                     l==0 ? 0.35f : l==1 ? 0.65f : 1.0f,
                     l==0 ? 1.0f  : l==1 ? 1.5f  : 2.5f,
                     l };
    }
}

// Draw stars with per-zone tint and a twinkle effect
void drawStars() {
    for (auto &s : stars) {
        float tw = s.bright * (0.8f + 0.2f * sinf(fc * 0.07f + s.x));
        float tr = tw, tg = tw, tb = tw;
        if (currentZone == ZONE_NEBULA) { tr = tw*0.7f; tb = tw*1.1f; }
        if (currentZone == ZONE_BOSS)   { tr = tw*1.2f; tg = tw*0.7f; tb = tw*0.7f; }
        glColor3f(tr, tg, tb);
        glPointSize(s.sz);
        glBegin(GL_POINTS); glVertex2f(s.x, s.y); glEnd();
        // bright stars get a soft halo
        if (s.layer == 2) {
            glColor4f(tr*0.8f, tg*0.85f, tb, 0.18f);
            glPointSize(s.sz * 2.5f);
            glBegin(GL_POINTS); glVertex2f(s.x, s.y); glEnd();
        }
    }
    glPointSize(1);
}

// Decorative Planet 
// Draws a large purple planet with orbital rings and an orbiting moon.
// Hidden during the boss fight so it doesn't distract the player.
void drawPlanet() {
    if (currentZone == ZONE_BOSS) return;
    const int px = 720, py = 560, pr = 90;

    // atmosphere halo
    for (int r = pr+18; r >= pr+1; r--) {
        float t = (r-pr)/18.0f;
        glColor4f(0.4f, 0.1f, 0.85f, 0.06f*(1-t)); mpc(px, py, r);
    }
    // planet body gradient
    for (int r = pr; r > 0; r -= 2) {
        float t = 1.0f - r/(float)pr;
        glColor3f(0.05f+0.35f*t, 0.0f+0.05f*t, 0.18f+0.38f*t); mpc(px, py, r);
    }
    glColor4f(0.6f, 0.5f, 1.0f, 0.25f);
    for (int r = pr-4; r <= pr-1; r++) mpc(px-18, py+18, r);

    // animated orbital ring bands (Bresenham lines)
    float roff = sinf(planetRot * 0.02f) * 6;
    glColor4f(0.55f, 0.22f, 1.0f, 0.45f);
    for (int b = -4; b <= 4; b++) {
        int off = (int)sqrtf(max(0.0f,(float)(pr*pr - (b*13)*(b*13))));
        bres(px-off-10, py+b*13+(int)roff, px+off+10, py+b*13+(int)roff);
    }
    glColor4f(0.8f, 0.5f, 1.0f, 0.18f);
    for (int b = -1; b <= 1; b++) {
        int off = (int)sqrtf(max(0.0f,(float)(pr*pr - (b*13)*(b*13))));
        bres(px-off-10, py+b*13+(int)roff, px+off+10, py+b*13+(int)roff);
    }

    // small orbiting moon (polar coordinate translation)
    float moonAng = planetRot * 0.008f;
    int mx = px+(int)(115*cosf(moonAng)), my = py+(int)(60*sinf(moonAng));
    glColor3f(0.55f, 0.52f, 0.58f);
    mpcFill(mx, my, 12);
    glColor4f(0.8f, 0.8f, 1.0f, 0.15f);
    for (int r = 13; r <= 16; r++) mpc(mx, my, r);
}

// Black Hole (Boss Arena)
// Draws an accretion-disc black hole with a pulsing jet effect.
void drawBlackHole() {
    if (currentZone != ZONE_BOSS) return;
    const int bx = 150, by = 580, br = 55;
    float pulse = 0.5f + 0.5f * sinf(blackHoleT * 0.04f);

    // accretion disc gradient
    for (int r = br+40; r >= br+5; r--) {
        float t = (r-br-5)/35.0f;
        float a = (r <= br+15) ? 0.25f*(1-t)*pulse : 0.06f*(1-t);
        glColor4f(0.6f+0.3f*sinf(t*PI), 0.2f, 0.05f, a); mpc(bx, by, r);
    }
    // event horizon
    glColor3f(0,0,0); mpcFill(bx, by, br);
    glColor4f(0.9f, 0.3f, 0.1f, 0.6f+0.35f*pulse);
    mpc(bx, by, br); mpc(bx, by, br+1);

    // relativistic jet rays
    for (int i = 0; i < 8; i++) {
        float ang = i*PI/4 + blackHoleT*0.01f, len = 18+10*pulse;
        glColor4f(1.0f, 0.5f, 0.1f, 0.2f*pulse);
        dda(bx+(int)(br*cosf(ang)), by+(int)(br*sinf(ang)),
            bx+(int)((br+len)*cosf(ang)), by+(int)((br+len)*sinf(ang)));
    }
}

//  Neon Rings 

// Place the ring origins across the play area
void initRings() {
    float xs[] = {200,680,450,150,750,500,350,600,100,800,250,700};
    float ys[] = {350,200,100,580,500,380,250,450,300,350,480,600};
    float sp[] = {0.4f,0.6f,0.5f,0.3f,0.45f,0.35f,0.55f,0.4f,0.65f,0.3f,0.5f,0.45f};
    for (int i = 0; i < NEB; i++)
        rings[i] = {xs[i], ys[i], 0, 0, sp[i], true};
}

// Expand each ring radius; reset when it fades out
void updateRings() {
    for (auto &rg : rings) {
        rg.r    += rg.spd;
        rg.alpha = max(0.0f, 0.22f - rg.r/300.0f);
        if (rg.r > 280) { rg.r = 0; rg.alpha = 0.20f; }
    }
}

// Draw rings with a colour tint that matches the current zone
void drawRings() {
    float cr = 0.2f, cg = 0.5f, cb = 1.0f;
    if (currentZone == ZONE_NEBULA) { cr=0.1f; cg=0.8f; cb=0.7f; }
    if (currentZone == ZONE_BOSS)   { cr=0.8f; cg=0.1f; cb=0.2f; }
    for (auto &rg : rings) {
        glColor4f(cr, cg, cb, rg.alpha*0.6f);
        mpc((int)rg.x, (int)rg.y, (int)rg.r);
        glColor4f(cr+0.2f, cg+0.2f, cb, rg.alpha*0.3f);
        mpc((int)rg.x, (int)rg.y, (int)rg.r+2);
    }
}

// Player Ship 
// Polygon hull assembled from glBegin(GL_POLYGON) calls.
// Wing colour shifts based on the active power-up.
// Engine exhaust length changes when RapidFire is active.
void drawShip(float x, float y, float ang, float sc) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glRotatef(ang, 0, 0, 1);
    glScalef(sc, sc, 1);

    bool flk = (fc % 6 < 3); // engine flicker flag

    // engine glow blobs
    glColor4f(0.2f, 0.5f, 1.0f, 0.12f);
    for (int r = 14; r >= 6; r--) { mpc(-12,-28,r); mpc(12,-28,r); }

    // exhaust plume (DDA lines)
    float exLen = flk ? -50 : -43;
    if (puRapidFire > 0) exLen -= 8;
    glColor3f(0.1f, 0.4f, 1.0f);
    dda(-11,-28,-13,(int)exLen); dda(11,-28,13,(int)exLen);
    glColor3f(0.4f, 0.8f, 1.0f);
    dda(-11,-28,-12,(int)(exLen+6)); dda(11,-28,12,(int)(exLen+6));
    glColor3f(0.9f, 0.95f, 1.0f);
    dda(-11,-28,-11,(int)(exLen+12)); dda(11,-28,11,(int)(exLen+12));

    // wing colour — tinted by active power-up
    float wr=0.3f, wg=0.7f, wb=1.0f;
    if (puRapidFire>0) { wr=1.0f; wg=0.8f; wb=0.2f; }
    if (puSlowMo>0)    { wr=0.5f; wg=0.2f; wb=1.0f; }
    if (puLaser>0)     { wr=1.0f; wg=0.3f; wb=0.8f; }
    if (puHoming>0)    { wr=0.2f; wg=1.0f; wb=0.6f; }

    glColor4f(wr, wg, wb, 0.15f);
    for (int r = 8; r >= 2; r--) { mpc(-18,-20,r); mpc(18,-20,r); }

    // main hull polygon
    glColor3f(0.45f, 0.80f, 1.0f);
    glBegin(GL_POLYGON);
    glVertex2f(0,42); glVertex2f(-12,12); glVertex2f(-16,-18);
    glVertex2f(0,-8); glVertex2f(16,-18); glVertex2f(12,12);
    glEnd();
    glColor4f(0.8f, 0.95f, 1.0f, 0.6f);
    dda(0,42,-12,12); dda(0,42,12,12);

    // cockpit window
    glColor3f(0.08f, 0.22f, 0.70f);
    glBegin(GL_POLYGON);
    glVertex2f(0,30); glVertex2f(-6,16); glVertex2f(0,12); glVertex2f(6,16);
    glEnd();
    glColor4f(0.7f, 0.9f, 1.0f, 0.5f);
    dda(-3,27,0,20);

    // left wing
    glColor3f(0.15f, 0.50f, 0.90f);
    glBegin(GL_POLYGON);
    glVertex2f(-12,12); glVertex2f(-16,-18); glVertex2f(-22,-24); glVertex2f(-9,8);
    glEnd();
    // right wing
    glBegin(GL_POLYGON);
    glVertex2f(12,12); glVertex2f(16,-18); glVertex2f(22,-24); glVertex2f(9,8);
    glEnd();

    // wing tip accent lines
    glColor3f(wr, wg, wb);
    dda(-22,-24,-28,-32); dda(-28,-32,-11,-27);
    dda( 22,-24, 28,-32); dda( 28,-32, 11,-27);

    // engine nacelle glow rings
    glColor4f(wr, wg, wb, 0.5f);
    mpc(-11,-28,5); mpc(11,-28,5);
    glColor4f(0.6f, 0.9f, 1.0f, 0.7f);
    dda(0,38,0,8);

    glPopMatrix();
}

// Player Trail 

// Push the current player position onto the trail deque each frame
void updatePlayerTrail() {
    if (!showTrails) return;
    playerTrail.push_front({sx, sy, 1.0f, 3.0f});
    if (playerTrail.size() > 22) playerTrail.pop_back();
    for (auto &n : playerTrail) n.alpha *= 0.82f;
}

// Render the trail as fading points; colour reflects active power-up
void drawPlayerTrail() {
    if (!showTrails) return;
    for (int i = 0; i < (int)playerTrail.size(); i++) {
        auto &n = playerTrail[i];
        float t  = 1.0f - i / (float)playerTrail.size();
        float tr = 0.3f, tg = 0.7f, tb = 1.0f;
        if (puRapidFire>0) { tr=1.0f; tg=0.5f; tb=0.1f; }
        if (puSlowMo>0)    { tr=0.4f; tg=0.2f; tb=1.0f; }
        glColor4f(tr, tg, tb, n.alpha*t*0.5f);
        glPointSize(n.sz*t);
        glBegin(GL_POINTS); glVertex2f(n.x, n.y); glEnd();
    }
    glPointSize(1);
}

// Shield Bubble 
// Pulsing translucent dome drawn around the player when active.
void drawShield(float x, float y) {
    float p = 0.5f + 0.5f * sinf(shieldPulse * 0.18f);
    // outer glow halo
    for (int r = 52; r >= 44; r--) {
        glColor4f(0.1f, 0.7f, 1.0f, 0.04f*(1-(r-44)/8.0f)); mpc((int)x,(int)y,r);
    }
    // translucent dome fill
    glColor4f(0.15f, 0.65f, 1.0f, 0.08f+0.05f*p);
    mpcFill((int)x,(int)y,40);
    // bright rim
    glColor4f(0.2f, 0.9f, 1.0f, 0.7f+0.25f*p);
    mpc((int)x,(int)y,40); mpc((int)x,(int)y,41);
    // hex-segment lines on the rim
    for (int i = 0; i < 12; i++) {
        float a1=i*30*PI/180, a2=(i+1)*30*PI/180;
        glColor4f(0.5f,1.0f,1.0f,0.5f+0.3f*p);
        bres((int)(x+43*cosf(a1)),(int)(y+43*sinf(a1)),
             (int)(x+43*cosf(a2)),(int)(y+43*sinf(a2)));
    }
}

// Enemy Ship 
// Colour-coded by AI type; includes a hit-flash and a per-HP bar.
void drawEnemy(const Enemy &e) {
    glPushMatrix();
    glTranslatef(e.x, e.y, 0);
    glRotatef(e.ang, 0, 0, 1);

    float flash = min(1.0f, e.flashTimer/8.0f);

    // base colour per AI variant
    float er=0.9f, eg=0.15f, eb=0.28f;
    switch (e.ai) {
        case AI_TRACKER:  er=1.0f; eg=0.5f; eb=0.0f; break;
        case AI_SWARM:    er=0.7f; eg=0.0f; eb=1.0f; break;
        case AI_KAMIKAZE: er=1.0f; eg=0.1f; eb=0.1f; break;
        case AI_ZIGZAG:   er=0.0f; eg=0.8f; eb=0.5f; break;
        default: break;
    }

    // engine glow
    glColor4f(er, eg+0.15f, eb, 0.25f);
    for (int r = 10; r >= 3; r--) mpc(0, 20, r);

    // hull polygon
    glColor3f(er+flash*0.1f, eg*(1-flash*0.5f), eb*(1-flash*0.5f));
    glBegin(GL_POLYGON);
    glVertex2f(0,-32); glVertex2f(-14,-6); glVertex2f(-18,18);
    glVertex2f(0,10);  glVertex2f( 18,18); glVertex2f( 14,-6);
    glEnd();

    // wing panels
    glColor3f(er*0.6f, eg*0.4f, eb*0.5f);
    glBegin(GL_POLYGON);
    glVertex2f(-14,-6); glVertex2f(-18,18); glVertex2f(-24,24); glVertex2f(-8,-2);
    glEnd();
    glBegin(GL_POLYGON);
    glVertex2f(14,-6); glVertex2f(18,18); glVertex2f(24,24); glVertex2f(8,-2);
    glEnd();

    // pulsing core orb
    float pulse = 0.5f + 0.5f*sinf(fc*0.1f + e.x);
    glColor3f(er, eg+0.3f*pulse, eb*0.5f);
    mpcFill(0, 0, 7);
    glColor4f(er, eg+0.5f, eb, 0.4f*pulse);
    mpc(0,0,10); mpc(0,0,11);

    // HP bar (only for multi-HP enemies)
    if (e.maxHp > 1) {
        float hfrac = (float)e.hp / e.maxHp;
        const int bw=28, bh=3, bx=-(bw/2), by=38;
        glColor4f(0.2f,0.0f,0.0f,0.7f);
        glBegin(GL_QUADS);
        glVertex2f(bx,by); glVertex2f(bx+bw,by);
        glVertex2f(bx+bw,by+bh); glVertex2f(bx,by+bh);
        glEnd();
        glColor3f(er, eg, eb);
        glBegin(GL_QUADS);
        glVertex2f(bx,by); glVertex2f(bx+bw*hfrac,by);
        glVertex2f(bx+bw*hfrac,by+bh); glVertex2f(bx,by+bh);
        glEnd();
    }

    glColor3f(er*1.1f, eg+0.45f, eb);
    dda(-18,18,-26,28); dda(18,18,26,28);
    bres(-8,12,8,12);

    glPopMatrix();
}

// Enemy Bullet 
// Glowing orange plasma orb with pulsing outer halo.
void drawEnemyBullet(const EnemyBullet &b) {
    int bx=(int)b.x, by=(int)b.y;
    float pulse = 0.5f + 0.5f*sinf(b.age*0.3f);
    for (int r = 8; r >= 4; r--) {
        glColor4f(1.0f, 0.3f, 0.1f, 0.08f*(1-(r-4)/4.0f)); mpc(bx, by, r);
    }
    glColor4f(1.0f, 0.5f, 0.1f, 0.7f); mpcFill(bx, by, 3);
    glColor4f(1.0f, 0.9f, 0.5f, 1.0f); mpcFill(bx, by, 1);
}

// Asteroid 
// Irregular polygon with craters; rotates each frame.
void drawAst(const Asteroid &a) {
    glPushMatrix();
    glTranslatef(a.x, a.y, 0);
    glRotatef(a.rot, 0, 0, 1);

    float flash = min(1.0f, a.flashTimer/8.0f);
    // outer silhouette
    glColor4f(0.7f+flash*0.3f, 0.55f+flash*0.2f, 0.35f, 0.2f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 10; i++) {
        float ang = i*2*PI/10;
        float rad = (a.r+3)*(0.80f+0.20f*sinf(ang*3+a.rot*0.01f));
        glVertex2f(rad*cosf(ang), rad*sinf(ang));
    }
    glEnd();
    // main body
    glColor3f(0.50f+flash*0.3f, 0.43f, 0.33f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 10; i++) {
        float ang = i*2*PI/10;
        float rad = a.r*(0.75f+0.25f*sinf(ang*3+a.rot*0.01f));
        glVertex2f(rad*cosf(ang), rad*sinf(ang));
    }
    glEnd();
    // surface craters (midpoint circles)
    glColor3f(0.30f, 0.25f, 0.18f);
    mpc((int)(a.r/3), (int)(a.r/4), (int)(a.r/4));
    mpc(-(int)(a.r/4), -(int)(a.r/3), (int)(a.r/5));
    // edge highlight
    glColor4f(0.75f, 0.7f, 0.6f, 0.35f);
    for (int i = 6; i <= 8; i++) mpc(-(int)(a.r/3), (int)(a.r/3), i);

    glPopMatrix();
}

// Player Bullet (Plasma Lance) 
// Normal mode: animated energy shaft with a crystalline tip.
// Laser mode:  full-height screen-spanning beam.
// Homing mode: adds a green tracking ring.
void drawBullet(const Bullet &b) {
    int bx=(int)b.x, by=(int)b.y;
    float pulse  = 0.5f + 0.5f*sinf(b.age*0.45f);
    float pulse2 = 0.5f + 0.5f*sinf(b.age*0.28f+1.2f);

    if (b.isLaser) {
        // screen-spanning laser beam
        for (int i = 0; i < 6; i++) {
            float t = i/5.0f;
            glColor4f(1.0f, 0.3f+0.5f*(1-t), 0.8f*(1-t), 0.15f*(1-t));
            dda(bx-4+i, by, bx-4+i, by+H);
        }
        glColor4f(1.0f, 0.6f, 1.0f, 0.8f); dda(bx,by,bx,by+H);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f); dda(bx,by+5,bx,by+H-5);
        return;
    }

    // homing indicator ring
    if (b.isHoming) {
        glColor4f(0.2f, 1.0f, 0.4f, 0.3f*pulse);
        mpc(bx,by,10); mpc(bx,by,11);
    }

    // energy trail segments (fading backward)
    for (int seg = 0; seg < 10; seg++) {
        float t=seg/9.0f, alp=(1.0f-t)*0.10f*(0.7f+0.3f*pulse);
        int rad=(int)(6-t*4), ty=by-4-seg*7;
        glColor4f(0.0f, 0.85f, 1.0f, alp);
        if (rad>0) mpc(bx, ty, rad);
    }
    for (int seg=0; seg<9; seg++) {
        float t=seg/8.0f, alp=(1.0f-t)*0.55f;
        glColor4f(0.15f, 0.65f+0.2f*pulse, 1.0f, alp);
        dda(bx, by-2-seg*6, bx, by-2-seg*6-5);
    }
    for (int seg=0; seg<8; seg++) {
        float t=seg/7.0f, alp=(1.0f-t)*0.80f;
        glColor4f(0.6f, 0.95f, 1.0f, alp);
        dda(bx, by-seg*5, bx, by-seg*5-4);
    }

    // side spark flickers
    for (int i = 0; i < 5; i++) {
        float phase=b.age*0.5f+i*1.3f, sp=sinf(phase);
        if (fabsf(sp)<0.3f) continue;
        int oy=by-6-i*8, len=(int)(3+2*fabsf(sp));
        glColor4f(0.3f, 0.9f, 1.0f, fabsf(sp)*0.7f);
        dda(bx-len, oy, bx-1, oy); dda(bx+1, oy, bx+len, oy);
    }

    // outer glow rings
    for (int r=11; r>=6; r--) {
        glColor4f(0.0f, 0.7f+0.2f*pulse, 1.0f,
                  0.06f*(1-(r-6)/5.0f)*(0.8f+0.2f*pulse)); mpc(bx,by,r);
    }
    for (int r=6; r>=3; r--) {
        glColor4f(0.15f,0.9f,1.0f,0.14f*(r/6.0f)*pulse2); mpc(bx,by,r);
    }

    // main shaft lines
    glColor4f(0.2f,0.95f,1.0f,0.9f);
    dda(bx-1,by-2,bx-1,by+18); dda(bx+1,by-2,bx+1,by+18);
    glColor4f(0.5f,1.0f,1.0f,1.0f); dda(bx,by,bx,by+20);
    glColor4f(1.0f,1.0f,1.0f,1.0f); dda(bx,by+2,bx,by+16);

    // crystalline diamond tip
    int tx2=bx, ty2=by+22;
    for (int r=9; r>=4; r--) {
        glColor4f(0.0f,0.8f+0.2f*pulse,1.0f,0.18f*(1-(r-4)/5.0f)*pulse);
        mpc(tx2,ty2,r);
    }
    glColor4f(0.4f,1.0f,1.0f,0.85f+0.15f*pulse); mpcFill(tx2,ty2,4);
    glColor4f(0.7f,1.0f,1.0f,0.9f);
    dda(tx2,ty2+7,tx2-4,ty2+1); dda(tx2,ty2+7,tx2+4,ty2+1);
    dda(tx2-4,ty2+1,tx2,ty2-4); dda(tx2+4,ty2+1,tx2,ty2-4);
    glColor4f(1.0f,1.0f,1.0f,1.0f); mpcFill(tx2,ty2+6,2);

    // tip accent lines
    float fl=0.4f+0.6f*pulse;
    glColor4f(0.3f,0.95f,1.0f,fl*0.7f); dda(tx2-6,ty2+6,tx2+6,ty2+6);
    glColor4f(0.5f,1.0f,1.0f,fl*0.5f);
    dda(tx2-4,ty2+9,tx2+4,ty2+3); dda(tx2-4,ty2+3,tx2+4,ty2+9);

    // wing sparks at mid-shaft
    glColor4f(0.2f,0.85f,1.0f,0.5f+0.3f*pulse2);
    dda(bx-1,by+12,bx-5,by+6); dda(bx+1,by+12,bx+5,by+6);
    glColor4f(0.5f,1.0f,1.0f,0.35f);
    dda(bx-1,by+14,bx-7,by+8); dda(bx+1,by+14,bx+7,by+8);

    // front glow ring
    glColor4f(0.1f,0.7f,1.0f,0.3f*pulse); mpc(bx,by-2,5);
    glColor4f(0.3f,0.9f,1.0f,0.15f*pulse); mpc(bx,by-2,7);
}

//  Power-Up Pickup
// Bobbing hexagonal icon with a label abbreviation.
void drawPowerUp(const PowerUp &p) {
    if (!p.on) return;
    int px=(int)p.x, py=(int)(p.y + sinf(p.bob)*5.0f);
    float pulse = 0.5f + 0.5f*sinf(p.animT*0.15f);

    float r, g, b;
    string label;
    switch (p.type) {
        case PU_RAPIDFIRE: r=1.0f; g=0.8f; b=0.1f; label="RF"; break;
        case PU_SHIELD:    r=0.1f; g=0.8f; b=1.0f; label="SH"; break;
        case PU_SLOWMO:    r=0.5f; g=0.1f; b=1.0f; label="SM"; break;
        case PU_LASER:     r=1.0f; g=0.2f; b=0.9f; label="LZ"; break;
        case PU_HOMING:    r=0.2f; g=1.0f; b=0.4f; label="HM"; break;
        case PU_BOMB:      r=1.0f; g=0.3f; b=0.1f; label="BM"; break;
        default:           r=g=b=1.0f; label="??"; break;
    }

    // outer halo
    for (int rad=22; rad>=16; rad--) {
        glColor4f(r,g,b,0.12f*(1-(rad-16)/6.0f)*pulse); mpc(px,py,rad);
    }
    // body fill and rotating hex frame
    glColor4f(r,g,b,0.3f+0.2f*pulse); mpcFill(px,py,12);
    glColor4f(r,g,b,0.8f+0.2f*pulse);
    for (int i = 0; i < 6; i++) {
        float a1=i*60*PI/180+p.animT*0.02f, a2=(i+1)*60*PI/180+p.animT*0.02f;
        bres(px+(int)(14*cosf(a1)),py+(int)(14*sinf(a1)),
             px+(int)(14*cosf(a2)),py+(int)(14*sinf(a2)));
    }
    // bright centre dot and label
    glColor4f(r+0.3f,g+0.2f,b+0.1f,1.0f); mpcFill(px,py,5);
    glColor4f(1.0f,1.0f,1.0f,0.9f);
    txtC(px, py-3, label, GLUT_BITMAP_HELVETICA_12);
}

// Boss
// Hexagonal layered body, four orbital shield orbs, multi-phase
// HP bar, and a Phase III outer ring burst pattern.
void drawBoss() {
    if (!boss.on) return;
    int bx=(int)boss.x, by=(int)boss.y;
    float flash  = min(1.0f, boss.flashTimer/10.0f);
    float pulse  = 0.5f + 0.5f*sinf(fc*0.08f);
    float hfrac  = (float)boss.hp / boss.maxHp;

    // colour shifts per phase
    float cr=0.9f, cg=0.1f, cb=0.3f;
    if (boss.phase==BOSS_PHASE2) { cr=1.0f; cg=0.4f; cb=0.0f; }
    if (boss.phase==BOSS_PHASE3) { cr=1.0f; cg=0.0f; cb=1.0f; }

    // aura halo (larger when enraged)
    int auraBase = boss.enraged ? 70 : 55;
    for (int r=auraBase; r>=auraBase-15; r--) {
        glColor4f(cr,cg,cb,0.05f*((r-(auraBase-15))/15.0f)*pulse); mpc(bx,by,r);
    }

    // outer hexagon body
    glColor3f(cr*(0.4f+flash*0.3f), cg*0.5f, cb*(0.4f+flash*0.3f));
    glBegin(GL_POLYGON);
    for (int i=0; i<6; i++) {
        float ang=i*60*PI/180+boss.ang*PI/180;
        glVertex2f(bx+48*cosf(ang), by+48*sinf(ang));
    }
    glEnd();

    // inner counter-rotating hexagon
    glColor3f(cr*0.7f, cg*0.3f, cb*0.7f);
    glBegin(GL_POLYGON);
    for (int i=0; i<6; i++) {
        float ang=i*60*PI/180-boss.ang*PI/180*0.5f;
        glVertex2f(bx+30*cosf(ang), by+30*sinf(ang));
    }
    glEnd();

    // six radial spines
    glColor4f(cr,cg,cb,0.6f+0.3f*pulse);
    for (int i=0; i<6; i++) {
        float ang=i*60*PI/180+boss.ang*PI/180;
        dda(bx+(int)(30*cosf(ang)), by+(int)(30*sinf(ang)),
            bx+(int)(60*cosf(ang)), by+(int)(60*sinf(ang)));
    }

    // bright core orb
    glColor3f(cr, cg+0.3f*pulse, cb);
    mpcFill(bx,by,14);
    glColor4f(1.0f,1.0f,1.0f,0.7f+0.3f*pulse);
    mpcFill(bx,by,7);
    glColor4f(cr, cg+0.5f, cb, 0.5f*pulse);
    mpc(bx,by,18); mpc(bx,by,19);

    // orbital shield orbs
    for (int i=0; i<4; i++) {
        if (!boss.orbOn[i]) continue;
        int ox=bx+(int)(38*cosf(boss.orbAng[i])), oy=by+(int)(38*sinf(boss.orbAng[i]));
        glColor4f(cr, cg+0.3f, cb, 0.8f); mpcFill(ox,oy,5);
        glColor4f(cr, cg+0.6f, cb, 0.4f*pulse);
        mpc(ox,oy,8); mpc(ox,oy,9);
    }

    // HP bar
    const int bw=200, bh=8, barX=bx-bw/2, barY=by+72;
    glColor4f(0.15f,0.0f,0.0f,0.85f);
    glBegin(GL_QUADS);
    glVertex2f(barX,barY); glVertex2f(barX+bw,barY);
    glVertex2f(barX+bw,barY+bh); glVertex2f(barX,barY+bh);
    glEnd();
    glColor3f(cr,cg,cb);
    glBegin(GL_QUADS);
    glVertex2f(barX,barY); glVertex2f(barX+bw*hfrac,barY);
    glVertex2f(barX+bw*hfrac,barY+bh); glVertex2f(barX,barY+bh);
    glEnd();
    glColor4f(cr+0.2f,cg+0.2f,cb+0.2f,0.7f);
    bres(barX,barY,barX+bw,barY); bres(barX,barY+bh,barX+bw,barY+bh);

    // phase label below HP bar
    glColor3f(cr, cg+0.5f, cb);
    txtC(bx, barY+12, "BOSS", GLUT_BITMAP_HELVETICA_12);
    string phaseStr = "PHASE ";
    if      (boss.phase==BOSS_PHASE1) phaseStr+="I";
    else if (boss.phase==BOSS_PHASE2) phaseStr+="II";
    else                              phaseStr+="III !!";
    txtC(bx, barY+24, phaseStr, GLUT_BITMAP_HELVETICA_12);

    // Phase III outer burst ring
    if (boss.phase==BOSS_PHASE3) {
        glColor4f(1.0f,0.0f,1.0f,0.4f+0.3f*pulse);
        for (int i=0; i<8; i++) {
            float ang=i*PI/4+boss.ang*PI/90;
            int ox=bx+(int)(70*cosf(ang)), oy=by+(int)(70*sinf(ang));
            mpc(ox,oy,4);
            int ox2=bx+(int)(70*cosf(ang+PI/4)), oy2=by+(int)(70*sinf(ang+PI/4));
            dda(ox,oy,ox2,oy2);
        }
    }
}

// Explosions 
// Each explosion has a scaling ring flash plus free-flying physics particles.
void drawExps() {
    for (auto &ex : exps) {
        if (!ex.on) continue;
        glPushMatrix();
        glTranslatef(ex.x, ex.y, 0);
        glScalef(ex.scale, ex.scale, 1);
        glColor4f(ex.r, ex.g, ex.b, ex.alpha*0.35f);    mpc(0,0,32);
        glColor4f(ex.r, ex.g+0.3f, ex.b, ex.alpha*0.55f); mpc(0,0,20);
        glColor4f(1.0f, 0.9f, 0.5f, ex.alpha*0.75f); mpcFill(0,0,11);
        glColor4f(1,1,1,ex.alpha);                    mpcFill(0,0,5);
        glPopMatrix();
        for (auto &p : ex.pts) {
            if (!p.on) continue;
            glColor4f(p.r, p.g, p.b, p.a * p.life/p.maxLife);
            glPointSize(p.sz);
            glBegin(GL_POINTS); glVertex2f(p.x, p.y); glEnd();
        }
    }
    glPointSize(1);
}

// Hit Rings

// Spawn a brief expanding ring at the hit location
void spawnHitRing(float x, float y, float cr=1.0f, float cg=0.8f, float cb=0.2f) {
    hitRings[hitRingIdx%16] = {x, y, 4, 1.0f, true, cr, cg, cb};
    hitRingIdx++;
}

// Update and draw all active hit rings each frame
void updateDrawHitRings() {
    for (auto &h : hitRings) {
        if (!h.on) continue;
        h.r    += 3.5f;
        h.alpha -= 0.06f;
        if (h.alpha <= 0) { h.on=false; continue; }
        glColor4f(h.cr, h.cg, h.cb, h.alpha);
        mpc((int)h.x,(int)h.y,(int)h.r);
        mpc((int)h.x,(int)h.y,(int)h.r+1);
    }
}

// Mini-Map Radar 
// Plots player, enemies, asteroids, and the boss on a small corner map.
// Includes an animated rotating sweep line.
void drawRadar() {
    if (!showRadar) return;
    const int rx=W-70, ry=60, rw=55, rh=55;

    // background panel
    glColor4f(0.0f,0.05f,0.12f,0.75f);
    glBegin(GL_QUADS);
    glVertex2f(rx-rw/2,ry-rh/2); glVertex2f(rx+rw/2,ry-rh/2);
    glVertex2f(rx+rw/2,ry+rh/2); glVertex2f(rx-rw/2,ry+rh/2);
    glEnd();
    // border
    glColor4f(0.2f,0.5f,0.9f,0.5f);
    bres(rx-rw/2,ry-rh/2,rx+rw/2,ry-rh/2);
    bres(rx-rw/2,ry+rh/2,rx+rw/2,ry+rh/2);
    bres(rx-rw/2,ry-rh/2,rx-rw/2,ry+rh/2);
    bres(rx+rw/2,ry-rh/2,rx+rw/2,ry+rh/2);

    glColor4f(0.3f,0.6f,1.0f,0.7f);
    txt(rx-rw/2+2, ry+rh/2-10, "RADAR", GLUT_BITMAP_HELVETICA_12);

    // player blip
    glColor3f(0.3f,0.9f,1.0f);
    mpcFill((int)(rx-rw/2+2+(sx/W)*(rw-4)), (int)(ry-rh/2+2+(sy/H)*(rh-4)), 2);

    // enemy blips
    for (auto &e : ene) {
        if (!e.on) continue;
        glColor3f(1.0f,0.2f,0.3f);
        mpcFill((int)(rx-rw/2+2+(e.x/W)*(rw-4)), (int)(ry-rh/2+2+(e.y/H)*(rh-4)), 1);
    }
    // boss blip with pulse
    if (boss.on) {
        float pulse=0.5f+0.5f*sinf(fc*0.12f);
        glColor4f(1.0f,0.3f,1.0f,0.7f+0.3f*pulse);
        mpcFill((int)(rx-rw/2+2+(boss.x/W)*(rw-4)), (int)(ry-rh/2+2+(boss.y/H)*(rh-4)), 3);
    }
    // asteroid blips
    for (auto &a : ast) {
        if (!a.on) continue;
        glColor3f(0.7f,0.5f,0.3f);
        glBegin(GL_POINTS);
        glVertex2f(rx-rw/2+2+(a.x/W)*(rw-4), ry-rh/2+2+(a.y/H)*(rh-4));
        glEnd();
    }
    // rotating sweep line
    glColor4f(0.3f,1.0f,0.5f,0.35f);
    dda(rx, ry,
        rx+(int)((rw/2-2)*cosf(fc*0.03f)),
        ry+(int)((rh/2-2)*sinf(fc*0.03f)));
}

// Combo Display 
// Shows the multiplier above the player; colour escalates with streak size.
void drawCombo() {
    if (comboCount < 2) return;
    float t   = min(1.0f, (float)comboTimer/60.0f);
    float pop = 1.0f + comboPop*0.4f;

    // background strip
    glColor4f(0.8f,0.5f,0.0f,0.3f*t);
    glBegin(GL_QUADS);
    glVertex2f(W/2-65, H/2+90); glVertex2f(W/2+65, H/2+90);
    glVertex2f(W/2+65, H/2+115); glVertex2f(W/2-65, H/2+115);
    glEnd();

    string comboStr = to_string(comboCount)+"x COMBO!";
    float sr=1.0f, sg=0.8f, sb=0.1f;
    if (comboCount>=5)  { sg=0.3f; sb=1.0f; }
    if (comboCount>=10) { sg=0.1f; sb=0.1f; }

    glPushMatrix();
    glTranslatef(W/2, H/2+105, 0);
    glScalef(pop, pop, 1);
    glTranslatef(-W/2, -H/2-105, 0);
    txtWave(W/2, H/2+105, comboStr, sr, sg, sb*t, 3.0f, fc*0.15f, GLUT_BITMAP_HELVETICA_18);
    glPopMatrix();
}

//  HUD 
// Top bar: score, wave number, zone name, and lives.
// Bottom bar: shield cooldown, active power-up timers, key hints.
void drawHUD() {
    // top bar background
    glColor4f(0.0f,0.04f,0.12f,0.88f);
    glBegin(GL_QUADS);
    glVertex2f(0,H); glVertex2f(W,H); glVertex2f(W,H-52); glVertex2f(0,H-52);
    glEnd();
    glColor4f(0.1f,0.5f,1.0f,0.7f); bres(0,H-52,W,H-52);
    glColor4f(0.2f,0.7f,1.0f,0.3f); bres(0,H-51,W,H-51);

    // score
    glColor3f(0.3f,0.6f,0.9f); txt(18,H-18,"SCORE",GLUT_BITMAP_HELVETICA_12);
    glColor3f(0.35f+scoreAnim*0.4f,1.0f,0.75f);
    txt(18,H-38,to_string(score),GLUT_BITMAP_TIMES_ROMAN_24);
    glColor4f(0.5f,0.8f,0.4f,0.7f);
    txt(18,H-50,"HI:"+to_string(highScore),GLUT_BITMAP_HELVETICA_12);

    glColor4f(0.2f,0.45f,0.8f,0.4f); bres(200,H-8,200,H-48);

    // wave / zone panel (centre)
    float wf = min(1.0f, waveFlash);
    glColor4f(0.05f+wf*0.3f,0.15f+wf*0.2f,0.35f+wf*0.2f,0.5f+wf*0.3f);
    glBegin(GL_QUADS);
    glVertex2f(W/2-80,H-8); glVertex2f(W/2+80,H-8);
    glVertex2f(W/2+80,H-52); glVertex2f(W/2-80,H-52);
    glEnd();
    glColor4f(0.2f+wf*0.5f,0.6f+wf*0.3f,1.0f,0.6f+wf*0.35f);
    bres(W/2-80,H-8,W/2+80,H-8); bres(W/2-80,H-52,W/2+80,H-52);
    bres(W/2-80,H-8,W/2-80,H-52); bres(W/2+80,H-8,W/2+80,H-52);

    glColor3f(0.5f,0.75f,1.0f); txtC(W/2,H-20,"WAVE",GLUT_BITMAP_HELVETICA_12);
    glColor3f(0.7f+wf*0.3f,0.9f,1.0f);
    txtC(W/2,H-38,to_string(wave),GLUT_BITMAP_TIMES_ROMAN_24);
    string zoneNames[]={"DEEP SPACE","ASTEROID BELT","NEBULA ZONE","BOSS ARENA"};
    glColor4f(0.6f,0.8f,1.0f,0.5f);
    txtC(W/2,H-50,zoneNames[currentZone],GLUT_BITMAP_HELVETICA_12);

    // lives icons
    glColor4f(0.2f,0.45f,0.8f,0.4f); bres(W-220,H-8,W-220,H-48);
    glColor3f(0.4f,0.65f,1.0f); txt(W-215,H-18,"LIVES",GLUT_BITMAP_HELVETICA_12);
    for (int i = 0; i < 3; i++) {
        float hx=W-210.0f+i*36, hy=H-40;
        if (i < lives) {
            glColor3f(0.0f,0.7f,1.0f);
            glBegin(GL_TRIANGLES);
            glVertex2f(hx,hy+12); glVertex2f(hx-10,hy-6); glVertex2f(hx+10,hy-6);
            glEnd();
            glColor4f(0.0f,0.8f,1.0f,0.3f);
            for (int r=12; r<=15; r++) mpc((int)hx,(int)(hy+2),r);
        } else {
            glColor4f(0.2f,0.3f,0.5f,0.5f);
            glBegin(GL_TRIANGLES);
            glVertex2f(hx,hy+12); glVertex2f(hx-10,hy-6); glVertex2f(hx+10,hy-6);
            glEnd();
        }
    }

    // bottom status bar
    glColor4f(0.0f,0.03f,0.10f,0.75f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(W,0); glVertex2f(W,28); glVertex2f(0,28);
    glEnd();
    glColor4f(0.1f,0.4f,0.9f,0.5f); bres(0,28,W,28);

    // shield cooldown bar
    if (shieldCool > 0) {
        float prog = 1.0f - shieldCool/300.0f;
        glColor4f(0.1f,0.15f,0.35f,0.8f);
        glBegin(GL_QUADS);
        glVertex2f(20,4); glVertex2f(160,4); glVertex2f(160,20); glVertex2f(20,20);
        glEnd();
        glColor3f(0.25f,0.4f,0.8f);
        glBegin(GL_QUADS);
        glVertex2f(20,4); glVertex2f(20+140*prog,4);
        glVertex2f(20+140*prog,20); glVertex2f(20,20);
        glEnd();
        glColor4f(0.4f,0.5f,1.0f,0.7f); txt(22,8,"SHIELD",GLUT_BITMAP_HELVETICA_12);
    } else {
        float shp = 0.6f+0.4f*sinf(fc*0.07f);
        glColor3f(0.2f,0.85f*shp,0.9f*shp);
        txt(10,8,"[S] SHIELD READY",GLUT_BITMAP_HELVETICA_12);
    }

    // active power-up icon bars
    int iconX = 180;
    auto drawPUIcon = [&](int cd, float r, float g, float b, const string &lbl) {
        if (cd <= 0) return;
        float prog = min(1.0f, (float)cd/180.0f);
        glColor4f(r,g,b,0.3f);
        glBegin(GL_QUADS);
        glVertex2f(iconX,3); glVertex2f(iconX+30,3);
        glVertex2f(iconX+30,22); glVertex2f(iconX,22);
        glEnd();
        glColor4f(r,g,b,0.6f);
        glBegin(GL_QUADS);
        glVertex2f(iconX,3); glVertex2f(iconX+30*prog,3);
        glVertex2f(iconX+30*prog,22); glVertex2f(iconX,22);
        glEnd();
        glColor3f(r,g,b); txtC(iconX+15,7,lbl,GLUT_BITMAP_HELVETICA_12);
        iconX+=35;
    };
    drawPUIcon(puRapidFire,1.0f,0.8f,0.1f,"RF");
    drawPUIcon(puSlowMo,   0.5f,0.2f,1.0f,"SM");
    drawPUIcon(puLaser,    1.0f,0.3f,0.9f,"LZ");
    drawPUIcon(puHoming,   0.2f,1.0f,0.5f,"HM");

    // key hint
    glColor4f(0.3f,0.45f,0.65f,0.6f);
    txt(W-280,8,"ENTER:FIRE  S:SHIELD  P:PAUSE",GLUT_BITMAP_HELVETICA_12);

    // corner bracket decorations
    glColor4f(0.3f,0.6f,1.0f,0.5f);
    bres(0,H,18,H);  bres(0,H,0,H-18);
    bres(W,H,W-18,H); bres(W,H,W,H-18);
    bres(0,28,18,28);  bres(0,28,0,46);
    bres(W,28,W-18,28); bres(W,28,W,46);
}

//  Slow-Motion Overlay 
// Nested vignette rectangles and a text label when gameSpeed is reduced.
void drawSlowMoOverlay() {
    if (gameSpeed >= 0.95f) return;
    float intensity = (1.0f - gameSpeed) * 0.6f;
    for (int i = 0; i < 6; i++) {
        glColor4f(0.2f,0.0f,0.4f,intensity*0.15f*(1-i/5.0f));
        int m = i*15;
        glBegin(GL_LINE_LOOP);
        glVertex2f(m,m); glVertex2f(W-m,m);
        glVertex2f(W-m,H-m); glVertex2f(m,H-m);
        glEnd();
    }
    if ((fc/8)%2==0) {
        glColor4f(0.6f,0.3f,1.0f,0.7f);
        txtC(W/2, H/2+150, "— SLOW MOTION —", GLUT_BITMAP_HELVETICA_12);
    }
}

//  Splash Screen ─
// Timed fade-in sequence: starfield → ship → title → subtitle → prompt.
void drawSplash() {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0,0,0.02f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(W,0); glVertex2f(W,H); glVertex2f(0,H);
    glEnd();

    float t = splashT / 180.0f;

    if (t > 0.15f) {
        float a = (t-0.15f)/0.3f;
        for (auto &s : stars) {
            float tw = s.bright * min(1.0f,a);
            glColor3f(tw,tw,tw*1.1f);
            glPointSize(s.sz);
            glBegin(GL_POINTS); glVertex2f(s.x,s.y); glEnd();
        }
        glPointSize(1);
    }
    if (t > 0.3f) {
        float a = clamp((t-0.3f)/0.25f,0,1);
        float glow = a*(0.5f+0.5f*sinf(splashT*0.1f));
        for (int r=80; r>=40; r-=4) {
            glColor4f(0.2f,0.5f,1.0f,glow*0.04f*(1-(r-40)/40.0f)); mpc(W/2,(int)(H*0.6f),r);
        }
        drawShip(W/2, (int)(H*0.58f), 0, 1.5f*a);
        txtGlow(W/2,(int)(H*0.75f),"PARTH & SPACE",0.4f,0.8f,1.0f,glow);
    }
    if (t > 0.55f) {
        float a = clamp((t-0.55f)/0.2f,0,1);
        glColor4f(0.6f,0.75f,0.9f,a);
        txtC(W/2,(int)(H*0.68f)-(1-a)*25,"ULTIMATE EDITION",GLUT_BITMAP_HELVETICA_18);
    }
    if (t > 0.7f) {
        glColor4f(0.35f,0.45f,0.65f,clamp((t-0.7f)/0.15f,0,1)*0.7f);
        txtC(W/2,(int)(H*0.62f),"by Shuvo Singh Partho",GLUT_BITMAP_HELVETICA_12);
    }
    if (t > 0.85f) {
        glColor4f(0.5f,0.8f,1.0f,0.5f+0.5f*sinf(splashT*0.15f));
        txtC(W/2,(int)(H*0.2f),"Press any key to continue...",GLUT_BITMAP_HELVETICA_18);
    }
    // fade to black at end of splash
    if (t > 0.92f) {
        glColor4f(0,0,0,clamp((t-0.92f)/0.08f,0,1));
        glBegin(GL_QUADS);
        glVertex2f(0,0); glVertex2f(W,0); glVertex2f(W,H); glVertex2f(0,H);
        glEnd();
    }
}

// Forward declarations for the three decorative feature functions
void drawSun();
void drawMoon();
void drawCompactPlanet();

//  Main Menu
void drawMenu() {
    drawBackground(); drawNebula(); drawStars(); drawRings(); drawPlanet();
    drawShip(W/2,170,0,1.3f);

    float gp = 0.5f+0.5f*sinf(menuAnim*0.05f);

    // title panel with animated border
    glColor4f(0.02f,0.06f,0.18f,0.82f);
    glBegin(GL_QUADS);
    glVertex2f(140,470); glVertex2f(760,470); glVertex2f(760,535); glVertex2f(140,535);
    glEnd();
    glColor4f(0.15f+0.15f*gp,0.55f+0.15f*gp,1.0f,0.65f+0.2f*gp);
    bres(140,470,760,470); bres(140,535,760,535);
    bres(140,470,140,535); bres(760,470,760,535);

    // corner accents
    glColor4f(0.5f,0.85f,1.0f,0.8f);
    bres(140,535,165,535); bres(140,535,140,510);
    bres(760,535,735,535); bres(760,535,760,510);
    bres(140,470,165,470); bres(140,470,140,497);
    bres(760,470,735,470); bres(760,470,760,497);

    txtWave(W/2,518,"PARTH & SPACE",0.35f+0.2f*gp,0.8f+0.15f*gp,1.0f,3.0f,menuAnim*0.06f,GLUT_BITMAP_TIMES_ROMAN_24);
    glColor3f(0.5f,0.7f,0.9f);
    txtC(W/2,478,"ULTIMATE EDITION  —  2D SPACE SHOOTER",GLUT_BITMAP_HELVETICA_12);
    glColor4f(0.8f,0.7f,0.2f,0.8f);
    txtC(W/2,462,"HIGH SCORE: "+to_string(highScore),GLUT_BITMAP_HELVETICA_18);

    // launch button
    float ep=0.5f+0.5f*sinf(menuAnim*0.09f);
    glColor4f(0.05f,0.2f+0.1f*ep,0.5f+0.15f*ep,0.7f+0.15f*ep);
    glBegin(GL_QUADS);
    glVertex2f(310,415); glVertex2f(590,415); glVertex2f(590,445); glVertex2f(310,445);
    glEnd();
    glColor4f(0.25f,0.7f+0.2f*ep,1.0f,0.8f+0.15f*ep);
    bres(310,415,590,415); bres(310,445,590,445);
    bres(310,415,310,445); bres(590,415,590,445);
    glColor3f(0.5f,0.9f+0.1f*ep,1.0f);
    txtC(W/2,425,"PRESS  SPACE  TO LAUNCH",GLUT_BITMAP_HELVETICA_18);

    // settings shortcut button
    glColor4f(0.04f,0.12f,0.28f,0.75f);
    glBegin(GL_QUADS);
    glVertex2f(350,382); glVertex2f(550,382); glVertex2f(550,408); glVertex2f(350,408);
    glEnd();
    glColor4f(0.2f,0.45f,0.8f,0.6f);
    bres(350,382,550,382); bres(350,408,550,408);
    bres(350,382,350,408); bres(550,382,550,408);
    glColor3f(0.45f,0.7f,1.0f);
    txtC(W/2,389,"[ TAB ]  SETTINGS",GLUT_BITMAP_HELVETICA_12);

    // controls reference panel
    glColor4f(0.01f,0.04f,0.12f,0.75f);
    glBegin(GL_QUADS);
    glVertex2f(200,280); glVertex2f(700,280); glVertex2f(700,372); glVertex2f(200,372);
    glEnd();
    glColor4f(0.15f,0.35f,0.7f,0.45f);
    bres(200,280,700,280); bres(200,372,700,372);
    bres(200,280,200,372); bres(700,280,700,372);

    glColor3f(0.55f,0.75f,1.0f); txtC(W/2,360,"CONTROLS",GLUT_BITMAP_HELVETICA_12);
    glColor3f(0.6f,0.75f,0.9f);
    txtC(W/2,344,"WASD / Arrow Keys  —  Move",GLUT_BITMAP_HELVETICA_12);
    txtC(W/2,328,"ENTER  —  Fire    S  —  Shield    P  —  Pause    R  —  Restart",GLUT_BITMAP_HELVETICA_12);
    txtC(W/2,312,"Collect glowing power-ups for upgrades!",GLUT_BITMAP_HELVETICA_12);
    txtC(W/2,296,"Survive waves, defeat the boss every 5 waves",GLUT_BITMAP_HELVETICA_12);
}

//  Settings Screen 
// Keyboard-navigable list: difficulty, radar toggle, trail toggle, back.
void drawSettings() {
    drawBackground(); drawStars();

    float gp = 0.5f+0.5f*sinf(settingsAnim*0.06f);
    // panel
    glColor4f(0.02f,0.05f,0.15f,0.92f);
    glBegin(GL_QUADS);
    glVertex2f(230,180); glVertex2f(670,180); glVertex2f(670,540); glVertex2f(230,540);
    glEnd();
    glColor4f(0.2f,0.5f,1.0f,0.6f+0.2f*gp);
    bres(230,180,670,180); bres(230,540,670,540);
    bres(230,180,230,540); bres(670,180,670,540);

    glColor3f(0.5f,0.8f,1.0f);
    txtC(W/2,520,"SETTINGS",GLUT_BITMAP_TIMES_ROMAN_24);
    glColor4f(0.3f,0.5f,0.8f,0.5f); bres(260,505,640,505);

    struct Opt { string label, value; };
    vector<Opt> opts = {
        {"DIFFICULTY", difficulty==0?"EASY":difficulty==1?"NORMAL":"HARD"},
        {"RADAR",      showRadar  ? "ON":"OFF"},
        {"TRAILS",     showTrails ? "ON":"OFF"},
        {"[BACK]",     "TAB / ESC"}
    };

    for (int i = 0; i < (int)opts.size(); i++) {
        float oy = 480 - i*55.0f;
        bool sel = (i==settingsSel);
        if (sel) {
            float h = 0.5f+0.5f*sinf(settingsAnim*0.12f);
            glColor4f(0.1f,0.3f+0.2f*h,0.6f+0.1f*h,0.4f+0.2f*h);
            glBegin(GL_QUADS);
            glVertex2f(245,oy-5); glVertex2f(655,oy-5);
            glVertex2f(655,oy+22); glVertex2f(245,oy+22);
            glEnd();
            glColor4f(0.3f,0.7f+0.2f*h,1.0f,0.7f);
            bres(245,oy-5,655,oy-5); bres(245,oy+22,655,oy+22);
        }
        glColor3f(sel?1.0f:0.6f, sel?1.0f:0.75f, sel?1.0f:0.9f);
        txt(265,oy,opts[i].label,GLUT_BITMAP_HELVETICA_18);
        glColor3f(sel?0.4f:0.3f, sel?1.0f:0.7f, sel?0.6f:0.5f);
        txt(635-strWidth(opts[i].value,GLUT_BITMAP_HELVETICA_18),oy,opts[i].value,GLUT_BITMAP_HELVETICA_18);
        if (sel && i<3) {
            glColor4f(0.5f,0.9f,1.0f,0.7f);
            txt(637,oy,">",GLUT_BITMAP_HELVETICA_18);
            txt(251,oy,"<",GLUT_BITMAP_HELVETICA_18);
        }
    }
    glColor4f(0.3f,0.5f,0.7f,0.55f);
    txtC(W/2,200,"UP/DOWN: Select    LEFT/RIGHT: Change    TAB: Back",GLUT_BITMAP_HELVETICA_12);
}

// Pause Overlay 
void drawPauseOverlay() {
    // dark translucent cover
    glColor4f(0,0,0.04f,0.62f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(W,0); glVertex2f(W,H); glVertex2f(0,H);
    glEnd();

    // panel with corner brackets
    glColor4f(0.02f,0.06f,0.16f,0.92f);
    glBegin(GL_QUADS);
    glVertex2f(300,290); glVertex2f(600,290); glVertex2f(600,430); glVertex2f(300,430);
    glEnd();
    glColor4f(0.3f,0.65f,1.0f,0.7f);
    bres(300,290,600,290); bres(300,430,600,430);
    bres(300,290,300,430); bres(600,290,600,430);
    glColor3f(0.5f,0.8f,1.0f);
    bres(300,430,318,430); bres(300,430,300,412);
    bres(600,430,582,430); bres(600,430,600,412);
    bres(300,290,318,290); bres(300,290,300,308);
    bres(600,290,582,290); bres(600,290,600,308);

    glColor3f(0.85f,0.9f,1.0f);
    txtC(W/2,408,"PAUSED",GLUT_BITMAP_TIMES_ROMAN_24);
    glColor3f(0.5f,0.75f,1.0f);
    txtC(W/2,372,"[ P ]  Resume Game",GLUT_BITMAP_HELVETICA_18);
    txtC(W/2,345,"[ TAB ]  Settings",GLUT_BITMAP_HELVETICA_12);
    txtC(W/2,325,"[ R ]  Restart    [ ESC ]  Quit",GLUT_BITMAP_HELVETICA_12);
    glColor3f(0.35f,0.65f,0.9f);
    txtC(W/2,305,"Score: "+to_string(score)+"   Wave: "+to_string(wave),GLUT_BITMAP_HELVETICA_12);
}

// Game Over Screen 
void drawOver() {
    // dark overlay
    glColor4f(0,0,0,0.75f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(W,0); glVertex2f(W,H); glVertex2f(0,H);
    glEnd();

    float gp = 0.5f+0.5f*sinf(menuAnim*0.07f);

    // result panel
    glColor4f(0.06f,0.02f,0.02f,0.96f);
    glBegin(GL_QUADS);
    glVertex2f(250,220); glVertex2f(650,220); glVertex2f(650,500); glVertex2f(250,500);
    glEnd();
    glColor4f(0.85f,0.15f,0.2f,0.6f+0.3f*gp);
    bres(250,220,650,220); bres(250,500,650,500);
    bres(250,220,250,500); bres(650,220,650,500);

    txtWave(W/2,470,"GAME  OVER",1.0f,0.15f,0.2f,4.0f,menuAnim*0.07f,GLUT_BITMAP_TIMES_ROMAN_24);
    glColor4f(0.6f,0.15f,0.2f,0.4f); bres(280,448,620,448);

    // score display
    glColor3f(0.7f,0.65f,0.25f);
    txtC(W/2,420,"SCORE",GLUT_BITMAP_HELVETICA_12);
    glColor3f(1.0f,0.88f,0.3f);
    txtC(W/2,396,to_string(score),GLUT_BITMAP_TIMES_ROMAN_24);

    // high score line
    if (score >= highScore) {
        glColor4f(1.0f,0.85f,0.1f,0.55f+0.4f*sinf(menuAnim*0.13f));
        txtC(W/2,370,"NEW  BEST!",GLUT_BITMAP_HELVETICA_18);
    } else {
        glColor3f(0.38f,0.5f,0.75f);
        txtC(W/2,370,"BEST: "+to_string(highScore),GLUT_BITMAP_HELVETICA_18);
    }
    glColor3f(0.5f,0.65f,0.88f);
    txtC(W/2,342,"WAVE  "+to_string(wave),GLUT_BITMAP_HELVETICA_18);
    glColor4f(0.35f,0.28f,0.55f,0.3f); bres(280,324,620,324);

    float ep = 0.5f+0.5f*sinf(menuAnim*0.10f);

    // restart button
    glColor4f(0.04f,0.16f+0.06f*ep,0.06f,0.82f);
    glBegin(GL_QUADS);
    glVertex2f(268,268); glVertex2f(438,268); glVertex2f(438,308); glVertex2f(268,308);
    glEnd();
    glColor4f(0.15f,0.72f+0.2f*ep,0.32f,0.85f);
    bres(268,268,438,268); bres(268,308,438,308);
    bres(268,268,268,308); bres(438,268,438,308);
    glColor3f(0.25f,0.95f,0.5f);
    txtC(353,281,"[ R ]  RESTART",GLUT_BITMAP_HELVETICA_18);

    // quit button
    glColor4f(0.10f,0.04f,0.04f,0.82f);
    glBegin(GL_QUADS);
    glVertex2f(458,268); glVertex2f(628,268); glVertex2f(628,308); glVertex2f(458,308);
    glEnd();
    glColor4f(0.75f,0.18f,0.22f,0.8f);
    bres(458,268,628,268); bres(458,308,628,308);
    bres(458,268,458,308); bres(628,268,628,308);
    glColor3f(0.95f,0.4f,0.45f);
    txtC(543,281,"[ ESC ]  QUIT",GLUT_BITMAP_HELVETICA_18);
}

//  Wave / Boss Banner 
// Centred banner that fades in then out whenever a new wave or boss starts.
void drawWaveBanner() {
    if (waveFlash <= 0) return;
    float a = (waveFlash<30) ? waveFlash/30.0f : min(1.0f, waveFlash/30.0f);
    bool isBoss = (currentZone == ZONE_BOSS);

    glColor4f(isBoss?0.8f:0.02f, isBoss?0.0f:0.08f, isBoss?0.05f:0.22f, 0.85f*a);
    glBegin(GL_QUADS);
    glVertex2f(200,H/2-32); glVertex2f(700,H/2-32);
    glVertex2f(700,H/2+32); glVertex2f(200,H/2+32);
    glEnd();
    glColor4f(isBoss?1.0f:0.3f, isBoss?0.2f:0.7f, isBoss?0.3f:1.0f, 0.8f*a);
    bres(200,H/2-32,700,H/2-32); bres(200,H/2+32,700,H/2+32);

    string msg = isBoss ? "★  BOSS INCOMING!  ★" : "WAVE "+to_string(wave)+" INCOMING!";
    glColor4f(isBoss?1.0f:0.6f, isBoss?0.3f:0.9f, isBoss?0.3f:1.0f, a);
    txtC(W/2, H/2+8, msg, GLUT_BITMAP_TIMES_ROMAN_24);
    if (isBoss) {
        glColor4f(1.0f,0.6f,0.1f,a*0.8f);
        txtC(W/2,H/2-15,"Survive the onslaught!",GLUT_BITMAP_HELVETICA_12);
    }
}

// ── Screen Transition Fade 
void drawTransitionFade() {
    if (transAlpha <= 0) return;
    glColor4f(0,0,0,transAlpha);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(W,0); glVertex2f(W,H); glVertex2f(0,H);
    glEnd();
}

void drawSun() {
    if (currentZone == ZONE_ASTEROID || currentZone == ZONE_BOSS) return;

    const int SX=80, SY=610, SR=30;
    float pulse  = 0.5f + 0.5f*sinf(sunT*0.08f);
    float pulse2 = 0.5f + 0.5f*sinf(sunT*0.13f+1.0f);

    // outer glow halo
    for (int r=SR+38; r>=SR+12; r-=3) {
        float t=(r-SR-12)/26.0f;
        glColor4f(1.0f, 0.75f+0.15f*pulse, 0.1f, 0.03f*(1.0f-t)*(0.6f+0.4f*pulse));
        mpc(SX,SY,r);
    }
    // mid corona ring
    for (int r=SR+11; r>=SR+2; r--) {
        float t=(r-SR-2)/9.0f;
        glColor4f(1.0f, 0.65f+0.25f*(1.0f-t), 0.05f, 0.18f*(1.0f-t)*(0.75f+0.25f*pulse));
        mpc(SX,SY,r);
    }
    // eight rotating light rays (DDA lines)
    int numRays=8;
    float rayLen=22.0f+10.0f*pulse2;
    float rotOff=sunT*0.012f;
    for (int i=0; i<numRays; i++) {
        float ang=i*(2.0f*PI/numRays)+rotOff;
        float ang2=ang+(PI/numRays);
        // long primary ray
        int x1=SX+(int)((SR+4)*cosf(ang)),   y1=SY+(int)((SR+4)*sinf(ang));
        int x2=SX+(int)((SR+4+(int)rayLen)*cosf(ang)), y2=SY+(int)((SR+4+(int)rayLen)*sinf(ang));
        glColor4f(1.0f,0.9f,0.3f,0.55f+0.35f*pulse); dda(x1,y1,x2,y2);
        // short interleaved secondary ray
        int x3=SX+(int)((SR+4)*cosf(ang2)),   y3=SY+(int)((SR+4)*sinf(ang2));
        int x4=SX+(int)((SR+4+(int)(rayLen*0.55f))*cosf(ang2));
        int y4=SY+(int)((SR+4+(int)(rayLen*0.55f))*sinf(ang2));
        glColor4f(1.0f,0.85f,0.2f,0.30f+0.20f*pulse2); dda(x3,y3,x4,y4);
    }
    // solid bright core
    glColor3f(1.0f, 0.92f+0.08f*pulse, 0.35f+0.15f*pulse2); mpcFill(SX,SY,SR);
    // hot white centre
    glColor4f(1.0f,1.0f,0.85f,0.90f+0.10f*pulse); mpcFill(SX,SY,SR/2);
    // bright rim
    glColor4f(1.0f,0.75f,0.1f,0.75f+0.20f*pulse); mpc(SX,SY,SR); mpc(SX,SY,SR+1);
}

void drawMoon() {
    if (currentZone == ZONE_ASTEROID || currentZone == ZONE_BOSS) return;

    const float OX=780.0f, OY=600.0f, ORBIT=55.0f;
    const int MR=18;
    float ang=moonT*0.018f;
    int mx=(int)(OX+ORBIT*cosf(ang)), my=(int)(OY+ORBIT*sinf(ang));
    float pulse=0.5f+0.5f*sinf(moonT*0.12f);

    // soft phase glow halo
    for (int r=MR+16; r>=MR+4; r-=2) {
        float t=(r-MR-4)/12.0f;
        glColor4f(0.75f,0.80f,1.0f,0.025f*(1.0f-t)*(0.5f+0.5f*pulse)); mpc(mx,my,r);
    }
    // body gradient fill
    for (int r=MR; r>0; r-=2) {
        float t=1.0f-r/(float)MR;
        glColor3f(0.42f+0.20f*t,0.42f+0.18f*t,0.48f+0.15f*t); mpc(mx,my,r);
    }
    glColor3f(0.50f,0.50f,0.56f); mpcFill(mx,my,MR-1);

    // light-side highlight band (DDA horizontal strips)
    for (int off=-4; off<=4; off++) {
        float bright=1.0f-fabsf(off)/5.0f;
        glColor4f(0.82f,0.84f,0.90f,0.18f*bright);
        dda(mx-MR+3,my+off,mx-2,my+off);
    }

    // crater 1 — large, lower-right
    glColor4f(0.28f,0.28f,0.32f,0.70f+0.15f*pulse); mpc(mx+6,my-5,5);
    glColor4f(0.62f,0.63f,0.68f,0.40f);              mpc(mx+6,my-5,6);
    // crater 2 — medium, upper area
    glColor4f(0.30f,0.30f,0.34f,0.65f+0.10f*pulse); mpc(mx-4,my+7,3);
    glColor4f(0.60f,0.62f,0.66f,0.35f);              mpc(mx-4,my+7,4);
    // crater 3 — small, centre-left
    glColor4f(0.32f,0.30f,0.35f,0.55f); mpc(mx-6,my-2,2);

    // outer rim
    glColor4f(0.78f,0.80f,0.88f,0.55f+0.25f*pulse); mpc(mx,my,MR);
}

void drawCompactPlanet() {
    if (currentZone == ZONE_ASTEROID || currentZone == ZONE_BOSS) return;

    const int CPX=820;
    float bob=sinf(compactPlanetT*0.04f)*6.0f;
    int CPY=200+(int)bob;
    const int CPR=26;
    float pulse  = 0.5f+0.5f*sinf(compactPlanetT*0.09f);
    float pulse2 = 0.5f+0.5f*sinf(compactPlanetT*0.06f+1.5f);

    // atmosphere halo
    for (int r=CPR+12; r>=CPR+3; r-=2) {
        float t=(r-CPR-3)/9.0f;
        glColor4f(0.30f,0.55f+0.20f*pulse,0.95f,0.04f*(1.0f-t)*(0.5f+0.5f*pulse));
        mpc(CPX,CPY,r);
    }
    // planet body gradient
    for (int r=CPR; r>0; r-=2) {
        float t=1.0f-r/(float)CPR;
        glColor3f(0.08f+0.30f*t,0.12f+0.45f*t,0.45f+0.35f*t); mpc(CPX,CPY,r);
    }
    glColor3f(0.18f,0.42f,0.72f); mpcFill(CPX,CPY,CPR-1);
    // polar cap
    glColor4f(0.55f,0.82f,1.0f,0.30f+0.15f*pulse2); mpcFill(CPX,CPY+CPR-7,5);

    // ring band 1 (wide, main ring — Bresenham lines)
    float ringShimmer=0.45f+0.25f*sinf(compactPlanetT*0.07f);
    glColor4f(0.65f,0.75f+0.15f*pulse,1.0f,ringShimmer);
    for (int b=-3; b<=3; b++) {
        int halfW=(int)sqrtf(fmaxf(0.0f,(float)((CPR+14)*(CPR+14)-b*b*16)));
        bres(CPX-halfW,CPY+b*4,CPX+halfW,CPY+b*4);
    }
    // ring band 2 (thin, inner)
    glColor4f(0.80f,0.90f,1.0f,(ringShimmer-0.10f)*pulse2);
    for (int b=-1; b<=1; b++) {
        int halfW=(int)sqrtf(fmaxf(0.0f,(float)((CPR+6)*(CPR+6)-b*b*16)));
        bres(CPX-halfW,CPY+b*4,CPX+halfW,CPY+b*4);
    }

    // redraw planet body on top of rings so rings appear behind
    glColor3f(0.18f,0.42f,0.72f); mpcFill(CPX,CPY,CPR);
    for (int r=CPR; r>CPR-6; r--) {
        float t=1.0f-r/(float)CPR;
        glColor3f(0.08f+0.30f*t,0.12f+0.45f*t,0.45f+0.35f*t); mpc(CPX,CPY,r);
    }
    glColor4f(0.55f,0.82f,1.0f,0.30f+0.15f*pulse2); mpcFill(CPX,CPY+CPR-7,5);

    // bright rim
    glColor4f(0.50f,0.80f,1.0f,0.60f+0.25f*pulse); mpc(CPX,CPY,CPR);

    // elliptical orbiting companion moon
    float mAng=compactPlanetT*0.035f;
    int mmx=CPX+(int)((CPR+16)*cosf(mAng));
    int mmy=CPY+(int)((CPR+16)*0.45f*sinf(mAng));
    glColor3f(0.60f,0.58f,0.62f); mpcFill(mmx,mmy,5);
    glColor4f(0.80f,0.80f,0.90f,0.45f+0.20f*pulse); mpc(mmx,mmy,6);
}

//  Spawn Helpers 
// Create an explosion at (x,y) with optional intensity and RGB tint
void spawnExplosion(float x, float y, float intensity, float cr, float cg, float cb) {
    Explosion ex{x,y,0.1f,intensity,true,{},cr,cg,cb};
    int numPts=(int)(20+intensity*18);
    for (int i=0; i<numPts; i++) {
        float a=frand(0,2*PI), s=frand(1.5f,4.5f+intensity*2), mL=frand(0.5f,1.0f);
        ex.pts.push_back({x,y,s*cosf(a),s*sinf(a),mL,mL,
                          cr*frand(0.8f,1.0f), cg*frand(0.4f,0.9f), cb*frand(0,0.4f),
                          intensity, frand(1.5f,3.5f+intensity),
                          frand(-0.02f,-0.08f), frand(0.96f,0.99f), true});
    }
    exps.push_back(ex);
    triggerScreenShake(intensity*4, 12);
}

// Spawn a random power-up pickup at the given position
void spawnPowerUp(float x, float y) {
    for (auto &p : pups)
        if (!p.on) {
            p = {x, y, -0.8f, (PowerUpType)(rand()%6), true, 0, frand(0,2*PI)};
            break;
        }
}

// Select an AI type appropriate for the current wave number and difficulty
void spawnEnemy() {
    EnemyAI ai;
    int r = rand()%10;
    if      (wave<3) ai=AI_SINE;
    else if (wave<5) ai=(r<6)?AI_ZIGZAG:AI_TRACKER;
    else if (wave<8) ai=(r<4)?AI_SWARM:(r<7?AI_TRACKER:AI_KAMIKAZE);
    else             ai=(r<3)?AI_KAMIKAZE:(r<6?AI_SWARM:AI_TRACKER);

    float spdMult=(difficulty==0)?0.7f:(difficulty==2)?1.3f:1.0f;
    for (auto &e : ene)
        if (!e.on) {
            e.x=frand(60,W-60); e.y=frand(H+20,H+150); e.baseY=e.y;
            e.spd=(frand(0.6f,1.2f)+wave*0.12f)*spdMult;
            e.ang=0; e.flashTimer=0; e.on=true;
            e.maxHp=(wave>=6)?3:(wave>=3)?2:1; e.hp=e.maxHp;
            e.ai=ai; e.aiTimer=0; e.tx=sx; e.ty=sy;
            e.swarmIdx=(int)(&e-ene);
            break;
        }
}

// Spawn a new asteroid with a random size, spin, and speed
void spawnAst() {
    for (auto &a : ast)
        if (!a.on) {
            a={frand(40,W-40),(float)(H+30),frand(0.5f,1.5f),
               frand(0,360),frand(-2.5f,2.5f),frand(14,28),true,0};
            break;
        }
}

// Initialise the boss for the current wave with phase-scaled HP
void spawnBoss() {
    boss.x=W/2; boss.y=H+80; boss.targetX=W/2; boss.targetY=H-160;
    boss.vx=boss.vy=0;
    int diffMul=(difficulty==0)?1:(difficulty==2)?3:2;
    boss.maxHp=200+wave*20*diffMul; boss.hp=boss.maxHp;
    boss.on=true; boss.phase=BOSS_PHASE1;
    boss.phaseTimer=boss.shootCd=0;
    boss.ang=0; boss.angSpd=0.5f; boss.flashTimer=0;
    boss.entryProgress=0; boss.enraged=false;
    boss.attackPattern=0; boss.patternTimer=0;
    for (int i=0; i<4; i++) {
        boss.orbAng[i]=i*PI/2; boss.orbOn[i]=true; boss.orbHp[i]=3;
    }
    waveFlash=120; currentZone=ZONE_BOSS;
    triggerScreenShake(8,30);
}

// Start the next wave: boss every 5th wave, otherwise mix of enemies/asteroids
void spawnWave() {
    if (wave%5==0) { spawnBoss(); return; }
    currentZone=(wave%5==4)?ZONE_NEBULA:(wave%5==3)?ZONE_ASTEROID:ZONE_SPACE;

    int numEne=3+wave+(difficulty==2?2:0);
    if (difficulty==0) numEne=max(2,numEne-2);
    for (int i=0; i<numEne; i++) spawnEnemy();
    if (wave>=2) spawnAst();
    if (wave>=4) spawnAst();
    waveFlash=90;
}

// Fire the player's weapon (respects power-up modes and cooldown)
void firePlayer() {
    int cooldown=puRapidFire>0?5:12;
    if (bcool>0) return;

    if (puLaser>0) {
        for (auto &b : bul)
            if (!b.on) { b={sx,sy+38,true,true,false,0,0,0}; bcool=cooldown; break; }
        return;
    }
    if (puHoming>0) {
        // find the nearest enemy to lock onto
        float best=1e9, tx2=sx, ty2=sy+200;
        for (auto &e : ene)
            if (e.on) { float d=dist(sx,sy,e.x,e.y); if (d<best){best=d;tx2=e.x;ty2=e.y;} }
        if (boss.on) { float d=dist(sx,sy,boss.x,boss.y); if (d<best){tx2=boss.x;ty2=boss.y;} }
        for (auto &b : bul)
            if (!b.on) { b={sx,sy+38,true,false,true,0,tx2,ty2}; bcool=cooldown; break; }
        return;
    }
    // normal (and RapidFire double) shot
    int shots=1+(puRapidFire>0?1:0);
    float offsets[]={0,-8,8,-16,16};
    int fired=0;
    for (auto &b : bul)
        if (!b.on && fired<shots) {
            b={sx+offsets[fired]*shots,sy+38,true,false,false,0,0,0};
            fired++; bcool=cooldown;
        }
}

// Bomb power-up: instantly destroys all enemies and asteroids on screen
void activateBomb() {
    for (auto &e : ene) if (e.on) { spawnExplosion(e.x,e.y,1.5f); score+=50; e.on=false; }
    for (auto &a : ast) if (a.on) { spawnExplosion(a.x,a.y,1.2f,0.7f,0.5f,0.3f); score+=25; a.on=false; }
    if (boss.on) { boss.hp-=50; triggerScreenShake(10,25); spawnHitRing(boss.x,boss.y,1.0f,0.5f,0.1f); }
    scoreAnim=1.0f; triggerScreenShake(12,30);
}

// Spawn an enemy projectile with given velocity components
void shootEnemyBullet(float x, float y, float vx, float vy) {
    for (auto &b : ebul)
        if (!b.on) { b={x,y,vx,vy,true,0}; break; }
}

//  Game Initialisation 
// Resets all game state and object arrays, then spawns the first wave.
void initGame() {
    sx=W/2; sy=80; sang=0; sscale=0.1f;
    score=0; lastScore=0; lives=3; wave=1; fc=0; bcool=0;
    comboCount=comboTimer=0; comboPop=0;
    shieldOn=shieldCool=0; waveFlash=0; scoreAnim=0;
    puRapidFire=puSlowMo=puLaser=puHoming=0;
    gameSpeed=1.0f; slowMoTarget=1.0f;
    shakeX=shakeY=shakeMag=0; shakeFrames=0;
    mL=mR=mU=mD=false;
    currentZone=ZONE_SPACE; planetRot=0; blackHoleT=0;

    for (auto &b : bul)   b.on=false;
    for (auto &e : ene)   e.on=false;
    for (auto &a : ast)   a.on=false;
    for (auto &p : pups)  p.on=false;
    for (auto &b : ebul)  b.on=false;
    for (auto &h : hitRings) h.on=false;
    boss.on=false; exps.clear();
    playerTrail.clear();
    for (int i=0; i<MB; i++) bulletTrails[i].clear();

    spawnWave();
    state=PLAY; transAlpha=1.0f; transDir=-1;
}

// ── Boss AI Update 
// Handles entry slide-in, phase transitions, movement tracking,
// orbital orb spin, multi-pattern shooting, and collision checks.
void updateBoss() {
    if (!boss.on) return;

    // slide-in entry animation (interpolated Y approach)
    if (boss.entryProgress < 1.0f) {
        boss.entryProgress+=0.008f;
        boss.y=lerp(H+80, H-160, boss.entryProgress);
        return;
    }

    // phase transitions based on remaining HP fraction
    float hfrac=(float)boss.hp/boss.maxHp;
    if (hfrac<0.66f && boss.phase==BOSS_PHASE1) {
        boss.phase=BOSS_PHASE2; boss.angSpd=1.2f;
        triggerScreenShake(6,20); spawnExplosion(boss.x,boss.y,0.8f,1.0f,0.5f,0.0f);
    }
    if (hfrac<0.33f && boss.phase==BOSS_PHASE2) {
        boss.phase=BOSS_PHASE3; boss.angSpd=2.5f; boss.enraged=true;
        triggerScreenShake(10,30); spawnExplosion(boss.x,boss.y,1.5f,1.0f,0.0f,1.0f);
    }

    boss.ang+=boss.angSpd*gameSpeed;

    // smooth pursuit of the player (lerp-based velocity)
    float moveSpeed=(boss.phase==BOSS_PHASE1)?0.8f:(boss.phase==BOSS_PHASE2)?1.2f:1.8f;
    float dx=sx-boss.x, dy=sy+200-boss.y;
    boss.vx=lerp(boss.vx,dx*0.012f*moveSpeed,0.04f);
    boss.vy=lerp(boss.vy,dy*0.012f*moveSpeed,0.04f);
    boss.x=clamp(boss.x+boss.vx*gameSpeed,80.0f,(float)W-80);
    boss.y=clamp(boss.y+boss.vy*gameSpeed,(float)H-300,(float)H-120);

    if (boss.flashTimer>0) boss.flashTimer-=gameSpeed;

    // spin orbital shield orbs (faster in Phase III)
    float orbSpeedMult=(boss.phase==BOSS_PHASE3)?2.5f:1.0f;
    for (int i=0; i<4; i++) boss.orbAng[i]+=0.03f*orbSpeedMult*gameSpeed;

    boss.patternTimer+=gameSpeed;
    boss.shootCd-=gameSpeed;

    // fire pattern cycle
    if (boss.shootCd<=0) {
        float shootRate=(boss.phase==BOSS_PHASE1)?55:(boss.phase==BOSS_PHASE2)?35:18;
        boss.shootCd=shootRate;
        boss.attackPattern=(boss.attackPattern+1)%3;

        float dx2=sx-boss.x, dy2=sy-boss.y;
        float len=sqrtf(dx2*dx2+dy2*dy2)+0.001f;
        float nx=dx2/len, ny=dy2/len;
        float spd=3.5f+(boss.phase==BOSS_PHASE3?1.5f:0);

        if (boss.phase==BOSS_PHASE1) {
            // single aimed shot
            shootEnemyBullet(boss.x,boss.y,nx*spd,ny*spd);
        } else if (boss.phase==BOSS_PHASE2) {
            // triple spread shot
            for (int i=-1; i<=1; i++) {
                float a=atan2f(ny,nx)+i*0.25f;
                shootEnemyBullet(boss.x,boss.y,cosf(a)*spd,sinf(a)*spd);
            }
        } else {
            // eight-way radial burst + aimed shot
            for (int i=0; i<8; i++) {
                float a=i*PI/4;
                shootEnemyBullet(boss.x,boss.y,cosf(a)*spd,sinf(a)*spd);
            }
            shootEnemyBullet(boss.x,boss.y,nx*(spd+1),ny*(spd+1));
        }
    }

    // player body collision
    if (!shieldOn && dist(sx,sy,boss.x,boss.y)<60) {
        spawnExplosion((boss.x+sx)/2,(boss.y+sy)/2,1.2f);
        if (--lives<=0) { saveHighScore(); state=OVER; }
        triggerScreenShake(8,20);
    }

    // player bullet hits on orbs and main body
    for (int i=0; i<MB; i++) {
        auto &b=bul[i];
        if (!b.on) continue;
        bool hit=false;
        for (int j=0; j<4; j++) {
            if (!boss.orbOn[j]) continue;
            int ox=(int)(boss.x+38*cosf(boss.orbAng[j]));
            int oy=(int)(boss.y+38*sinf(boss.orbAng[j]));
            if (dist(b.x,b.y,ox,oy)<10) {
                b.on=false;
                spawnHitRing(ox,oy,1.0f,0.6f,0.2f);
                if (--boss.orbHp[j]<=0) boss.orbOn[j]=false;
                hit=true; break;
            }
        }
        if (hit) continue;
        if (dist(b.x,b.y,boss.x,boss.y)<50) {
            b.on=false;
            boss.hp-=b.isLaser?5:1;
            boss.flashTimer=8;
            spawnHitRing(boss.x,boss.y,1.0f,0.4f,0.1f);
            score+=5; scoreAnim=min(1.0f,scoreAnim+0.2f);
            comboCount++; comboTimer=120;
            if (boss.hp<=0) {
                spawnExplosion(boss.x,boss.y,3.0f,1.0f,0.3f,1.0f);
                for (int k=0; k<6; k++)
                    spawnExplosion(boss.x+frand(-60,60),boss.y+frand(-60,60),1.5f);
                score+=1000+wave*100; scoreAnim=1.0f;
                boss.on=false; triggerScreenShake(15,50);
                spawnPowerUp(boss.x,boss.y);
                wave++; currentZone=ZONE_SPACE; spawnWave();
            }
        }
    }
}

//  Main Game Update 
// Called every 16 ms; drives all game logic, physics, and AI.
void update() {
    menuAnim++; settingsAnim++;

    // non-play states only need minimal updates
    if (state==SPLASH) {
        splashT++;
        for (auto &s : stars) { s.y-=s.spd*0.3f; if (s.y<0){s.y=H;s.x=frand(0,W);} }
        if (splashT>=180) state=MENU;
        glutPostRedisplay(); return;
    }
    if (state==MENU)     { updateRings(); glutPostRedisplay(); return; }
    if (state==SETTINGS) { glutPostRedisplay(); return; }
    if (state==OVER)     { updateRings(); transAlpha=max(0.0f,transAlpha-0.02f); glutPostRedisplay(); return; }
    if (state==PAUSE)    { glutPostRedisplay(); return; }

    // fade transition
    transAlpha=(transDir<0)?max(0.0f,transAlpha-0.04f):min(1.0f,transAlpha+0.04f);

    fc++;
    updateRings();
    applyShake();

    // smooth slow-motion interpolation
    gameSpeed=lerp(gameSpeed,slowMoTarget,0.06f);
    slowMoTarget=(puSlowMo>0)?0.35f:1.0f;

    // decrement power-up timers
    if (puRapidFire>0) puRapidFire-=(int)max(1.0f,gameSpeed);
    if (puSlowMo>0)    puSlowMo--;
    if (puLaser>0)     puLaser-=(int)max(1.0f,gameSpeed);
    if (puHoming>0)    puHoming-=(int)max(1.0f,gameSpeed);
    if (waveFlash>0)   waveFlash-=gameSpeed;
    if (scoreAnim>0)   scoreAnim-=0.06f;
    if (comboPop>0)    comboPop-=0.06f;
    if (comboTimer>0)  comboTimer-=(int)gameSpeed;
    else if (comboCount>0) comboCount=0;

    // advance decoration timers
    planetRot+=0.5f*gameSpeed; blackHoleT+=gameSpeed;
    sunT+=gameSpeed; moonT+=gameSpeed; compactPlanetT+=gameSpeed;

    // player movement (2D translation with smooth tilt)
    float tAng=0;
    if (mL){ sx-=SPDS*gameSpeed; tAng=12;  }
    if (mR){ sx+=SPDS*gameSpeed; tAng=-12; }
    if (mU) sy+=SPDS*0.8f*gameSpeed;
    if (mD) sy-=SPDS*0.8f*gameSpeed;
    sang+=(tAng-sang)*0.12f;
    sx=max(28.0f,min((float)W-28,sx));
    sy=max(55.0f,min((float)H-80,sy));
    sscale=min(1.0f,sscale+0.04f);

    updatePlayerTrail();

    // parallax star scroll
    for (auto &s : stars) { s.y-=s.spd*gameSpeed; if (s.y<0){s.y=H;s.x=frand(0,W);} }

    // player bullet movement
    if (bcool>0) bcool-=(int)ceil(gameSpeed);
    for (int i=0; i<MB; i++) {
        auto &b=bul[i];
        if (!b.on) continue;
        if (b.isHoming) {
            // steer toward locked target
            float dx=b.hx-b.x, dy=b.hy-b.y, len=sqrtf(dx*dx+dy*dy)+0.001f;
            b.x+=dx/len*7*gameSpeed; b.y+=dy/len*7*gameSpeed;
        } else {
            b.y+=9*gameSpeed;
        }
        b.age++;
        if (b.y>H+10) { b.on=false; continue; }
        if (showTrails) {
            bulletTrails[i].push_front({b.x,b.y,0.8f,2.0f});
            if (bulletTrails[i].size()>10) bulletTrails[i].pop_back();
            for (auto &n : bulletTrails[i]) n.alpha*=0.7f;
        }
    }

    // enemy bullet movement and player hit detection
    for (auto &b : ebul) {
        if (!b.on) continue;
        b.x+=b.vx*gameSpeed; b.y+=b.vy*gameSpeed; b.age++;
        if (b.x<-20||b.x>W+20||b.y<-20||b.y>H+20) { b.on=false; continue; }
        if (!shieldOn && dist(b.x,b.y,sx,sy)<22) {
            b.on=false;
            spawnHitRing(sx,sy,1.0f,0.4f,0.1f);
            triggerScreenShake(5,10);
            if (--lives<=0) { saveHighScore(); state=OVER; }
        }
    }

    // enemy AI, movement, and collision logic
    for (auto &e : ene) {
        if (!e.on) continue;
        e.aiTimer+=gameSpeed;
        if (e.flashTimer>0) e.flashTimer-=gameSpeed;

        switch (e.ai) {
            case AI_SINE:
                e.y-=e.spd*gameSpeed;
                e.x+=sinf(fc*0.025f+(&e-ene))*1.2f*gameSpeed;
                e.ang=sinf(fc*0.05f+(&e-ene))*12.0f;
                break;
            case AI_ZIGZAG: {
                e.y-=e.spd*gameSpeed;
                float zz=(sinf(e.aiTimer*0.07f)>0)?1.5f:-1.5f;
                e.x=clamp(e.x+zz*e.spd*1.5f*gameSpeed,30.0f,(float)W-30);
                e.ang=zz*20;
                break;
            }
            case AI_TRACKER: {
                float dx=sx-e.x, dy=sy-e.y, len=sqrtf(dx*dx+dy*dy)+0.001f;
                e.x+=dx/len*e.spd*0.6f*gameSpeed;
                e.y+=dy/len*e.spd*0.6f*gameSpeed;
                e.ang=atan2f(dx,dy)*180/PI;
                break;
            }
            case AI_SWARM: {
                // orbit a moving formation centre
                float fx=W/2+cosf(fc*0.01f+e.swarmIdx*PI/4)*160;
                float fy=H-160+sinf(fc*0.008f+e.swarmIdx*PI/3)*60;
                e.x+=(fx-e.x)*0.025f*gameSpeed;
                e.y+=(fy-e.y)*0.025f*gameSpeed;
                e.y-=e.spd*0.3f*gameSpeed;
                break;
            }
            case AI_KAMIKAZE: {
                // accelerates continuously toward the player
                float dx=sx-e.x, dy=sy-e.y, len=sqrtf(dx*dx+dy*dy)+0.001f;
                float kSpd=e.spd*(1+e.aiTimer*0.004f);
                e.x+=dx/len*kSpd*gameSpeed;
                e.y+=dy/len*kSpd*gameSpeed;
                e.ang=atan2f(dx,dy)*180/PI;
                break;
            }
        }

        // enemy leaves screen — costs a life
        if (e.y<-40) {
            e.on=false;
            if (--lives<=0) { saveHighScore(); state=OVER; }
            continue;
        }
        if (e.x<-60||e.x>W+60) { e.on=false; continue; }

        // player bullet vs enemy
        for (int i=0; i<MB; i++) {
            auto &b=bul[i];
            if (!b.on) continue;
            if (dist(b.x,b.y,e.x,e.y)<(b.isLaser?30.0f:22.0f)) {
                b.on=false; e.flashTimer=8; spawnHitRing(e.x,e.y);
                if (--e.hp<=0) {
                    spawnExplosion(e.x,e.y,1.0f);
                    comboCount++; comboPop=1.0f; comboTimer=120;
                    int base=100;
                    if      (comboCount>=10) base*=5;
                    else if (comboCount>=5)  base*=3;
                    else if (comboCount>=2)  base*=comboCount;
                    score+=base; scoreAnim=1.0f;
                    if (rand()%5==0) spawnPowerUp(e.x,e.y);
                    e.on=false;
                }
            }
        }
        // enemy body vs player
        if (!shieldOn && dist(sx,sy,e.x,e.y)<30) {
            spawnExplosion(e.x,e.y,0.8f,1.0f,0.4f,0.1f);
            e.on=false; comboCount=0; triggerScreenShake(6,15);
            if (--lives<=0) { saveHighScore(); state=OVER; }
        }
    }

    // asteroid movement, rotation, and collision
    for (auto &a : ast) {
        if (!a.on) continue;
        a.y-=a.spd*gameSpeed; a.rot+=a.rotSpd*gameSpeed;
        if (a.flashTimer>0) a.flashTimer-=gameSpeed;
        if (a.y<-50) { a.on=false; continue; }
        // bullet hit
        for (int i=0; i<MB; i++) {
            auto &b=bul[i];
            if (!b.on||dist(b.x,b.y,a.x,a.y)>=a.r) continue;
            b.on=false; a.flashTimer=8;
            spawnHitRing(a.x,a.y,0.8f,0.6f,0.4f);
            spawnExplosion(a.x,a.y,0.9f,0.7f,0.5f,0.3f);
            a.on=false; comboCount++; comboPop=1.0f; comboTimer=120;
            score+=50+(comboCount>2?50:0); scoreAnim=1.0f;
        }
        // player body collision
        if (!shieldOn && dist(sx,sy,a.x,a.y)<a.r+18) {
            spawnExplosion(a.x,a.y,0.9f,0.7f,0.5f,0.3f);
            a.on=false; comboCount=0; triggerScreenShake(5,12);
            if (--lives<=0) { saveHighScore(); state=OVER; }
        }
    }

    // power-up drift, animation, and collection
    for (auto &p : pups) {
        if (!p.on) continue;
        p.y+=p.vy*gameSpeed; p.animT+=gameSpeed; p.bob+=0.06f*gameSpeed;
        if (p.y<-20) { p.on=false; continue; }
        if (dist(sx,sy,p.x,p.y)<28) {
            switch (p.type) {
                case PU_RAPIDFIRE: puRapidFire=300; break;
                case PU_SHIELD:    shieldOn=120; shieldCool=0; break;
                case PU_SLOWMO:    puSlowMo=180; break;
                case PU_LASER:     puLaser=180; break;
                case PU_HOMING:    puHoming=300; break;
                case PU_BOMB:      activateBomb(); break;
            }
            spawnHitRing(p.x,p.y,0.5f,1.0f,0.5f);
            p.on=false; score+=25; scoreAnim=1.0f;
        }
    }

    updateBoss();

    // wave complete: advance when all enemies and asteroids are gone
    if (!boss.on) {
        bool anyActive=false;
        for (auto &e : ene) if (e.on){anyActive=true;break;}
        if (!anyActive) for (auto &a : ast) if (a.on){anyActive=true;break;}
        if (!anyActive) {
            wave++;
            if (score>highScore) saveHighScore();
            spawnWave();
        }
    }

    // shield cooldown
    shieldPulse++;
    if (shieldOn>0)    shieldOn-=(int)ceil(gameSpeed);
    if (shieldCool>0)  shieldCool-=(int)ceil(gameSpeed);

    // explosion particle physics (gravity + friction per particle)
    for (auto &ex : exps) {
        if (!ex.on) continue;
        ex.scale+=0.055f*gameSpeed; ex.alpha-=0.028f*gameSpeed;
        if (ex.alpha<=0) { ex.on=false; continue; }
        for (auto &p : ex.pts) {
            if (!p.on) continue;
            p.x+=p.vx*gameSpeed; p.y+=p.vy*gameSpeed;
            p.vy+=p.gravity*gameSpeed;
            p.vx*=pow(p.friction,gameSpeed); p.vy*=pow(p.friction,gameSpeed);
            p.life-=0.022f*gameSpeed;
            if (p.life<=0) p.on=false;
        }
    }

    glutPostRedisplay();
}

// Display Callback 
// Renders the correct content for the current game state each frame.
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (state==SPLASH) { drawSplash(); glutSwapBuffers(); return; }
    if (state==MENU)   { drawMenu();   glutSwapBuffers(); return; }
    if (state==SETTINGS){ drawSettings(); glutSwapBuffers(); return; }

    glPushMatrix();
    glTranslatef(shakeX, shakeY, 0); // apply screen shake offset

    // layered background rendering
    drawBackground();
    drawBlackHole();
    drawNebula();
    drawStars();
    drawRings();
    drawPlanet();
    drawSun();
    drawMoon();
    drawCompactPlanet();

    // bullet motion trails
    if (showTrails) {
        for (int i=0; i<MB; i++)
            for (auto &n : bulletTrails[i]) {
                glColor4f(0.3f,0.9f,1.0f,n.alpha*0.4f);
                glPointSize(n.sz);
                glBegin(GL_POINTS); glVertex2f(n.x,n.y); glEnd();
            }
        glPointSize(1);
    }

    // gameplay objects
    drawPlayerTrail();
    drawExps();
    updateDrawHitRings();
    for (auto &a : ast)  if (a.on) drawAst(a);
    for (auto &e : ene)  if (e.on) drawEnemy(e);
    for (auto &b : ebul) if (b.on) drawEnemyBullet(b);
    drawBoss();
    for (auto &p : pups) if (p.on) drawPowerUp(p);
    for (auto &b : bul)  if (b.on) drawBullet(b);
    if (shieldOn>0) drawShield(sx,sy);
    drawShip(sx,sy,sang,sscale);

    // overlays
    drawSlowMoOverlay();
    drawHUD();
    drawRadar();
    drawCombo();
    drawWaveBanner();
    glPopMatrix();

    if (state==PAUSE) drawPauseOverlay();
    if (state==OVER)  drawOver();
    drawTransitionFade();

    glutSwapBuffers();
}

// ── Window & Input Callbacks 

void reshape(int w, int h) {
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0,W,0,H);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}

void keyDown(unsigned char k, int, int) {
    if (k==27) {
        if (state==SETTINGS){ state=MENU; return; }
        exit(0);
    }
    if (state==SPLASH) { state=MENU; splashT=180; return; }
    if (state==SETTINGS) { if (k=='\t'||k==9) state=MENU; return; }

    if (k==' ' && state==MENU)  { initGame(); return; }
    if ((k=='\t'||k==9) && state==MENU)  { state=SETTINGS; return; }
    if ((k=='\t'||k==9) && state==PLAY)  { state=PAUSE; return; }
    if ((k=='\t'||k==9) && state==PAUSE) { state=SETTINGS; return; }

    if (k==13 && state==PLAY) firePlayer();
    if (k=='r'||k=='R') { initGame(); return; }
    if ((k=='p'||k=='P') && (state==PLAY||state==PAUSE))
        state=(state==PLAY)?PAUSE:PLAY;
    if ((k=='s'||k=='S') && state==PLAY && !shieldCool && !shieldOn)
        { shieldOn=90; shieldCool=300; }

    if (k=='w'||k=='W') mU=1;
    if (k=='a'||k=='A') mL=1;
    if (k=='d'||k=='D') mR=1;
    if (k=='x'||k=='X') mD=1;
    if (k=='s' && shieldCool>0) mD=1;
}

void keyUp(unsigned char k, int, int) {
    if (k=='w'||k=='W') mU=0;
    if (k=='a'||k=='A') mL=0;
    if (k=='d'||k=='D') mR=0;
    if (k=='x'||k=='X') mD=0;
    if (k=='s' && shieldCool>0) mD=0;
}

void spDown(int k, int, int) {
    if (state==SETTINGS) {
        if (k==GLUT_KEY_UP)   settingsSel=max(0,settingsSel-1);
        if (k==GLUT_KEY_DOWN) settingsSel=min(3,settingsSel+1);
        if (k==GLUT_KEY_LEFT||k==GLUT_KEY_RIGHT) {
            int dir=(k==GLUT_KEY_RIGHT)?1:-1;
            switch (settingsSel) {
                case 0: difficulty=clamp(difficulty+dir,0,2); break;
                case 1: showRadar=!showRadar; break;
                case 2: showTrails=!showTrails; break;
                case 3: state=MENU; break;
            }
        }
        return;
    }
    if (k==GLUT_KEY_UP)    mU=1;
    if (k==GLUT_KEY_DOWN)  mD=1;
    if (k==GLUT_KEY_LEFT)  mL=1;
    if (k==GLUT_KEY_RIGHT) mR=1;
    if (k==GLUT_KEY_F1 && state==PLAY) firePlayer();
}

void spUp(int k, int, int) {
    if (state==SETTINGS) return;
    if (k==GLUT_KEY_UP)    mU=0;
    if (k==GLUT_KEY_DOWN)  mD=0;
    if (k==GLUT_KEY_LEFT)  mL=0;
    if (k==GLUT_KEY_RIGHT) mR=0;
}

// Timer callback: drives the update loop at approximately 60 fps
void timer(int) { update(); glutTimerFunc(16,timer,0); }

// ── Entry Point 
int main(int argc, char **argv) {
    srand(time(0));
    loadHighScore();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(W,H);
    glutInitWindowPosition(100,60);
    glutCreateWindow("PARTH & SPACE SHOOTER");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0,0,0.04f,1);
    initStars();
    initRings();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(spDown);
    glutSpecialUpFunc(spUp);
    glutTimerFunc(16,timer,0);
    glutMainLoop();
    return 0;
}