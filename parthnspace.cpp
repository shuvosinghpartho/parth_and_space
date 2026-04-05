/*
 * =========================================
 *  PARTH & SPACE — ULTIMATE EDITION
 *  Next-Level 2D Space Shooter
 *  OpenGL / FreeGLUT | C++
 *  Author : Shuvo Singh Partho
 *  Gmail  : partho23105101445@diu.edu.bd
 *
 *  Original → Enhanced by: Claude (Anthropic)
 * =========================================
 *
 *  NEW FEATURES:
 *   - Boss fights with multi-phase attacks
 *   - Power-ups: RapidFire, Shield, SlowMo, Laser, Homing
 *   - Enemy AI: Zigzag, Tracker, Swarm, Kamikaze
 *   - Combo scoring + dynamic difficulty
 *   - Level zones (Space, Asteroid Belt, Nebula, Boss Arena)
 *   - Screen shake, slow-motion, trail effects
 *   - Animated intro splash screen
 *   - Save/load high score (file I/O)
 *   - Mini-map / radar
 *   - Settings menu
 *   - Smooth state transitions with alpha fades
 *   - Cinematic hit effects & kill animations
 *   - Rotating planets & animated backgrounds
 *   - Real-time particle physics (gravity, friction)
 *   - Modular entity component system
 *
 *  COMPILE:
 *    g++ parthnspace_ultimate.cpp -o parthnspace_ultimate -lGL -lGLU -lglut -lm
 *  OR on Windows:
 *    g++ parthnspace_ultimate.cpp -o parthnspace_ultimate -lfreeglut -lopengl32 -lglu32
 * =========================================
 */

#include <GL/glut.h>
#include <bits/stdc++.h>
#include <fstream>
using namespace std;

// ── Window & Constants ─────────────────────────────────────────────────────
const int W = 900, H = 700;
const float PI = acos(-1.0f);

// ── Forward declarations ────────────────────────────────────────────────────
void initGame();
void spawnBoss();
void spawnExplosion(float x, float y, float intensity = 1.0f,
                   float cr = 1.0f, float cg = 0.5f, float cb = 0.1f);
void triggerScreenShake(float intensity, int frames);
void spawnPowerUp(float x, float y);

// ─────────────────────────────────────────────────────────────────────────────
//  ENUMS & TYPES
// ─────────────────────────────────────────────────────────────────────────────
enum State        { SPLASH, MENU, SETTINGS, PLAY, PAUSE, OVER, TRANSITION };
enum ZoneType     { ZONE_SPACE, ZONE_ASTEROID, ZONE_NEBULA, ZONE_BOSS };
enum EnemyAI      { AI_SINE, AI_ZIGZAG, AI_TRACKER, AI_SWARM, AI_KAMIKAZE };
enum PowerUpType  { PU_RAPIDFIRE, PU_SHIELD, PU_SLOWMO, PU_LASER, PU_HOMING, PU_BOMB };
enum BossPhase    { BOSS_PHASE1, BOSS_PHASE2, BOSS_PHASE3 };

State     state        = SPLASH;
State     nextState    = MENU;   // for TRANSITION
ZoneType  currentZone  = ZONE_SPACE;

// ─────────────────────────────────────────────────────────────────────────────
//  STRUCTS — Base / Component style
// ─────────────────────────────────────────────────────────────────────────────
struct Star   { float x, y, spd, bright, sz; int layer; };
struct NeonRing{ float x, y, r, alpha, spd; bool on; };

// Trail node for player / bullets
struct TrailNode { float x, y, alpha, sz; };

// Generic Particle with physics
struct Particle {
    float x, y, vx, vy;
    float life, maxLife;
    float r, g, b, a;
    float sz, gravity, friction;
    bool  on;
};

// Explosion cluster
struct Explosion {
    float x, y, scale, alpha;
    bool  on;
    vector<Particle> pts;
    float r, g, b;   // color tint
};

// Player bullet
struct Bullet {
    float x, y;
    bool  on;
    int   age;
    bool  isLaser;   // wide laser shot
    bool  isHoming;  // home to nearest enemy
    float hx, hy;    // homing target
};

// Enemy
struct Enemy {
    float x, y, baseY;
    float spd, ang, flashTimer;
    bool  on;
    int   hp, maxHp;
    EnemyAI ai;
    float aiTimer;   // general AI timer
    float tx, ty;    // tracking target
    int   swarmIdx;  // for swarm formation
};

// Asteroid
struct Asteroid {
    float x, y, spd, rot, rotSpd, r;
    bool  on;
    float flashTimer;
};

// Power-Up pickup
struct PowerUp {
    float x, y, vy;
    PowerUpType type;
    bool on;
    float animT;
    float bob;       // bobbing phase
};

// Enemy bullet (boss shoots back)
struct EnemyBullet {
    float x, y, vx, vy;
    bool on;
    int  age;
};

// ── BOSS ───────────────────────────────────────────────────────────────────
struct Boss {
    float  x, y;
    float  targetX, targetY;
    float  vx, vy;
    int    hp, maxHp;
    bool   on;
    BossPhase phase;
    float  phaseTimer;
    float  shootCd;
    float  ang, angSpd;
    float  flashTimer;
    float  entryProgress;    // 0→1 slide-in animation
    bool   enraged;
    int    attackPattern;    // 0,1,2 different patterns
    float  patternTimer;
    // Shield orbs rotating around boss
    float  orbAng[4];
    bool   orbOn[4];
    int    orbHp[4];
};
Boss boss;

// ─────────────────────────────────────────────────────────────────────────────
//  GAME STATE
// ─────────────────────────────────────────────────────────────────────────────

// Player state
float sx=W/2, sy=80, sang=0, sscale=1;
bool  mL=0, mR=0, mU=0, mD=0;
const float SPDS = 4.5f;

// Gameplay
int   score=0, highScore=0, lives=3, wave=1, fc=0, bcool=0;
int   comboCount=0, comboTimer=0;
float comboPop=0;           // pop animation
int   shieldOn=0, shieldCool=0;
float shieldPulse=0;

// Active power-ups (duration countdown)
int   puRapidFire=0, puSlowMo=0, puLaser=0, puHoming=0;

// Slow-mo state
float gameSpeed = 1.0f;     // multiplied onto all updates
float slowMoTarget = 1.0f;

// Screen shake
float shakeX=0, shakeY=0;
float shakeMag=0;
int   shakeFrames=0;

// Transition fade
float transAlpha = 0.0f;
int   transDir   = 1;       // +1 fade in, -1 fade out

// Animations
float menuAnim   = 0;
float waveFlash  = 0;
float scoreAnim  = 0;
int   lastScore  = 0;
float splashT    = 0;       // splash timer (0–180 frames)
float settingsAnim = 0;
int   settingsSel  = 0;     // selected setting index

// Settings data
int   difficulty    = 1;    // 0=Easy 1=Normal 2=Hard
bool  showRadar     = true;
bool  showTrails    = true;

// Planet rotation
float planetRot = 0.0f;
// Black hole pulse
float blackHoleT = 0.0f;

// Cinematic slow kill
float slowKillTimer = 0.0f;  // frames left in slow-motion kill

// Level progression
int   levelZoneTimer = 0;    // frames until next zone

// Array sizes
const int MB=16, ME=10, MA=8, NS=240, NPU=6, NEB=12;
Bullet      bul[MB];
Enemy       ene[ME];
Asteroid    ast[MA];
Star        stars[NS];
NeonRing    rings[NEB];
PowerUp     pups[NPU];
EnemyBullet ebul[24];
vector<Explosion> exps;

// Trail history for player
deque<TrailNode> playerTrail;
// Per-bullet trail (indexed)
deque<TrailNode> bulletTrails[MB];

// ─────────────────────────────────────────────────────────────────────────────
//  FILE I/O — Save & Load High Score
// ─────────────────────────────────────────────────────────────────────────────
const string SAVE_FILE = "parthnspace_save.dat";

void loadHighScore(){
    ifstream f(SAVE_FILE);
    if(f.is_open()){ f >> highScore; f.close(); }
    else highScore = 0;
}

void saveHighScore(){
    if(score > highScore){
        highScore = score;
        ofstream f(SAVE_FILE);
        if(f.is_open()){ f << highScore; f.close(); }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  HELPERS
// ─────────────────────────────────────────────────────────────────────────────
float frand(float lo,float hi){ return lo+(hi-lo)*(rand()/(float)RAND_MAX); }
float dist(float ax,float ay,float bx,float by){ return hypotf(ax-bx,ay-by); }
float lerp(float a, float b, float t){ return a + (b-a)*t; }
float clamp(float v, float lo, float hi){ return max(lo, min(hi, v)); }

// ─────────────────────────────────────────────────────────────────────────────
//  PRIMITIVE ALGORITHMS (DDA, Bresenham, Midpoint Circle) — PRESERVED
// ─────────────────────────────────────────────────────────────────────────────
void dda(int x1,int y1,int x2,int y2){
    int dx=x2-x1, dy=y2-y1;
    int steps=max(abs(dx),abs(dy));
    if(!steps) return;
    float xi=dx/(float)steps, yi=dy/(float)steps;
    float x=x1, y=y1;
    glBegin(GL_POINTS);
    for(int i=0;i<=steps;i++,x+=xi,y+=yi)
        glVertex2f(roundf(x),roundf(y));
    glEnd();
}

void bres(int x1,int y1,int x2,int y2){
    int dx=abs(x2-x1), dy=abs(y2-y1);
    int sx_=(x1<x2)?1:-1, sy_=(y1<y2)?1:-1, err=dx-dy;
    glBegin(GL_POINTS);
    while(true){
        glVertex2f(x1,y1);
        if(x1==x2&&y1==y2) break;
        int e2=2*err;
        if(e2>-dy){err-=dy;x1+=sx_;}
        if(e2< dx){err+=dx;y1+=sy_;}
    }
    glEnd();
}

void mpc(int cx,int cy,int r){
    if(r<=0) return;
    int x=0, y=r, p=1-r;
    auto p8=[&](int px,int py){
        glVertex2f(cx+px,cy+py); glVertex2f(cx-px,cy+py);
        glVertex2f(cx+px,cy-py); glVertex2f(cx-px,cy-py);
        glVertex2f(cx+py,cy+px); glVertex2f(cx-py,cy+px);
        glVertex2f(cx+py,cy-px); glVertex2f(cx-py,cy-px);
    };
    glBegin(GL_POINTS); p8(x,y);
    while(x<y){
        x++;
        p=(p<0)?p+2*x+1:p+2*(x- --y)+1;
        p8(x,y);
    }
    glEnd();
}

void mpcFill(int cx,int cy,int r){
    for(int i=1;i<=r;i++) mpc(cx,cy,i);
}

// ─────────────────────────────────────────────────────────────────────────────
//  GLOW HELPERS — layered glow, neon bloom
// ─────────────────────────────────────────────────────────────────────────────
void glowCircle(int cx,int cy,int coreR,float r,float g,float b,float coreAlpha,int layers=5){
    for(int i=layers;i>=1;i--){
        float t=i/(float)layers;
        int   rad=coreR+layers*3-i*3;
        glColor4f(r,g,b,coreAlpha*t*0.35f);
        mpc(cx,cy,rad);
    }
    glColor4f(r,g,b,coreAlpha);
    mpcFill(cx,cy,coreR);
}

// Thick glow line (DDA-based)
void glowLine(int x1,int y1,int x2,int y2,float r,float g,float b,float a,int thick=2){
    for(int t=thick;t>=1;t--){
        glColor4f(r,g,b,a*t/(float)thick*0.5f);
        for(int off=-t;off<=t;off++){
            dda(x1,y1+off,x2,y2+off);
            dda(x1+off,y1,x2+off,y2);
        }
    }
    glColor4f(r,g,b,a);
    dda(x1,y1,x2,y2);
}

// ─────────────────────────────────────────────────────────────────────────────
//  TEXT HELPERS
// ─────────────────────────────────────────────────────────────────────────────
void txt(float x,float y,const string& s,void* f=GLUT_BITMAP_HELVETICA_18){
    glRasterPos2f(x,y);
    for(char c:s) glutBitmapCharacter(f,c);
}

int strWidth(const string& s,void* f){
    int w=0;
    for(char c:s) w+=glutBitmapWidth(f,c);
    return w;
}

void txtC(float cx,float y,const string& s,void* f=GLUT_BITMAP_HELVETICA_18){
    float w=strWidth(s,f);
    txt(cx-w/2.0f,y,s,f);
}

void txtShadowC(float cx,float y,const string& s,
                float r,float g,float b,void* f=GLUT_BITMAP_HELVETICA_18){
    float w=strWidth(s,f);
    float x=cx-w/2.0f;
    glColor4f(0,0,0,0.75f); txt(x+2,y-2,s,f);
    glColor3f(r,g,b);       txt(x,y,s,f);
}

// Glowing text — draws shadow + main + a bright outline offset for bloom feel
void txtGlow(float cx,float y,const string& s,
             float r,float g,float b,float glow,
             void* f=GLUT_BITMAP_TIMES_ROMAN_24){
    float w=strWidth(s,f);
    float x=cx-w/2.0f;
    // shadow
    glColor4f(0,0,0,0.8f); txt(x+2,y-2,s,f);
    // glow halo (slight offsets)
    glColor4f(r,g,b,glow*0.3f);
    txt(x-1,y+1,s,f); txt(x+1,y+1,s,f);
    txt(x-1,y-1,s,f); txt(x+1,y-1,s,f);
    // main
    glColor3f(r+glow*0.2f,g+glow*0.1f,b);
    txt(x,y,s,f);
}

// Wave text animation — each character slightly offset by sin
void txtWave(float cx,float y,const string& s,float r,float g,float b,
             float waveAmt,float phase,void* f=GLUT_BITMAP_HELVETICA_18){
    int totalW=strWidth(s,f);
    float x=cx-totalW/2.0f;
    for(int i=0;i<(int)s.size();i++){
        float wy=waveAmt*sinf(phase+i*0.4f);
        glColor3f(r,g,b);
        glRasterPos2f(x,y+wy);
        glutBitmapCharacter(f,s[i]);
        x+=glutBitmapWidth(f,s[i]);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  SCREEN SHAKE
// ─────────────────────────────────────────────────────────────────────────────
void triggerScreenShake(float intensity,int frames){
    shakeMag=intensity; shakeFrames=frames;
}

void applyShake(){
    if(shakeFrames>0){
        shakeX=frand(-shakeMag,shakeMag);
        shakeY=frand(-shakeMag,shakeMag);
        shakeFrames--; shakeMag*=0.88f;
    } else { shakeX=shakeY=0; }
}

// ─────────────────────────────────────────────────────────────────────────────
//  ZONE BACKGROUNDS
// ─────────────────────────────────────────────────────────────────────────────

// Deep space gradient (original)
void drawBackgroundSpace(){
    glBegin(GL_QUADS);
        glColor3f(0.00f,0.00f,0.05f); glVertex2f(0,0); glVertex2f(W,0);
        glColor3f(0.01f,0.00f,0.09f); glVertex2f(W,H/2); glVertex2f(0,H/2);
    glEnd();
    glBegin(GL_QUADS);
        glColor3f(0.01f,0.00f,0.09f); glVertex2f(0,H/2); glVertex2f(W,H/2);
        glColor3f(0.03f,0.00f,0.12f); glVertex2f(W,H); glVertex2f(0,H);
    glEnd();
}

// Asteroid Belt — brownish tinge
void drawBackgroundAsteroid(){
    glBegin(GL_QUADS);
        glColor3f(0.04f,0.02f,0.00f); glVertex2f(0,0); glVertex2f(W,0);
        glColor3f(0.07f,0.04f,0.01f); glVertex2f(W,H/2); glVertex2f(0,H/2);
    glEnd();
    glBegin(GL_QUADS);
        glColor3f(0.07f,0.04f,0.01f); glVertex2f(0,H/2); glVertex2f(W,H/2);
        glColor3f(0.05f,0.03f,0.01f); glVertex2f(W,H); glVertex2f(0,H);
    glEnd();
    // Dust bands
    for(int i=0;i<4;i++){
        float y0=100+i*150;
        glColor4f(0.15f,0.10f,0.04f,0.04f);
        glBegin(GL_QUADS);
            glVertex2f(0,y0); glVertex2f(W,y0+30);
            glVertex2f(W,y0+55); glVertex2f(0,y0+25);
        glEnd();
    }
}

// Nebula Zone — green/teal dream
void drawBackgroundNebula(){
    glBegin(GL_QUADS);
        glColor3f(0.00f,0.03f,0.05f); glVertex2f(0,0); glVertex2f(W,0);
        glColor3f(0.00f,0.06f,0.10f); glVertex2f(W,H/2); glVertex2f(0,H/2);
    glEnd();
    glBegin(GL_QUADS);
        glColor3f(0.00f,0.06f,0.10f); glVertex2f(0,H/2); glVertex2f(W,H/2);
        glColor3f(0.02f,0.08f,0.10f); glVertex2f(W,H); glVertex2f(0,H);
    glEnd();
}

// Boss Arena — dark red/crimson
void drawBackgroundBoss(){
    glBegin(GL_QUADS);
        glColor3f(0.06f,0.00f,0.00f); glVertex2f(0,0); glVertex2f(W,0);
        glColor3f(0.10f,0.00f,0.01f); glVertex2f(W,H/2); glVertex2f(0,H/2);
    glEnd();
    glBegin(GL_QUADS);
        glColor3f(0.10f,0.00f,0.01f); glVertex2f(0,H/2); glVertex2f(W,H/2);
        glColor3f(0.08f,0.01f,0.02f); glVertex2f(W,H); glVertex2f(0,H);
    glEnd();
}

void drawBackground(){
    switch(currentZone){
        case ZONE_SPACE:    drawBackgroundSpace();    break;
        case ZONE_ASTEROID: drawBackgroundAsteroid(); break;
        case ZONE_NEBULA:   drawBackgroundNebula();   break;
        case ZONE_BOSS:     drawBackgroundBoss();     break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  NEBULA CLOUDS (zone-adaptive)
// ─────────────────────────────────────────────────────────────────────────────
void drawNebula(){
    if(currentZone==ZONE_SPACE){
        for(int r=120;r>0;r-=4){
            float t=r/120.0f;
            glColor4f(0.28f*t,0.05f*t,0.55f*t,0.018f*(1-t));
            mpc(180,440,r);
        }
        for(int r=95;r>0;r-=4){
            float t=r/95.0f;
            glColor4f(0.0f,0.25f*t,0.55f*t,0.016f*(1-t));
            mpc(700,240,r);
        }
    } else if(currentZone==ZONE_NEBULA){
        // More vivid nebula clouds
        for(int r=150;r>0;r-=3){
            float t=r/150.0f;
            glColor4f(0.0f,0.4f*t,0.6f*t,0.022f*(1-t));
            mpc(200,400,r);
        }
        for(int r=120;r>0;r-=3){
            float t=r/120.0f;
            glColor4f(0.1f*t,0.5f*t,0.3f*t,0.020f*(1-t));
            mpc(680,280,r);
        }
        for(int r=100;r>0;r-=3){
            float t=r/100.0f;
            glColor4f(0.2f*t,0.45f*t,0.55f*t,0.018f*(1-t));
            mpc(450,180,r);
        }
    } else if(currentZone==ZONE_BOSS){
        // Dark red energy clouds
        for(int r=140;r>0;r-=4){
            float t=r/140.0f;
            float pulse=0.5f+0.5f*sinf(fc*0.02f);
            glColor4f(0.5f*t*pulse,0.0f,0.05f*t,0.018f*(1-t));
            mpc(180,350,r);
        }
        for(int r=110;r>0;r-=4){
            float t=r/110.0f;
            float pulse=0.5f+0.5f*sinf(fc*0.025f+1.5f);
            glColor4f(0.4f*t*pulse,0.05f*t,0.0f,0.016f*(1-t));
            mpc(700,380,r);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  PARALLAX STARS (3 layers, zone-tinted)
// ─────────────────────────────────────────────────────────────────────────────
void initStars(){
    for(int i=0;i<NS;i++){
        int l=i%3;
        stars[i]={ frand(0,W),frand(0,H),
                   l==0?0.25f:l==1?0.7f:1.6f,
                   l==0?0.35f:l==1?0.65f:1.0f,
                   l==0?1.0f:l==1?1.5f:2.5f, l };
    }
}

void drawStars(){
    for(auto& s:stars){
        float tw=s.bright*(0.8f+0.2f*sinf(fc*0.07f+s.x));
        // Tint stars based on zone
        float tr=tw,tg=tw,tb=tw;
        if(currentZone==ZONE_NEBULA){ tr=tw*0.7f; tg=tw*1.0f; tb=tw*1.1f; }
        if(currentZone==ZONE_BOSS)  { tr=tw*1.2f; tg=tw*0.7f; tb=tw*0.7f; }
        glColor3f(tr,tg,tb);
        glPointSize(s.sz);
        glBegin(GL_POINTS); glVertex2f(s.x,s.y); glEnd();
        if(s.layer==2){
            glColor4f(tr*0.8f,tg*0.85f,tb*1.0f,0.18f);
            glPointSize(s.sz*2.5f);
            glBegin(GL_POINTS); glVertex2f(s.x,s.y); glEnd();
        }
    }
    glPointSize(1);
}

// ─────────────────────────────────────────────────────────────────────────────
//  ANIMATED PLANET — now rotates via planetRot
// ─────────────────────────────────────────────────────────────────────────────
void drawPlanet(){
    if(currentZone==ZONE_BOSS) return; // Boss arena has no planet

    int px=720, py=560, pr=90;

    // Atmosphere halo
    for(int r=pr+18;r>=pr+1;r--){
        float t=(r-pr)/18.0f;
        glColor4f(0.4f,0.1f,0.85f,0.06f*(1-t));
        mpc(px,py,r);
    }
    // Planet body (rotates via drawing rings at planetRot offset)
    for(int r=pr;r>0;r-=2){
        float t=1.0f-r/(float)pr;
        glColor3f(0.05f+0.35f*t,0.0f+0.05f*t,0.18f+0.38f*t);
        mpc(px,py,r);
    }
    // Surface highlight
    glColor4f(0.6f,0.5f,1.0f,0.25f);
    for(int r=pr-4;r<=pr-1;r++) mpc(px-18,py+18,r);
    // Rings (orbital bands, shifted by planetRot)
    float roff=sinf(planetRot*0.02f)*6;
    glColor4f(0.55f,0.22f,1.0f,0.45f);
    for(int b=-4;b<=4;b++){
        int off=(int)sqrtf(max(0.0f,(float)(pr*pr-(b*13)*(b*13))));
        bres(px-off-10,py+b*13+(int)roff,px+off+10,py+b*13+(int)roff);
    }
    glColor4f(0.8f,0.5f,1.0f,0.18f);
    for(int b=-1;b<=1;b++){
        int off=(int)sqrtf(max(0.0f,(float)(pr*pr-(b*13)*(b*13))));
        bres(px-off-10,py+b*13+(int)roff,px+off+10,py+b*13+(int)roff);
    }
    // Moon with orbit
    float moonAng=planetRot*0.008f;
    int mx=px+(int)(115*cosf(moonAng)), my=py+(int)(60*sinf(moonAng));
    glColor3f(0.55f,0.52f,0.58f); mpcFill(mx,my,12);
    glColor4f(0.8f,0.8f,1.0f,0.15f);
    for(int r=13;r<=16;r++) mpc(mx,my,r);
}

// ── BLACK HOLE (Boss arena background element) ─────────────────────────────
void drawBlackHole(){
    if(currentZone!=ZONE_BOSS) return;
    int bx=150, by=580, br=55;
    float pulse=0.5f+0.5f*sinf(blackHoleT*0.04f);
    // Accretion disk
    for(int r=br+40;r>=br+5;r--){
        float t=(r-br-5)/35.0f;
        float a=0.0f;
        if(r<=br+15) a=0.25f*(1-t)*pulse;
        else         a=0.06f*(1-t);
        float hr=0.6f+0.3f*sinf(t*PI);
        glColor4f(hr,0.2f,0.05f,a);
        mpc(bx,by,r);
    }
    // Event horizon (pure black fill)
    glColor3f(0,0,0);
    mpcFill(bx,by,br);
    // Inner glowing ring
    glColor4f(0.9f,0.3f,0.1f,0.6f+0.35f*pulse);
    mpc(bx,by,br); mpc(bx,by,br+1);
    // Gravitational lens flare (4 lines radiating)
    for(int i=0;i<8;i++){
        float ang=i*PI/4+blackHoleT*0.01f;
        float len=18+10*pulse;
        glColor4f(1.0f,0.5f,0.1f,0.2f*pulse);
        dda(bx+(int)(br*cosf(ang)),by+(int)(br*sinf(ang)),
            bx+(int)((br+len)*cosf(ang)),by+(int)((br+len)*sinf(ang)));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  NEON RINGS
// ─────────────────────────────────────────────────────────────────────────────
void initRings(){
    float xs[]={200,680,450,150,750,500, 350,600,100,800,250,700};
    float ys[]={350,200,100,580,500,380, 250,450,300,350,480,600};
    float sp[]={0.4f,0.6f,0.5f,0.3f,0.45f,0.35f, 0.55f,0.4f,0.65f,0.3f,0.5f,0.45f};
    for(int i=0;i<NEB;i++)
        rings[i]={xs[i],ys[i],0,0,sp[i],true};
}

void updateRings(){
    for(auto& rg:rings){
        rg.r+=rg.spd;
        rg.alpha=max(0.0f,0.22f-rg.r/300.0f);
        if(rg.r>280){ rg.r=0; rg.alpha=0.20f; }
    }
}

void drawRings(){
    float cr=0.2f,cg=0.5f,cb=1.0f;
    if(currentZone==ZONE_NEBULA){ cr=0.1f; cg=0.8f; cb=0.7f; }
    if(currentZone==ZONE_BOSS)  { cr=0.8f; cg=0.1f; cb=0.2f; }
    for(auto& rg:rings){
        glColor4f(cr,cg,cb,rg.alpha*0.6f); mpc((int)rg.x,(int)rg.y,(int)rg.r);
        glColor4f(cr+0.2f,cg+0.2f,cb,rg.alpha*0.3f); mpc((int)rg.x,(int)rg.y,(int)rg.r+2);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  PLAYER SHIP DRAW (with trail)
// ─────────────────────────────────────────────────────────────────────────────
void drawShip(float x,float y,float ang,float sc){
    glPushMatrix();
    glTranslatef(x,y,0);
    glRotatef(ang,0,0,1);
    glScalef(sc,sc,1);

    bool flk=(fc%6<3);

    // Engine bloom
    glColor4f(0.2f,0.5f,1.0f,0.12f);
    for(int r=14;r>=6;r--) mpc(-12,-28,r);
    for(int r=14;r>=6;r--) mpc( 12,-28,r);

    // Exhaust flame — brighter if rapid fire
    float exLen=flk?-50:-43;
    if(puRapidFire>0) exLen-=8;
    glColor3f(0.1f,0.4f,1.0f);
    dda(-11,-28,-13,(int)exLen); dda(11,-28,13,(int)exLen);
    glColor3f(0.4f,0.8f,1.0f);
    dda(-11,-28,-12,(int)(exLen+6)); dda(11,-28,12,(int)(exLen+6));
    glColor3f(0.9f,0.95f,1.0f);
    dda(-11,-28,-11,(int)(exLen+12)); dda(11,-28,11,(int)(exLen+12));

    // Power-up tint on wings
    float wr=0.3f,wg=0.7f,wb=1.0f;
    if(puRapidFire>0){ wr=1.0f; wg=0.8f; wb=0.2f; }
    if(puSlowMo>0)   { wr=0.5f; wg=0.2f; wb=1.0f; }
    if(puLaser>0)    { wr=1.0f; wg=0.3f; wb=0.8f; }
    if(puHoming>0)   { wr=0.2f; wg=1.0f; wb=0.6f; }

    // Wing glow
    glColor4f(wr,wg,wb,0.15f);
    for(int r=8;r>=2;r--){ mpc(-18,-20,r); mpc(18,-20,r); }

    // Hull
    glColor3f(0.45f,0.80f,1.0f);
    glBegin(GL_POLYGON);
        glVertex2f(0,42); glVertex2f(-12,12);
        glVertex2f(-16,-18); glVertex2f(0,-8);
        glVertex2f(16,-18); glVertex2f(12,12);
    glEnd();
    glColor4f(0.8f,0.95f,1.0f,0.6f);
    dda(0,42,-12,12); dda(0,42,12,12);

    // Cockpit
    glColor3f(0.08f,0.22f,0.70f);
    glBegin(GL_POLYGON);
        glVertex2f(0,30); glVertex2f(-6,16); glVertex2f(0,12); glVertex2f(6,16);
    glEnd();
    glColor4f(0.7f,0.9f,1.0f,0.5f); dda(-3,27,0,20);

    // Wings
    glColor3f(0.15f,0.50f,0.90f);
    glBegin(GL_POLYGON);
        glVertex2f(-12,12);glVertex2f(-16,-18);glVertex2f(-22,-24);glVertex2f(-9,8);
    glEnd();
    glBegin(GL_POLYGON);
        glVertex2f(12,12);glVertex2f(16,-18);glVertex2f(22,-24);glVertex2f(9,8);
    glEnd();

    // Wing tips
    glColor3f(wr,wg,wb);
    dda(-22,-24,-28,-32); dda(-28,-32,-11,-27);
    dda(22,-24,28,-32);   dda(28,-32,11,-27);

    // Engine pods
    glColor4f(wr,wg,wb,0.5f);
    mpc(-11,-28,5); mpc(11,-28,5);

    // Spine
    glColor4f(0.6f,0.9f,1.0f,0.7f); dda(0,38,0,8);

    glPopMatrix();
}

// ─────────────────────────────────────────────────────────────────────────────
//  TRAIL RENDERING
// ─────────────────────────────────────────────────────────────────────────────
void updatePlayerTrail(){
    if(!showTrails) return;
    playerTrail.push_front({sx,sy,1.0f,3.0f});
    if(playerTrail.size()>22) playerTrail.pop_back();
    // fade all nodes
    for(auto& n:playerTrail) n.alpha*=0.82f;
}

void drawPlayerTrail(){
    if(!showTrails) return;
    for(int i=0;i<(int)playerTrail.size();i++){
        auto& n=playerTrail[i];
        float t=1.0f-i/(float)playerTrail.size();
        // Use power-up color for trail
        float tr=0.3f,tg=0.7f,tb=1.0f;
        if(puRapidFire>0){ tr=1.0f; tg=0.5f; tb=0.1f; }
        if(puSlowMo>0)   { tr=0.4f; tg=0.2f; tb=1.0f; }
        glColor4f(tr,tg,tb,n.alpha*t*0.5f);
        glPointSize(n.sz*t);
        glBegin(GL_POINTS); glVertex2f(n.x,n.y); glEnd();
    }
    glPointSize(1);
}

// ─────────────────────────────────────────────────────────────────────────────
//  SHIELD
// ─────────────────────────────────────────────────────────────────────────────
void drawShield(float x,float y){
    float p=0.5f+0.5f*sinf(shieldPulse*0.18f);
    for(int r=52;r>=44;r--){
        glColor4f(0.1f,0.7f,1.0f,0.04f*(1-(r-44)/8.0f)); mpc((int)x,(int)y,r);
    }
    glColor4f(0.15f,0.65f,1.0f,0.08f+0.05f*p);
    mpcFill((int)x,(int)y,40);
    glColor4f(0.2f,0.9f,1.0f,0.7f+0.25f*p);
    mpc((int)x,(int)y,40); mpc((int)x,(int)y,41);
    for(int i=0;i<12;i++){
        float a1=i*30*PI/180, a2=(i+1)*30*PI/180;
        glColor4f(0.5f,1.0f,1.0f,0.5f+0.3f*p);
        bres((int)(x+43*cosf(a1)),(int)(y+43*sinf(a1)),
             (int)(x+43*cosf(a2)),(int)(y+43*sinf(a2)));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  ENEMY DRAW (with AI type indicator)
// ─────────────────────────────────────────────────────────────────────────────
void drawEnemy(const Enemy& e){
    glPushMatrix();
    glTranslatef(e.x,e.y,0);
    glRotatef(e.ang,0,0,1);

    float flash=min(1.0f,e.flashTimer/8.0f);

    // AI type color modifier
    float er=0.9f,eg=0.15f,eb=0.28f;
    switch(e.ai){
        case AI_TRACKER:  er=1.0f; eg=0.5f; eb=0.0f; break; // orange
        case AI_SWARM:    er=0.7f; eg=0.0f; eb=1.0f; break; // purple
        case AI_KAMIKAZE: er=1.0f; eg=0.1f; eb=0.1f; break; // red
        case AI_ZIGZAG:   er=0.0f; eg=0.8f; eb=0.5f; break; // teal
        default: break;
    }

    // Engine glow
    glColor4f(er,eg+0.15f,eb,0.25f);
    for(int r=10;r>=3;r--) mpc(0,20,r);

    // Hull
    glColor3f(er+flash*0.1f,eg*(1-flash*0.5f),eb*(1-flash*0.5f));
    glBegin(GL_POLYGON);
        glVertex2f(0,-32); glVertex2f(-14,-6);
        glVertex2f(-18,18); glVertex2f(0,10);
        glVertex2f(18,18); glVertex2f(14,-6);
    glEnd();

    // Wing detail
    glColor3f(er*0.6f,eg*0.4f,eb*0.5f);
    glBegin(GL_POLYGON);
        glVertex2f(-14,-6);glVertex2f(-18,18);glVertex2f(-24,24);glVertex2f(-8,-2);
    glEnd();
    glBegin(GL_POLYGON);
        glVertex2f(14,-6);glVertex2f(18,18);glVertex2f(24,24);glVertex2f(8,-2);
    glEnd();

    // Core orb
    float pulse=0.5f+0.5f*sinf(fc*0.1f+e.x);
    glColor3f(er,eg+0.3f*pulse,eb*0.5f);
    mpcFill(0,0,7);
    glColor4f(er,eg+0.5f,eb,0.4f*pulse);
    mpc(0,0,10); mpc(0,0,11);

    // HP bar
    if(e.maxHp>1){
        float hfrac=(float)e.hp/e.maxHp;
        int bw=28,bh=3,bx=-(bw/2),by=38;
        glColor4f(0.2f,0.0f,0.0f,0.7f);
        glBegin(GL_QUADS);
            glVertex2f(bx,by);glVertex2f(bx+bw,by);
            glVertex2f(bx+bw,by+bh);glVertex2f(bx,by+bh);
        glEnd();
        glColor3f(er,eg,eb);
        glBegin(GL_QUADS);
            glVertex2f(bx,by);glVertex2f(bx+bw*hfrac,by);
            glVertex2f(bx+bw*hfrac,by+bh);glVertex2f(bx,by+bh);
        glEnd();
    }

    // Wing tips + details
    glColor3f(er*1.1f,eg+0.45f,eb);
    dda(-18,18,-26,28); dda(18,18,26,28);
    bres(-8,12,8,12);

    glPopMatrix();
}

// ─────────────────────────────────────────────────────────────────────────────
//  ENEMY BULLET DRAW
// ─────────────────────────────────────────────────────────────────────────────
void drawEnemyBullet(const EnemyBullet& b){
    int bx=(int)b.x, by=(int)b.y;
    float pulse=0.5f+0.5f*sinf(b.age*0.3f);
    // Glow
    for(int r=8;r>=4;r--){
        float t=(r-4)/4.0f;
        glColor4f(1.0f,0.3f,0.1f,0.08f*(1-t));
        mpc(bx,by,r);
    }
    glColor4f(1.0f,0.5f,0.1f,0.7f); mpcFill(bx,by,3);
    glColor4f(1.0f,0.9f,0.5f,1.0f); mpcFill(bx,by,1);
}

// ─────────────────────────────────────────────────────────────────────────────
//  ASTEROID DRAW
// ─────────────────────────────────────────────────────────────────────────────
void drawAst(const Asteroid& a){
    glPushMatrix();
    glTranslatef(a.x,a.y,0);
    glRotatef(a.rot,0,0,1);
    float flash=min(1.0f,a.flashTimer/8.0f);
    glColor4f(0.7f+flash*0.3f,0.55f+flash*0.2f,0.35f,0.2f);
    glBegin(GL_POLYGON);
    for(int i=0;i<10;i++){
        float ang=i*2*PI/10;
        float rad=(a.r+3)*(0.80f+0.20f*sinf(ang*3+a.rot*0.01f));
        glVertex2f(rad*cosf(ang),rad*sinf(ang));
    }
    glEnd();
    glColor3f(0.50f+flash*0.3f,0.43f,0.33f);
    glBegin(GL_POLYGON);
    for(int i=0;i<10;i++){
        float ang=i*2*PI/10;
        float rad=a.r*(0.75f+0.25f*sinf(ang*3+a.rot*0.01f));
        glVertex2f(rad*cosf(ang),rad*sinf(ang));
    }
    glEnd();
    glColor3f(0.30f,0.25f,0.18f);
    mpc((int)(a.r/3),(int)(a.r/4),(int)(a.r/4));
    mpc(-(int)(a.r/4),-(int)(a.r/3),(int)(a.r/5));
    glColor4f(0.75f,0.7f,0.6f,0.35f);
    for(int i=6;i<=8;i++) mpc(-(int)(a.r/3),(int)(a.r/3),i);
    glPopMatrix();
}

// ─────────────────────────────────────────────────────────────────────────────
//  BULLET DRAW (Plasma Lance — original design preserved + enhanced)
// ─────────────────────────────────────────────────────────────────────────────
void drawBullet(const Bullet& b){
    int bx=(int)b.x, by=(int)b.y;
    float pulse=0.5f+0.5f*sinf(b.age*0.45f);
    float pulse2=0.5f+0.5f*sinf(b.age*0.28f+1.2f);

    if(b.isLaser){
        // Wide laser beam
        for(int i=0;i<6;i++){
            float t=i/5.0f;
            glColor4f(1.0f,0.3f+0.5f*(1-t),0.8f*(1-t),0.15f*(1-t));
            dda(bx-4+i,by,bx-4+i,by+H);
        }
        glColor4f(1.0f,0.6f,1.0f,0.8f);
        dda(bx,by,bx,by+H);
        glColor4f(1.0f,1.0f,1.0f,1.0f);
        dda(bx,by+5,bx,by+H-5);
        return;
    }

    // Homing indicator
    if(b.isHoming){
        glColor4f(0.2f,1.0f,0.4f,0.3f*pulse);
        mpc(bx,by,10); mpc(bx,by,11);
    }

    // Energy trail
    for(int seg=0;seg<10;seg++){
        float t=seg/9.0f;
        float alp=(1.0f-t)*0.10f*(0.7f+0.3f*pulse);
        int   rad=(int)(6-t*4);
        int   ty=by-4-seg*7;
        glColor4f(0.0f,0.85f,1.0f,alp);
        if(rad>0) mpc(bx,ty,rad);
    }
    for(int seg=0;seg<9;seg++){
        float t=seg/8.0f;
        float alp=(1.0f-t)*0.55f;
        int ty=by-2-seg*6;
        glColor4f(0.15f,0.65f+0.2f*pulse,1.0f,alp);
        dda(bx,ty,bx,ty-5);
    }
    for(int seg=0;seg<8;seg++){
        float t=seg/7.0f;
        float alp=(1.0f-t)*0.80f;
        int ty=by-seg*5;
        glColor4f(0.6f,0.95f,1.0f,alp);
        dda(bx,ty,bx,ty-4);
    }

    // Side sparks
    for(int i=0;i<5;i++){
        float phase=b.age*0.5f+i*1.3f;
        float sp=sinf(phase);
        if(fabsf(sp)<0.3f) continue;
        int oy=by-6-i*8;
        int len=(int)(3+2*fabsf(sp));
        float alp=fabsf(sp)*0.7f;
        glColor4f(0.3f,0.9f,1.0f,alp);
        dda(bx-len,oy,bx-1,oy); dda(bx+1,oy,bx+len,oy);
    }

    // Main bolt body
    for(int r=11;r>=6;r--){
        float t=(r-6)/5.0f;
        glColor4f(0.0f,0.7f+0.2f*pulse,1.0f,0.06f*(1-t)*(0.8f+0.2f*pulse));
        mpc(bx,by,r);
    }
    for(int r=6;r>=3;r--){
        glColor4f(0.15f,0.9f,1.0f,0.14f*(r/6.0f)*pulse2);
        mpc(bx,by,r);
    }
    glColor4f(0.2f,0.95f,1.0f,0.9f);
    dda(bx-1,by-2,bx-1,by+18); dda(bx+1,by-2,bx+1,by+18);
    glColor4f(0.5f,1.0f,1.0f,1.0f);
    dda(bx,by,bx,by+20);
    glColor4f(1.0f,1.0f,1.0f,1.0f);
    dda(bx,by+2,bx,by+16);

    // Crystalline tip
    int tx=bx, ty2=by+22;
    for(int r=9;r>=4;r--){
        float t=(r-4)/5.0f;
        glColor4f(0.0f,0.8f+0.2f*pulse,1.0f,0.18f*(1-t)*pulse);
        mpc(tx,ty2,r);
    }
    glColor4f(0.4f,1.0f,1.0f,0.85f+0.15f*pulse);
    mpcFill(tx,ty2,4);
    glColor4f(0.7f,1.0f,1.0f,0.9f);
    dda(tx,ty2+7,tx-4,ty2+1); dda(tx,ty2+7,tx+4,ty2+1);
    dda(tx-4,ty2+1,tx,ty2-4); dda(tx+4,ty2+1,tx,ty2-4);
    glColor4f(1.0f,1.0f,1.0f,1.0f); mpcFill(tx,ty2+6,2);

    // Tip flare
    float fl=0.4f+0.6f*pulse;
    glColor4f(0.3f,0.95f,1.0f,fl*0.7f);
    dda(tx-6,ty2+6,tx+6,ty2+6);
    glColor4f(0.5f,1.0f,1.0f,fl*0.5f);
    dda(tx-4,ty2+9,tx+4,ty2+3); dda(tx-4,ty2+3,tx+4,ty2+9);

    // Fins
    glColor4f(0.2f,0.85f,1.0f,0.5f+0.3f*pulse2);
    dda(bx-1,by+12,bx-5,by+6); dda(bx+1,by+12,bx+5,by+6);
    glColor4f(0.5f,1.0f,1.0f,0.35f);
    dda(bx-1,by+14,bx-7,by+8); dda(bx+1,by+14,bx+7,by+8);

    // Base ring
    glColor4f(0.1f,0.7f,1.0f,0.3f*pulse);  mpc(bx,by-2,5);
    glColor4f(0.3f,0.9f,1.0f,0.15f*pulse); mpc(bx,by-2,7);
}

// ─────────────────────────────────────────────────────────────────────────────
//  POWER-UP DRAW
// ─────────────────────────────────────────────────────────────────────────────
void drawPowerUp(const PowerUp& p){
    if(!p.on) return;
    int px=(int)p.x, py=(int)(p.y+sinf(p.bob)*5.0f);
    float pulse=0.5f+0.5f*sinf(p.animT*0.15f);

    // Color per type
    float r,g,b;
    string label;
    switch(p.type){
        case PU_RAPIDFIRE: r=1.0f; g=0.8f; b=0.1f; label="RF"; break;
        case PU_SHIELD:    r=0.1f; g=0.8f; b=1.0f; label="SH"; break;
        case PU_SLOWMO:    r=0.5f; g=0.1f; b=1.0f; label="SM"; break;
        case PU_LASER:     r=1.0f; g=0.2f; b=0.9f; label="LZ"; break;
        case PU_HOMING:    r=0.2f; g=1.0f; b=0.4f; label="HM"; break;
        case PU_BOMB:      r=1.0f; g=0.3f; b=0.1f; label="BM"; break;
    }

    // Outer glow ring (pulsing)
    for(int rad=22;rad>=16;rad--){
        float t=(rad-16)/6.0f;
        glColor4f(r,g,b,0.12f*(1-t)*pulse);
        mpc(px,py,rad);
    }

    // Hexagon body (6 bres lines)
    glColor4f(r,g,b,0.3f+0.2f*pulse);
    mpcFill(px,py,12);
    glColor4f(r,g,b,0.8f+0.2f*pulse);
    for(int i=0;i<6;i++){
        float a1=i*60*PI/180+p.animT*0.02f;
        float a2=(i+1)*60*PI/180+p.animT*0.02f;
        bres(px+(int)(14*cosf(a1)),py+(int)(14*sinf(a1)),
             px+(int)(14*cosf(a2)),py+(int)(14*sinf(a2)));
    }

    // Inner bright core
    glColor4f(r+0.3f,g+0.2f,b+0.1f,1.0f);
    mpcFill(px,py,5);

    // Label
    glColor4f(1.0f,1.0f,1.0f,0.9f);
    txtC(px,py-3,label,GLUT_BITMAP_HELVETICA_12);
}

// ─────────────────────────────────────────────────────────────────────────────
//  BOSS DRAW
// ─────────────────────────────────────────────────────────────────────────────
void drawBoss(){
    if(!boss.on) return;
    int bx=(int)boss.x, by=(int)boss.y;
    float flash=min(1.0f,boss.flashTimer/10.0f);
    float pulse=0.5f+0.5f*sinf(fc*0.08f);
    float hfrac=(float)boss.hp/boss.maxHp;

    // Phase-dependent color
    float cr=0.9f,cg=0.1f,cb=0.3f;
    if(boss.phase==BOSS_PHASE2){ cr=1.0f; cg=0.4f; cb=0.0f; }
    if(boss.phase==BOSS_PHASE3){ cr=1.0f; cg=0.0f; cb=1.0f; }

    // Outer aura bloom (pulsing, larger in phase 3)
    int auraBase=boss.enraged?70:55;
    for(int r=auraBase;r>=auraBase-15;r--){
        float t=(r-(auraBase-15))/15.0f;
        glColor4f(cr,cg,cb,0.05f*t*pulse);
        mpc(bx,by,r);
    }

    // Hull — large hexagonal fortress shape
    glColor3f(cr*(0.4f+flash*0.3f), cg*0.5f, cb*(0.4f+flash*0.3f));
    glBegin(GL_POLYGON);
    for(int i=0;i<6;i++){
        float ang=i*60*PI/180+boss.ang*PI/180;
        glVertex2f(bx+48*cosf(ang),by+48*sinf(ang));
    }
    glEnd();

    // Inner hull ring
    glColor3f(cr*0.7f,cg*0.3f,cb*0.7f);
    glBegin(GL_POLYGON);
    for(int i=0;i<6;i++){
        float ang=i*60*PI/180-boss.ang*PI/180*0.5f;
        glVertex2f(bx+30*cosf(ang),by+30*sinf(ang));
    }
    glEnd();

    // Armor spines (radiating lines using DDA)
    glColor4f(cr,cg,cb,0.6f+0.3f*pulse);
    for(int i=0;i<6;i++){
        float ang=i*60*PI/180+boss.ang*PI/180;
        dda(bx+(int)(30*cosf(ang)),by+(int)(30*sinf(ang)),
            bx+(int)(60*cosf(ang)),by+(int)(60*sinf(ang)));
    }

    // Core power reactor
    glColor3f(cr,cg+0.3f*pulse,cb);
    mpcFill(bx,by,14);
    glColor4f(1.0f,1.0f,1.0f,0.7f+0.3f*pulse);
    mpcFill(bx,by,7);
    // Reactor rings
    glColor4f(cr,cg+0.5f,cb,0.5f*pulse);
    mpc(bx,by,18); mpc(bx,by,19);

    // Orbital shield orbs
    for(int i=0;i<4;i++){
        if(!boss.orbOn[i]) continue;
        int ox=bx+(int)(38*cosf(boss.orbAng[i]));
        int oy=by+(int)(38*sinf(boss.orbAng[i]));
        glColor4f(cr,cg+0.3f,cb,0.8f);
        mpcFill(ox,oy,5);
        glColor4f(cr,cg+0.6f,cb,0.4f*pulse);
        mpc(ox,oy,8); mpc(ox,oy,9);
    }

    // Health bar (prominent, at top of boss)
    int bw=200, bh=8;
    int barX=bx-bw/2, barY=by+72;
    glColor4f(0.15f,0.0f,0.0f,0.85f);
    glBegin(GL_QUADS);
        glVertex2f(barX,barY); glVertex2f(barX+bw,barY);
        glVertex2f(barX+bw,barY+bh); glVertex2f(barX,barY+bh);
    glEnd();
    // Fill — changes color by phase
    glColor3f(cr,cg,cb);
    glBegin(GL_QUADS);
        glVertex2f(barX,barY); glVertex2f(barX+bw*hfrac,barY);
        glVertex2f(barX+bw*hfrac,barY+bh); glVertex2f(barX,barY+bh);
    glEnd();
    glColor4f(cr+0.2f,cg+0.2f,cb+0.2f,0.7f);
    bres(barX,barY,barX+bw,barY);
    bres(barX,barY+bh,barX+bw,barY+bh);
    // "BOSS" label
    glColor3f(cr,cg+0.5f,cb);
    txtC(bx,barY+12,"BOSS",GLUT_BITMAP_HELVETICA_12);
    // Phase text
    string phaseStr="PHASE ";
    if(boss.phase==BOSS_PHASE1) phaseStr+="I";
    else if(boss.phase==BOSS_PHASE2) phaseStr+="II";
    else phaseStr+="III !!";
    txtC(bx,barY+24,phaseStr,GLUT_BITMAP_HELVETICA_12);

    // Phase 3 extra: spinning outer ring
    if(boss.phase==BOSS_PHASE3){
        glColor4f(1.0f,0.0f,1.0f,0.4f+0.3f*pulse);
        for(int i=0;i<8;i++){
            float ang=i*PI/4+boss.ang*PI/90;
            int ox=bx+(int)(70*cosf(ang));
            int oy=by+(int)(70*sinf(ang));
            mpc(ox,oy,4);
            // Connect with dda
            int ox2=bx+(int)(70*cosf(ang+PI/4));
            int oy2=by+(int)(70*sinf(ang+PI/4));
            dda(ox,oy,ox2,oy2);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  EXPLOSIONS DRAW
// ─────────────────────────────────────────────────────────────────────────────
void drawExps(){
    for(auto& ex:exps){
        if(!ex.on) continue;
        glPushMatrix();
        glTranslatef(ex.x,ex.y,0);
        glScalef(ex.scale,ex.scale,1);
        glColor4f(ex.r,ex.g,ex.b,ex.alpha*0.35f);  mpc(0,0,32);
        glColor4f(ex.r,ex.g+0.3f,ex.b,ex.alpha*0.55f); mpc(0,0,20);
        glColor4f(1.0f,0.9f,0.5f,ex.alpha*0.75f);   mpcFill(0,0,11);
        glColor4f(1,1,1,ex.alpha);                    mpcFill(0,0,5);
        glPopMatrix();
        for(auto& p:ex.pts){
            if(!p.on) continue;
            glColor4f(p.r,p.g,p.b,p.a*p.life/p.maxLife);
            glPointSize(p.sz);
            glBegin(GL_POINTS); glVertex2f(p.x,p.y); glEnd();
        }
    }
    glPointSize(1);
}

// ─────────────────────────────────────────────────────────────────────────────
//  HIT EFFECT — flash ring on impact
// ─────────────────────────────────────────────────────────────────────────────
struct HitRing { float x, y, r, alpha; bool on; float cr, cg, cb; };
HitRing hitRings[16];
int hitRingIdx=0;

void spawnHitRing(float x,float y,float cr=1.0f,float cg=0.8f,float cb=0.2f){
    hitRings[hitRingIdx%16]={x,y,4,1.0f,true,cr,cg,cb};
    hitRingIdx++;
}

void updateDrawHitRings(){
    for(auto& h:hitRings){
        if(!h.on) continue;
        h.r+=3.5f; h.alpha-=0.06f;
        if(h.alpha<=0){ h.on=false; continue; }
        glColor4f(h.cr,h.cg,h.cb,h.alpha);
        mpc((int)h.x,(int)h.y,(int)h.r);
        mpc((int)h.x,(int)h.y,(int)h.r+1);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  MINI-MAP / RADAR
// ─────────────────────────────────────────────────────────────────────────────
void drawRadar(){
    if(!showRadar) return;
    int rx=W-70, ry=60, rw=55, rh=55;

    // Panel bg
    glColor4f(0.0f,0.05f,0.12f,0.75f);
    glBegin(GL_QUADS);
        glVertex2f(rx-rw/2,ry-rh/2); glVertex2f(rx+rw/2,ry-rh/2);
        glVertex2f(rx+rw/2,ry+rh/2); glVertex2f(rx-rw/2,ry+rh/2);
    glEnd();
    glColor4f(0.2f,0.5f,0.9f,0.5f);
    bres(rx-rw/2,ry-rh/2,rx+rw/2,ry-rh/2);
    bres(rx-rw/2,ry+rh/2,rx+rw/2,ry+rh/2);
    bres(rx-rw/2,ry-rh/2,rx-rw/2,ry+rh/2);
    bres(rx+rw/2,ry-rh/2,rx+rw/2,ry+rh/2);

    // Radar title
    glColor4f(0.3f,0.6f,1.0f,0.7f);
    txt(rx-rw/2+2,ry+rh/2-10,"RADAR",GLUT_BITMAP_HELVETICA_12);

    // Player dot
    float npx=rx-rw/2+2+(sx/W)*(rw-4);
    float npy=ry-rh/2+2+(sy/H)*(rh-4);
    glColor3f(0.3f,0.9f,1.0f);
    mpcFill((int)npx,(int)npy,2);

    // Enemies
    for(auto& e:ene){
        if(!e.on) continue;
        float ex2=rx-rw/2+2+(e.x/W)*(rw-4);
        float ey2=ry-rh/2+2+(e.y/H)*(rh-4);
        glColor3f(1.0f,0.2f,0.3f);
        mpcFill((int)ex2,(int)ey2,1);
    }

    // Boss
    if(boss.on){
        float ex2=rx-rw/2+2+(boss.x/W)*(rw-4);
        float ey2=ry-rh/2+2+(boss.y/H)*(rh-4);
        float pulse=0.5f+0.5f*sinf(fc*0.12f);
        glColor4f(1.0f,0.3f,1.0f,0.7f+0.3f*pulse);
        mpcFill((int)ex2,(int)ey2,3);
    }

    // Asteroids
    for(auto& a:ast){
        if(!a.on) continue;
        float ax2=rx-rw/2+2+(a.x/W)*(rw-4);
        float ay2=ry-rh/2+2+(a.y/H)*(rh-4);
        glColor3f(0.7f,0.5f,0.3f);
        glBegin(GL_POINTS); glVertex2f(ax2,ay2); glEnd();
    }

    // Sweep line (rotating)
    float sweepAng=fc*0.03f;
    glColor4f(0.3f,1.0f,0.5f,0.35f);
    dda(rx,ry,rx+(int)((rw/2-2)*cosf(sweepAng)),ry+(int)((rh/2-2)*sinf(sweepAng)));
}

// ─────────────────────────────────────────────────────────────────────────────
//  COMBO DISPLAY
// ─────────────────────────────────────────────────────────────────────────────
void drawCombo(){
    if(comboCount<2) return;
    float t=min(1.0f,(float)comboTimer/60.0f);
    float pop=1.0f+comboPop*0.4f;
    float a=t;
    // Background tag
    glColor4f(0.8f,0.5f,0.0f,0.3f*a);
    glBegin(GL_QUADS);
        glVertex2f(W/2-65,H/2+90); glVertex2f(W/2+65,H/2+90);
        glVertex2f(W/2+65,H/2+115); glVertex2f(W/2-65,H/2+115);
    glEnd();
    // Combo text wave
    string comboStr=to_string(comboCount)+"x COMBO!";
    float phase=fc*0.15f;
    float sr=1.0f,sg=0.8f,sb=0.1f;
    if(comboCount>=5){ sr=1.0f; sg=0.3f; sb=1.0f; } // purple for 5+
    if(comboCount>=10){ sr=1.0f; sg=0.1f; sb=0.1f; } // red for 10+
    glPushMatrix();
    glTranslatef(W/2,H/2+105,0);
    glScalef(pop,pop,1);
    glTranslatef(-W/2,-H/2-105,0);
    txtWave(W/2,H/2+105,comboStr,sr,sg,sb*a,3.0f,phase,GLUT_BITMAP_HELVETICA_18);
    glPopMatrix();
}

// ─────────────────────────────────────────────────────────────────────────────
//  HUD
// ─────────────────────────────────────────────────────────────────────────────
void drawHUD(){
    // TOP BAR
    glColor4f(0.0f,0.04f,0.12f,0.88f);
    glBegin(GL_QUADS);
        glVertex2f(0,H); glVertex2f(W,H);
        glVertex2f(W,H-52); glVertex2f(0,H-52);
    glEnd();
    glColor4f(0.1f,0.5f,1.0f,0.7f); bres(0,H-52,W,H-52);
    glColor4f(0.2f,0.7f,1.0f,0.3f); bres(0,H-51,W,H-51);

    // SCORE
    glColor3f(0.3f,0.6f,0.9f);
    txt(18,H-18,"SCORE",GLUT_BITMAP_HELVETICA_12);
    float sr=0.35f+scoreAnim*0.4f,sg=1.0f,sb=0.75f;
    glColor3f(sr,sg,sb);
    txt(18,H-38,to_string(score),GLUT_BITMAP_TIMES_ROMAN_24);

    // HIGH SCORE
    glColor4f(0.5f,0.8f,0.4f,0.7f);
    txt(18,H-50,"HI:"+to_string(highScore),GLUT_BITMAP_HELVETICA_12);

    // Divider
    glColor4f(0.2f,0.45f,0.8f,0.4f); bres(200,H-8,200,H-48);

    // WAVE / ZONE center
    float wf=min(1.0f,waveFlash);
    glColor4f(0.05f+wf*0.3f,0.15f+wf*0.2f,0.35f+wf*0.2f,0.5f+wf*0.3f);
    glBegin(GL_QUADS);
        glVertex2f(W/2-80,H-8);  glVertex2f(W/2+80,H-8);
        glVertex2f(W/2+80,H-52); glVertex2f(W/2-80,H-52);
    glEnd();
    glColor4f(0.2f+wf*0.5f,0.6f+wf*0.3f,1.0f,0.6f+wf*0.35f);
    bres(W/2-80,H-8,W/2+80,H-8); bres(W/2-80,H-52,W/2+80,H-52);
    bres(W/2-80,H-8,W/2-80,H-52); bres(W/2+80,H-8,W/2+80,H-52);
    glColor3f(0.5f,0.75f,1.0f);
    txtC(W/2,H-20,"WAVE",GLUT_BITMAP_HELVETICA_12);
    glColor3f(0.7f+wf*0.3f,0.9f,1.0f);
    txtC(W/2,H-38,to_string(wave),GLUT_BITMAP_TIMES_ROMAN_24);
    // Zone name
    string zoneName;
    switch(currentZone){
        case ZONE_SPACE:    zoneName="DEEP SPACE"; break;
        case ZONE_ASTEROID: zoneName="ASTEROID BELT"; break;
        case ZONE_NEBULA:   zoneName="NEBULA ZONE"; break;
        case ZONE_BOSS:     zoneName="BOSS ARENA"; break;
    }
    glColor4f(0.6f,0.8f,1.0f,0.5f);
    txtC(W/2,H-50,zoneName,GLUT_BITMAP_HELVETICA_12);

    // LIVES
    glColor4f(0.2f,0.45f,0.8f,0.4f); bres(W-220,H-8,W-220,H-48);
    glColor3f(0.4f,0.65f,1.0f);
    txt(W-215,H-18,"LIVES",GLUT_BITMAP_HELVETICA_12);
    for(int i=0;i<3;i++){
        float hx=W-210.0f+i*36, hy=H-40;
        if(i<lives){
            glColor3f(0.0f,0.7f,1.0f);
            glBegin(GL_TRIANGLES);
                glVertex2f(hx,hy+12); glVertex2f(hx-10,hy-6); glVertex2f(hx+10,hy-6);
            glEnd();
            glColor4f(0.0f,0.8f,1.0f,0.3f);
            for(int r=12;r<=15;r++) mpc((int)hx,(int)(hy+2),r);
        } else {
            glColor4f(0.2f,0.3f,0.5f,0.5f);
            glBegin(GL_TRIANGLES);
                glVertex2f(hx,hy+12); glVertex2f(hx-10,hy-6); glVertex2f(hx+10,hy-6);
            glEnd();
        }
    }

    // BOTTOM STATUS BAR
    glColor4f(0.0f,0.03f,0.10f,0.75f);
    glBegin(GL_QUADS);
        glVertex2f(0,0); glVertex2f(W,0);
        glVertex2f(W,28); glVertex2f(0,28);
    glEnd();
    glColor4f(0.1f,0.4f,0.9f,0.5f); bres(0,28,W,28);

    // Shield status
    if(shieldCool>0){
        float prog=1.0f-shieldCool/300.0f;
        int bw=140;
        glColor4f(0.1f,0.15f,0.35f,0.8f);
        glBegin(GL_QUADS);
            glVertex2f(20,4);glVertex2f(20+bw,4);
            glVertex2f(20+bw,20);glVertex2f(20,20);
        glEnd();
        glColor3f(0.25f,0.4f,0.8f);
        glBegin(GL_QUADS);
            glVertex2f(20,4);glVertex2f(20+bw*prog,4);
            glVertex2f(20+bw*prog,20);glVertex2f(20,20);
        glEnd();
        glColor4f(0.4f,0.5f,1.0f,0.7f);
        txt(22,8,"SHIELD",GLUT_BITMAP_HELVETICA_12);
    } else {
        float shp=0.6f+0.4f*sinf(fc*0.07f);
        glColor3f(0.2f,0.85f*shp,0.9f*shp);
        txt(10,8,"[S] SHIELD READY",GLUT_BITMAP_HELVETICA_12);
    }

    // Active power-up icons
    int iconX=180;
    auto drawPUIcon=[&](int cd, float r, float g, float b, const string& lbl){
        if(cd<=0) return;
        float prog=min(1.0f,(float)cd/180.0f);
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

    // Controls hint right
    glColor4f(0.3f,0.45f,0.65f,0.6f);
    txt(W-280,8,"ENTER:FIRE  S:SHIELD  P:PAUSE",GLUT_BITMAP_HELVETICA_12);

    // Corner brackets
    glColor4f(0.3f,0.6f,1.0f,0.5f);
    bres(0,H,18,H); bres(0,H,0,H-18);
    bres(W,H,W-18,H); bres(W,H,W,H-18);
    bres(0,28,18,28); bres(0,28,0,46);
    bres(W,28,W-18,28); bres(W,28,W,46);
}

// ─────────────────────────────────────────────────────────────────────────────
//  SLOW-MO VIGNETTE
// ─────────────────────────────────────────────────────────────────────────────
void drawSlowMoOverlay(){
    if(gameSpeed>=0.95f) return;
    float intensity=(1.0f-gameSpeed)*0.6f;
    // Edge vignette (dark purple tint)
    for(int i=0;i<6;i++){
        float t=i/5.0f;
        glColor4f(0.2f,0.0f,0.4f,intensity*0.15f*(1-t));
        // Draw as nested quads shrinking inward
        int m=i*15;
        glBegin(GL_LINE_LOOP);
            glVertex2f(m,m); glVertex2f(W-m,m);
            glVertex2f(W-m,H-m); glVertex2f(m,H-m);
        glEnd();
    }
    // "SLOW-MO" text flicker
    if((fc/8)%2==0){
        glColor4f(0.6f,0.3f,1.0f,0.7f);
        txtC(W/2,H/2+150,"— SLOW MOTION —",GLUT_BITMAP_HELVETICA_12);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  SPLASH SCREEN
// ─────────────────────────────────────────────────────────────────────────────
void drawSplash(){
    glClear(GL_COLOR_BUFFER_BIT);
    // Deep black bg
    glColor3f(0,0,0.02f);
    glBegin(GL_QUADS);
        glVertex2f(0,0);glVertex2f(W,0);glVertex2f(W,H);glVertex2f(0,H);
    glEnd();

    float t=splashT/180.0f;  // 0→1

    // Stars fade in
    if(t>0.15f){
        float a=(t-0.15f)/0.3f;
        for(auto& s:stars){
            float tw=s.bright*min(1.0f,a);
            glColor3f(tw,tw,tw*1.1f);
            glPointSize(s.sz);
            glBegin(GL_POINTS); glVertex2f(s.x,s.y); glEnd();
        }
        glPointSize(1);
    }

    // Logo "P&S" large glow text
    if(t>0.3f){
        float a=clamp((t-0.3f)/0.25f,0,1);
        float glow=a*(0.5f+0.5f*sinf(splashT*0.1f));

        // Glow rings behind logo
        for(int r=80;r>=40;r-=4){
            float rt=(r-40)/40.0f;
            glColor4f(0.2f,0.5f,1.0f,glow*0.04f*(1-rt));
            mpc(W/2,(int)(H*0.6f),r);
        }

        // Draw ship preview (scaled up)
        float sc=1.5f*a;
        drawShip(W/2,(int)(H*0.58f),0,sc);

        // Title with glow
        txtGlow(W/2,(int)(H*0.75f),"PARTH & SPACE",0.4f,0.8f,1.0f,glow);
    }

    // Subtitle slide up
    if(t>0.55f){
        float a=clamp((t-0.55f)/0.2f,0,1);
        float yOff=(1-a)*25;
        glColor4f(0.6f,0.75f,0.9f,a);
        txtC(W/2,(int)(H*0.68f)-yOff,"ULTIMATE EDITION",GLUT_BITMAP_HELVETICA_18);
    }

    // Author
    if(t>0.7f){
        float a=clamp((t-0.7f)/0.15f,0,1);
        glColor4f(0.35f,0.45f,0.65f,a*0.7f);
        txtC(W/2,(int)(H*0.62f),"by Shuvo Singh Partho",GLUT_BITMAP_HELVETICA_12);
    }

    // "Press any key" blink
    if(t>0.85f){
        float a=0.5f+0.5f*sinf(splashT*0.15f);
        glColor4f(0.5f,0.8f,1.0f,a);
        txtC(W/2,(int)(H*0.2f),"Press any key to continue...",GLUT_BITMAP_HELVETICA_18);
    }

    // Fade-out at end
    if(t>0.92f){
        float fa=clamp((t-0.92f)/0.08f,0,1);
        glColor4f(0,0,0,fa);
        glBegin(GL_QUADS);
            glVertex2f(0,0);glVertex2f(W,0);glVertex2f(W,H);glVertex2f(0,H);
        glEnd();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  MENU SCREEN
// ─────────────────────────────────────────────────────────────────────────────
void drawMenu(){
    drawBackground();
    drawNebula();
    drawStars();
    drawRings();
    drawPlanet();

    drawShip(W/2,170,0,1.3f);

    float gp=0.5f+0.5f*sinf(menuAnim*0.05f);

    // Title panel
    glColor4f(0.02f,0.06f,0.18f,0.82f);
    glBegin(GL_QUADS);
        glVertex2f(140,470);glVertex2f(760,470);
        glVertex2f(760,535);glVertex2f(140,535);
    glEnd();
    glColor4f(0.15f+0.15f*gp,0.55f+0.15f*gp,1.0f,0.65f+0.2f*gp);
    bres(140,470,760,470); bres(140,535,760,535);
    bres(140,470,140,535); bres(760,470,760,535);
    // Corner accents
    glColor4f(0.5f,0.85f,1.0f,0.8f);
    bres(140,535,165,535); bres(140,535,140,510);
    bres(760,535,735,535); bres(760,535,760,510);
    bres(140,470,165,470); bres(140,470,140,497);
    bres(760,470,735,470); bres(760,470,760,497);

    // Title — wave effect
    txtWave(W/2,518,"PARTH & SPACE",0.35f+0.2f*gp,0.8f+0.15f*gp,1.0f,
            3.0f,menuAnim*0.06f,GLUT_BITMAP_TIMES_ROMAN_24);
    glColor3f(0.5f,0.7f,0.9f);
    txtC(W/2,478,"ULTIMATE EDITION  —  2D SPACE SHOOTER",GLUT_BITMAP_HELVETICA_12);

    // High score
    glColor4f(0.8f,0.7f,0.2f,0.8f);
    txtC(W/2,462,"HIGH SCORE: "+to_string(highScore),GLUT_BITMAP_HELVETICA_18);

    // ENTER button
    float ep=0.5f+0.5f*sinf(menuAnim*0.09f);
    glColor4f(0.05f,0.2f+0.1f*ep,0.5f+0.15f*ep,0.7f+0.15f*ep);
    glBegin(GL_QUADS);
        glVertex2f(310,415);glVertex2f(590,415);
        glVertex2f(590,445);glVertex2f(310,445);
    glEnd();
    glColor4f(0.25f,0.7f+0.2f*ep,1.0f,0.8f+0.15f*ep);
    bres(310,415,590,415); bres(310,445,590,445);
    bres(310,415,310,445); bres(590,415,590,445);
    glColor3f(0.5f,0.9f+0.1f*ep,1.0f);
    txtC(W/2,425,"PRESS  SPACE  TO LAUNCH",GLUT_BITMAP_HELVETICA_18);

    // Settings button
    glColor4f(0.04f,0.12f,0.28f,0.75f);
    glBegin(GL_QUADS);
        glVertex2f(350,382);glVertex2f(550,382);
        glVertex2f(550,408);glVertex2f(350,408);
    glEnd();
    glColor4f(0.2f,0.45f,0.8f,0.6f);
    bres(350,382,550,382); bres(350,408,550,408);
    bres(350,382,350,408); bres(550,382,550,408);
    glColor3f(0.45f,0.7f,1.0f);
    txtC(W/2,389,"[ TAB ]  SETTINGS",GLUT_BITMAP_HELVETICA_12);

    // Controls box
    glColor4f(0.01f,0.04f,0.12f,0.75f);
    glBegin(GL_QUADS);
        glVertex2f(200,280);glVertex2f(700,280);
        glVertex2f(700,372);glVertex2f(200,372);
    glEnd();
    glColor4f(0.15f,0.35f,0.7f,0.45f);
    bres(200,280,700,280); bres(200,372,700,372);
    bres(200,280,200,372); bres(700,280,700,372);

    glColor3f(0.55f,0.75f,1.0f);
    txtC(W/2,360,"CONTROLS",GLUT_BITMAP_HELVETICA_12);
    glColor3f(0.6f,0.75f,0.9f);
    txtC(W/2,344,"WASD / Arrow Keys  —  Move",GLUT_BITMAP_HELVETICA_12);
    txtC(W/2,328,"ENTER  —  Fire    S  —  Shield    P  —  Pause    R  —  Restart",GLUT_BITMAP_HELVETICA_12);
    txtC(W/2,312,"Collect glowing power-ups for upgrades!",GLUT_BITMAP_HELVETICA_12);
    txtC(W/2,296,"Survive waves, defeat the boss every 5 waves",GLUT_BITMAP_HELVETICA_12);

    glColor4f(0.3f,0.4f,0.6f,0.5f);
    txtC(W/2,266,"Author: Shuvo Singh Partho  |  OpenGL / FreeGLUT  |  C++",GLUT_BITMAP_HELVETICA_12);
}

// ─────────────────────────────────────────────────────────────────────────────
//  SETTINGS SCREEN
// ─────────────────────────────────────────────────────────────────────────────
void drawSettings(){
    drawBackground();
    drawStars();

    float gp=0.5f+0.5f*sinf(settingsAnim*0.06f);

    // Panel
    glColor4f(0.02f,0.05f,0.15f,0.92f);
    glBegin(GL_QUADS);
        glVertex2f(230,180);glVertex2f(670,180);
        glVertex2f(670,540);glVertex2f(230,540);
    glEnd();
    glColor4f(0.2f,0.5f,1.0f,0.6f+0.2f*gp);
    bres(230,180,670,180); bres(230,540,670,540);
    bres(230,180,230,540); bres(670,180,670,540);

    // Title
    glColor3f(0.5f,0.8f,1.0f);
    txtC(W/2,520,"SETTINGS",GLUT_BITMAP_TIMES_ROMAN_24);
    glColor4f(0.3f,0.5f,0.8f,0.5f);
    bres(260,505,640,505);

    // Option rows
    struct Opt { string label; string value; };
    vector<Opt> opts={
        {"DIFFICULTY", difficulty==0?"EASY":difficulty==1?"NORMAL":"HARD"},
        {"RADAR",      showRadar?"ON":"OFF"},
        {"TRAILS",     showTrails?"ON":"OFF"},
        {"[BACK]",     "TAB / ESC"}
    };

    for(int i=0;i<(int)opts.size();i++){
        float oy=480-i*55.0f;
        bool sel=(i==settingsSel);
        // Row highlight
        if(sel){
            float h=0.5f+0.5f*sinf(settingsAnim*0.12f);
            glColor4f(0.1f,0.3f+0.2f*h,0.6f+0.1f*h,0.4f+0.2f*h);
            glBegin(GL_QUADS);
                glVertex2f(245,oy-5);glVertex2f(655,oy-5);
                glVertex2f(655,oy+22);glVertex2f(245,oy+22);
            glEnd();
            glColor4f(0.3f,0.7f+0.2f*h,1.0f,0.7f);
            bres(245,oy-5,655,oy-5); bres(245,oy+22,655,oy+22);
        }
        // Label
        glColor3f(sel?1.0f:0.6f, sel?1.0f:0.75f, sel?1.0f:0.9f);
        txt(265,oy,opts[i].label,GLUT_BITMAP_HELVETICA_18);
        // Value
        glColor3f(sel?0.4f:0.3f, sel?1.0f:0.7f, sel?0.6f:0.5f);
        float vw=strWidth(opts[i].value,GLUT_BITMAP_HELVETICA_18);
        txt(635-vw,oy,opts[i].value,GLUT_BITMAP_HELVETICA_18);
        // Arrows for selected
        if(sel && i<3){
            glColor4f(0.5f,0.9f,1.0f,0.7f);
            txt(637,oy,">",GLUT_BITMAP_HELVETICA_18);
            txt(251,oy,"<",GLUT_BITMAP_HELVETICA_18);
        }
    }

    glColor4f(0.3f,0.5f,0.7f,0.55f);
    txtC(W/2,200,"UP/DOWN: Select    LEFT/RIGHT: Change    TAB: Back",GLUT_BITMAP_HELVETICA_12);
}

// ─────────────────────────────────────────────────────────────────────────────
//  PAUSE OVERLAY
// ─────────────────────────────────────────────────────────────────────────────
void drawPauseOverlay(){
    glColor4f(0,0,0.04f,0.62f);
    glBegin(GL_QUADS);
        glVertex2f(0,0);glVertex2f(W,0);glVertex2f(W,H);glVertex2f(0,H);
    glEnd();
    glColor4f(0.02f,0.06f,0.16f,0.92f);
    glBegin(GL_QUADS);
        glVertex2f(300,290);glVertex2f(600,290);
        glVertex2f(600,430);glVertex2f(300,430);
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

// ─────────────────────────────────────────────────────────────────────────────
//  GAME OVER SCREEN
// ─────────────────────────────────────────────────────────────────────────────
void drawOver(){
    glColor4f(0.04f,0.0f,0.0f,0.80f);
    glBegin(GL_QUADS);
        glVertex2f(0,0);glVertex2f(W,0);glVertex2f(W,H);glVertex2f(0,H);
    glEnd();

    float gp=0.5f+0.5f*sinf(menuAnim*0.07f);

    glColor4f(0.07f,0.02f,0.02f,0.92f);
    glBegin(GL_QUADS);
        glVertex2f(200,240);glVertex2f(700,240);
        glVertex2f(700,490);glVertex2f(200,490);
    glEnd();
    glColor4f(0.9f+gp*0.1f,0.15f,0.25f,0.7f+gp*0.2f);
    bres(200,240,700,240); bres(200,490,700,490);
    bres(200,240,200,490); bres(700,240,700,490);
    glColor3f(1.0f,0.3f,0.4f);
    bres(200,490,225,490); bres(200,490,200,465);
    bres(700,490,675,490); bres(700,490,700,465);
    bres(200,240,225,240); bres(200,240,200,265);
    bres(700,240,675,240); bres(700,240,700,265);

    // Title wave
    txtWave(W/2,468,"GAME  OVER",1.0f,0.2f+gp*0.1f,0.3f,5.0f,menuAnim*0.08f);
    glColor4f(0.7f,0.2f,0.3f,0.45f);
    bres(240,445,660,445);

    // Stats
    glColor3f(0.9f,0.8f,0.3f);
    txtC(W/2,420,"FINAL SCORE",GLUT_BITMAP_HELVETICA_12);
    glColor3f(1.0f,0.9f,0.4f);
    txtC(W/2,400,to_string(score),GLUT_BITMAP_TIMES_ROMAN_24);

    bool newHigh=(score>highScore);
    if(newHigh){
        float np=0.5f+0.5f*sinf(menuAnim*0.12f);
        glColor4f(1.0f,0.9f,0.1f,np);
        txtC(W/2,378,"★  NEW HIGH SCORE!  ★",GLUT_BITMAP_HELVETICA_18);
    } else {
        glColor3f(0.4f,0.55f,0.8f);
        txtC(W/2,378,"Best: "+to_string(highScore),GLUT_BITMAP_HELVETICA_12);
    }

    glColor3f(0.55f,0.7f,0.9f);
    txtC(W/2,355,"Wave Reached: "+to_string(wave),GLUT_BITMAP_HELVETICA_18);

    // Zone
    string zn;
    switch(currentZone){
        case ZONE_SPACE:    zn="Deep Space"; break;
        case ZONE_ASTEROID: zn="Asteroid Belt"; break;
        case ZONE_NEBULA:   zn="Nebula Zone"; break;
        case ZONE_BOSS:     zn="Boss Arena"; break;
    }
    glColor4f(0.45f,0.6f,0.85f,0.7f);
    txtC(W/2,336,"Zone: "+zn,GLUT_BITMAP_HELVETICA_12);

    glColor4f(0.4f,0.3f,0.6f,0.35f);
    bres(240,320,660,320);

    // Buttons
    float ep=0.5f+0.5f*sinf(menuAnim*0.1f);
    glColor4f(0.04f,0.18f+0.06f*ep,0.08f,0.75f);
    glBegin(GL_QUADS);
        glVertex2f(265,260);glVertex2f(445,260);
        glVertex2f(445,305);glVertex2f(265,305);
    glEnd();
    glColor4f(0.15f,0.75f+0.2f*ep,0.35f,0.8f);
    bres(265,260,445,260); bres(265,305,445,305);
    bres(265,260,265,305); bres(445,260,445,305);
    glColor3f(0.3f,1.0f,0.55f);
    txtC(355,277,"[ R ]  Restart",GLUT_BITMAP_HELVETICA_18);

    glColor4f(0.12f,0.05f,0.05f,0.75f);
    glBegin(GL_QUADS);
        glVertex2f(460,260);glVertex2f(640,260);
        glVertex2f(640,305);glVertex2f(460,305);
    glEnd();
    glColor4f(0.7f,0.2f,0.25f,0.7f);
    bres(460,260,640,260); bres(460,305,640,305);
    bres(460,260,460,305); bres(640,260,640,305);
    glColor3f(0.9f,0.45f,0.5f);
    txtC(550,277,"[ ESC ]  Quit",GLUT_BITMAP_HELVETICA_18);
}

// ─────────────────────────────────────────────────────────────────────────────
//  WAVE / BOSS BANNERS
// ─────────────────────────────────────────────────────────────────────────────
void drawWaveBanner(){
    if(waveFlash<=0) return;
    float a=min(1.0f,waveFlash/30.0f);
    if(waveFlash<30) a=waveFlash/30.0f;

    bool isBoss=(currentZone==ZONE_BOSS);
    float br=isBoss?0.8f:0.02f, bg=isBoss?0.0f:0.08f, bb=isBoss?0.05f:0.22f;

    glColor4f(br,bg,bb,0.85f*a);
    glBegin(GL_QUADS);
        glVertex2f(200,H/2-32);glVertex2f(700,H/2-32);
        glVertex2f(700,H/2+32);glVertex2f(200,H/2+32);
    glEnd();
    glColor4f(isBoss?1.0f:0.3f,isBoss?0.2f:0.7f,isBoss?0.3f:1.0f,0.8f*a);
    bres(200,H/2-32,700,H/2-32); bres(200,H/2+32,700,H/2+32);

    string msg=isBoss?"★  BOSS INCOMING!  ★":"WAVE "+to_string(wave)+" INCOMING!";
    float mr=isBoss?1.0f:0.6f, mg=isBoss?0.3f:0.9f, mb=isBoss?0.3f:1.0f;
    glColor4f(mr,mg,mb,a);
    txtC(W/2,H/2+8,msg,GLUT_BITMAP_TIMES_ROMAN_24);
    if(isBoss){
        glColor4f(1.0f,0.6f,0.1f,a*0.8f);
        txtC(W/2,H/2-15,"Survive the onslaught!",GLUT_BITMAP_HELVETICA_12);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  TRANSITION FADE
// ─────────────────────────────────────────────────────────────────────────────
void drawTransitionFade(){
    if(transAlpha<=0) return;
    glColor4f(0,0,0,transAlpha);
    glBegin(GL_QUADS);
        glVertex2f(0,0);glVertex2f(W,0);glVertex2f(W,H);glVertex2f(0,H);
    glEnd();
}

// ─────────────────────────────────────────────────────────────────────────────
//  SPAWN / INIT
// ─────────────────────────────────────────────────────────────────────────────
void spawnExplosion(float x,float y,float intensity,float cr,float cg,float cb){
    Explosion ex{x,y,0.1f,intensity,true,{},cr,cg,cb};
    int numPts=(int)(20+intensity*18);
    for(int i=0;i<numPts;i++){
        float a=frand(0,2*PI), s=frand(1.5f,4.5f+intensity*2);
        float mL2=frand(0.5f,1.0f);
        float sz=frand(1.5f,3.5f+intensity);
        float gr=frand(-0.02f,-0.08f);  // slight gravity
        float fr=frand(0.96f,0.99f);    // friction
        ex.pts.push_back({x,y,s*cosf(a),s*sinf(a),mL2,mL2,
                          cr*frand(0.8f,1.0f),cg*frand(0.4f,0.9f),
                          cb*frand(0,0.4f),intensity,sz,gr,fr,true});
    }
    exps.push_back(ex);
    // Screen shake proportional to intensity
    triggerScreenShake(intensity*4,12);
}

void spawnPowerUp(float x,float y){
    for(auto& p:pups) if(!p.on){
        int t=rand()%6;
        p={x,y,-0.8f,(PowerUpType)t,true,0,frand(0,2*PI)};
        break;
    }
}

void spawnEnemy(){
    // Determine AI type based on wave
    EnemyAI ai;
    int r=rand()%10;
    if(wave<3)       ai=AI_SINE;
    else if(wave<5)  ai=r<6?AI_ZIGZAG:AI_TRACKER;
    else if(wave<8)  ai=r<4?AI_SWARM:(r<7?AI_TRACKER:AI_KAMIKAZE);
    else             ai=r<3?AI_KAMIKAZE:(r<6?AI_SWARM:AI_TRACKER);

    // Difficulty modifier
    float spdMult=(difficulty==0)?0.7f:(difficulty==2)?1.3f:1.0f;

    for(auto& e:ene) if(!e.on){
        e.x=frand(60,W-60); e.y=frand(H+20,H+150);
        e.baseY=e.y;
        e.spd=(frand(0.6f,1.2f)+wave*0.12f)*spdMult;
        e.ang=0; e.flashTimer=0; e.on=true;
        e.maxHp=(wave>=3)?2:(wave>=6)?3:1;
        e.hp=e.maxHp;
        e.ai=ai; e.aiTimer=0;
        e.tx=sx; e.ty=sy;
        e.swarmIdx=(int)(&e-ene);
        break;
    }
}

void spawnAst(){
    for(auto& a:ast) if(!a.on){
        a={frand(40,W-40),(float)(H+30),
           frand(0.5f,1.5f),frand(0,360),
           frand(-2.5f,2.5f),frand(14,28),true,0};
        break;
    }
}

void spawnBoss(){
    boss.x=W/2; boss.y=H+80;
    boss.targetX=W/2; boss.targetY=H-160;
    boss.vx=0; boss.vy=0;
    int diffMul=(difficulty==0)?1:(difficulty==2)?3:2;
    boss.maxHp=200+wave*20*diffMul;
    boss.hp=boss.maxHp;
    boss.on=true;
    boss.phase=BOSS_PHASE1;
    boss.phaseTimer=0; boss.shootCd=0;
    boss.ang=0; boss.angSpd=0.5f;
    boss.flashTimer=0;
    boss.entryProgress=0;
    boss.enraged=false;
    boss.attackPattern=0;
    boss.patternTimer=0;
    for(int i=0;i<4;i++){
        boss.orbAng[i]=i*PI/2;
        boss.orbOn[i]=true;
        boss.orbHp[i]=3;
    }
    waveFlash=120;
    currentZone=ZONE_BOSS;
    triggerScreenShake(8,30);
}

void spawnWave(){
    // Choose zone
    if(wave%5==0){
        spawnBoss();
        return;
    }
    if(wave%5==4)      currentZone=ZONE_NEBULA;
    else if(wave%5==3) currentZone=ZONE_ASTEROID;
    else if(wave%5==1) currentZone=ZONE_SPACE;
    else               currentZone=ZONE_SPACE;

    int numEne=3+wave+(difficulty==2?2:0);
    if(difficulty==0) numEne=max(2,numEne-2);
    for(int i=0;i<numEne;i++) spawnEnemy();
    if(wave>=2) spawnAst();
    if(wave>=4) spawnAst();
    waveFlash=90;
}

void firePlayer(){
    int cooldown=puRapidFire>0?5:12;
    if(bcool>0) return;

    if(puLaser>0){
        // Find a free slot for laser — it occupies one wide beam
        for(auto& b:bul) if(!b.on){
            b={sx,sy+38,true,0,true,false,0,0};
            bcool=cooldown;
            break;
        }
        return;
    }

    if(puHoming>0){
        // Find closest enemy
        float best=1e9; float tx2=sx, ty2=sy+200;
        for(auto& e:ene) if(e.on){
            float d=dist(sx,sy,e.x,e.y);
            if(d<best){ best=d; tx2=e.x; ty2=e.y; }
        }
        if(boss.on){
            float d=dist(sx,sy,boss.x,boss.y);
            if(d<best){ tx2=boss.x; ty2=boss.y; }
        }
        for(auto& b:bul) if(!b.on){
            b={sx,sy+38,true,0,false,true,tx2,ty2};
            bcool=cooldown;
            break;
        }
        return;
    }

    // Normal fire (double shot if rapid)
    int shotsToFire=1+(puRapidFire>0?1:0);
    float offsets[]={0,-8,8,-16,16};
    int fired=0;
    for(auto& b:bul){
        if(!b.on && fired<shotsToFire){
            b={sx+offsets[fired]*shotsToFire,sy+38,true,0,false,false,0,0};
            fired++;
            bcool=cooldown;
        }
    }
}

void activateBomb(){
    // Destroys all on-screen enemies
    for(auto& e:ene) if(e.on){
        spawnExplosion(e.x,e.y,1.5f);
        score+=50;
        e.on=false;
    }
    for(auto& a:ast) if(a.on){
        spawnExplosion(a.x,a.y,1.2f,0.7f,0.5f,0.3f);
        score+=25;
        a.on=false;
    }
    // Damage boss
    if(boss.on){
        boss.hp-=50;
        triggerScreenShake(10,25);
        spawnHitRing(boss.x,boss.y,1.0f,0.5f,0.1f);
    }
    scoreAnim=1.0f;
    triggerScreenShake(12,30);
}

void shootEnemyBullet(float x,float y,float vx,float vy){
    for(auto& b:ebul) if(!b.on){
        b={x,y,vx,vy,true,0};
        break;
    }
}

void initGame(){
    sx=W/2; sy=80; sang=0; sscale=0.1f;
    score=0; lastScore=0; lives=3; wave=1; fc=0; bcool=0;
    comboCount=0; comboTimer=0; comboPop=0;
    shieldOn=shieldCool=0; waveFlash=0; scoreAnim=0;
    puRapidFire=puSlowMo=puLaser=puHoming=0;
    gameSpeed=1.0f; slowMoTarget=1.0f;
    shakeX=shakeY=shakeMag=0; shakeFrames=0;
    mL=mR=mU=mD=false;
    currentZone=ZONE_SPACE;
    planetRot=0; blackHoleT=0;
    levelZoneTimer=0;
    for(auto& b:bul)   b.on=false;
    for(auto& e:ene)   e.on=false;
    for(auto& a:ast)   a.on=false;
    for(auto& p:pups)  p.on=false;
    for(auto& b:ebul)  b.on=false;
    for(auto& h:hitRings) h.on=false;
    boss.on=false;
    exps.clear();
    playerTrail.clear();
    for(int i=0;i<MB;i++) bulletTrails[i].clear();
    spawnWave();
    state=PLAY;
    transAlpha=1.0f; transDir=-1;
}

// ─────────────────────────────────────────────────────────────────────────────
//  BOSS AI UPDATE
// ─────────────────────────────────────────────────────────────────────────────
void updateBoss(){
    if(!boss.on) return;

    // Entry animation
    if(boss.entryProgress<1.0f){
        boss.entryProgress+=0.008f;
        boss.y=lerp(H+80,H-160,boss.entryProgress);
        return;
    }

    // Phase transitions
    float hfrac=(float)boss.hp/boss.maxHp;
    if(hfrac<0.66f && boss.phase==BOSS_PHASE1){
        boss.phase=BOSS_PHASE2; boss.angSpd=1.2f;
        boss.enraged=false;
        triggerScreenShake(6,20);
        spawnExplosion(boss.x,boss.y,0.8f,1.0f,0.5f,0.0f);
    }
    if(hfrac<0.33f && boss.phase==BOSS_PHASE2){
        boss.phase=BOSS_PHASE3; boss.angSpd=2.5f; boss.enraged=true;
        triggerScreenShake(10,30);
        spawnExplosion(boss.x,boss.y,1.5f,1.0f,0.0f,1.0f);
    }

    boss.ang+=boss.angSpd*gameSpeed;

    // Movement: oscillate toward player
    float dx=sx-boss.x, dy=sy+200-boss.y;
    float moveSpeed=(boss.phase==BOSS_PHASE1)?0.8f:
                    (boss.phase==BOSS_PHASE2)?1.2f:1.8f;
    boss.vx=lerp(boss.vx,dx*0.012f*moveSpeed,0.04f);
    boss.vy=lerp(boss.vy,dy*0.012f*moveSpeed,0.04f);
    boss.x=clamp(boss.x+boss.vx*gameSpeed,80,(float)W-80);
    boss.y=clamp(boss.y+boss.vy*gameSpeed,H-300,(float)H-120);

    if(boss.flashTimer>0) boss.flashTimer-=gameSpeed;

    // Rotate shield orbs
    for(int i=0;i<4;i++)
        boss.orbAng[i]+=0.03f*(boss.phase==BOSS_PHASE3?2.5f:1.0f)*gameSpeed;

    // Attack patterns
    boss.patternTimer+=gameSpeed;
    boss.shootCd-=gameSpeed;

    if(boss.shootCd<=0){
        float shootRate=(boss.phase==BOSS_PHASE1)?55:
                        (boss.phase==BOSS_PHASE2)?35:18;
        boss.shootCd=shootRate;
        boss.attackPattern=(boss.attackPattern+1)%3;

        float dx2=sx-boss.x, dy2=sy-boss.y;
        float len=sqrtf(dx2*dx2+dy2*dy2)+0.001f;
        float nx=dx2/len, ny=dy2/len;
        float spd=3.5f+(boss.phase==BOSS_PHASE3?1.5f:0);

        if(boss.phase==BOSS_PHASE1){
            // Single aimed shot
            shootEnemyBullet(boss.x,boss.y,nx*spd,ny*spd);
        } else if(boss.phase==BOSS_PHASE2){
            // 3-way spread
            for(int i=-1;i<=1;i++){
                float a=atan2f(ny,nx)+i*0.25f;
                shootEnemyBullet(boss.x,boss.y,cosf(a)*spd,sinf(a)*spd);
            }
        } else {
            // 8-way radial burst
            for(int i=0;i<8;i++){
                float a=i*PI/4;
                shootEnemyBullet(boss.x,boss.y,cosf(a)*spd,sinf(a)*spd);
            }
            // Plus aimed
            shootEnemyBullet(boss.x,boss.y,nx*(spd+1),ny*(spd+1));
        }
    }

    // Check player collision
    if(!shieldOn && dist(sx,sy,boss.x,boss.y)<60){
        spawnExplosion(boss.x/2+sx/2,boss.y/2+sy/2,1.2f);
        if(--lives<=0){ saveHighScore(); state=OVER; }
        triggerScreenShake(8,20);
    }

    // Check player bullets
    for(int i=0;i<MB;i++){
        auto& b=bul[i];
        if(!b.on) continue;
        bool hit=false;

        // Check orb collision first
        for(int j=0;j<4;j++){
            if(!boss.orbOn[j]) continue;
            int ox=(int)(boss.x+38*cosf(boss.orbAng[j]));
            int oy=(int)(boss.y+38*sinf(boss.orbAng[j]));
            if(dist(b.x,b.y,ox,oy)<10){
                b.on=false;
                spawnHitRing(ox,oy,1.0f,0.6f,0.2f);
                if(--boss.orbHp[j]<=0) boss.orbOn[j]=false;
                hit=true; break;
            }
        }
        if(hit) continue;

        if(dist(b.x,b.y,boss.x,boss.y)<50){
            b.on=false;
            int dmg=b.isLaser?5:1;
            boss.hp-=dmg;
            boss.flashTimer=8;
            spawnHitRing(boss.x,boss.y,1.0f,0.4f,0.1f);
            score+=5; scoreAnim=min(1.0f,scoreAnim+0.2f);
            comboCount++; comboTimer=120;

            if(boss.hp<=0){
                // Boss dead!
                spawnExplosion(boss.x,boss.y,3.0f,1.0f,0.3f,1.0f);
                for(int k=0;k<6;k++){
                    spawnExplosion(boss.x+frand(-60,60),
                                   boss.y+frand(-60,60),1.5f);
                }
                score+=1000+wave*100;
                scoreAnim=1.0f;
                boss.on=false;
                triggerScreenShake(15,50);
                spawnPowerUp(boss.x,boss.y);
                // Next wave
                wave++;
                currentZone=ZONE_SPACE;
                spawnWave();
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  GAME UPDATE
// ─────────────────────────────────────────────────────────────────────────────
void update(){
    menuAnim++; settingsAnim++;

    if(state==SPLASH){
        splashT++;
        for(auto& s:stars){ s.y-=s.spd*0.3f; if(s.y<0){s.y=H;s.x=frand(0,W);} }
        if(splashT>=180){ state=MENU; }
        glutPostRedisplay(); return;
    }

    if(state==MENU){ updateRings(); glutPostRedisplay(); return; }
    if(state==SETTINGS){ glutPostRedisplay(); return; }
    if(state==OVER){
        updateRings();
        // Fade-in transition
        transAlpha=max(0.0f,transAlpha-0.02f);
        glutPostRedisplay(); return;
    }
    if(state==PAUSE){ glutPostRedisplay(); return; }

    // Transition fade
    if(transDir<0) transAlpha=max(0.0f,transAlpha-0.04f);
    else           transAlpha=min(1.0f,transAlpha+0.04f);

    fc++;
    updateRings();
    applyShake();

    // Smooth slow-mo
    gameSpeed=lerp(gameSpeed,slowMoTarget,0.06f);
    if(puSlowMo<=0) slowMoTarget=1.0f;
    if(puSlowMo>0)  slowMoTarget=0.35f;

    // Decrement power-up timers
    if(puRapidFire>0) puRapidFire-=(int)max(1.0f,gameSpeed);
    if(puSlowMo>0)    puSlowMo--;
    if(puLaser>0)     puLaser-=(int)max(1.0f,gameSpeed);
    if(puHoming>0)    puHoming-=(int)max(1.0f,gameSpeed);

    if(waveFlash>0) waveFlash-=gameSpeed;
    if(scoreAnim>0) scoreAnim-=0.06f;
    if(comboPop>0)  comboPop-=0.06f;
    if(comboTimer>0) comboTimer-=(int)gameSpeed;
    else if(comboTimer<=0 && comboCount>0) comboCount=0;

    // Animations
    planetRot+=0.5f*gameSpeed;
    blackHoleT+=gameSpeed;

    // Player movement
    float tAng=0;
    if(mL){ sx-=SPDS*gameSpeed; tAng= 12; }
    if(mR){ sx+=SPDS*gameSpeed; tAng=-12; }
    if(mU) sy+=SPDS*0.8f*gameSpeed;
    if(mD) sy-=SPDS*0.8f*gameSpeed;
    sang+=(tAng-sang)*0.12f;
    sx=max(28.0f,min((float)W-28,sx));
    sy=max(55.0f,min((float)H-80,sy));
    sscale=min(1.0f,sscale+0.04f);

    // Update trail
    updatePlayerTrail();

    // Stars scroll
    for(auto& s:stars){
        s.y-=s.spd*gameSpeed;
        if(s.y<0){ s.y=H; s.x=frand(0,W); }
    }

    // Bullets
    if(bcool>0) bcool-=(int)ceil(gameSpeed);
    for(int i=0;i<MB;i++){
        auto& b=bul[i];
        if(!b.on) continue;
        // Homing: steer toward target
        if(b.isHoming){
            float dx2=b.hx-b.x, dy2=b.hy-b.y;
            float len=sqrtf(dx2*dx2+dy2*dy2)+0.001f;
            b.x+=dx2/len*7*gameSpeed;
            b.y+=dy2/len*7*gameSpeed;
        } else {
            b.y+=9*gameSpeed;
        }
        b.age++;
        if(b.y>H+10) b.on=false;
        // Bullet trail
        if(showTrails){
            bulletTrails[i].push_front({b.x,b.y,0.8f,2.0f});
            if(bulletTrails[i].size()>10) bulletTrails[i].pop_back();
            for(auto& n:bulletTrails[i]) n.alpha*=0.7f;
        }
    }

    // Enemy bullets
    for(auto& b:ebul){
        if(!b.on) continue;
        b.x+=b.vx*gameSpeed; b.y+=b.vy*gameSpeed; b.age++;
        if(b.x<-20||b.x>W+20||b.y<-20||b.y>H+20){ b.on=false; continue; }
        // Hit player
        if(!shieldOn && dist(b.x,b.y,sx,sy)<22){
            b.on=false;
            spawnHitRing(sx,sy,1.0f,0.4f,0.1f);
            triggerScreenShake(5,10);
            if(--lives<=0){ saveHighScore(); state=OVER; }
        }
    }

    // Enemies
    int alive=0;
    for(auto& e:ene){
        if(!e.on) continue;
        alive++;
        e.aiTimer+=gameSpeed;
        if(e.flashTimer>0) e.flashTimer-=gameSpeed;

        // AI behavior
        switch(e.ai){
            case AI_SINE:
                e.y-=e.spd*gameSpeed;
                e.x+=sinf(fc*0.025f+(&e-ene))*1.2f*gameSpeed;
                e.ang=sinf(fc*0.05f+(&e-ene))*12.0f;
                break;
            case AI_ZIGZAG:{
                e.y-=e.spd*gameSpeed;
                float zz=sinf(e.aiTimer*0.07f)>0?1.5f:-1.5f;
                e.x+=zz*e.spd*1.5f*gameSpeed;
                e.ang=zz*20;
                e.x=clamp(e.x,30.0f,(float)W-30);
                break;
            }
            case AI_TRACKER:
                // Chase player slowly
                {float dx2=sx-e.x, dy2=sy-e.y;
                float len=sqrtf(dx2*dx2+dy2*dy2)+0.001f;
                e.x+=dx2/len*e.spd*0.6f*gameSpeed;
                e.y+=dy2/len*e.spd*0.6f*gameSpeed;
                e.ang=atan2f(dx2,dy2)*180/PI; }
                break;
            case AI_SWARM:{
                // Swarm: follow a formation + offset by index
                float formX=W/2+cosf(fc*0.01f+e.swarmIdx*PI/4)*160;
                float formY=H-160+sinf(fc*0.008f+e.swarmIdx*PI/3)*60;
                float dx2=formX-e.x, dy2=formY-e.y;
                e.x+=dx2*0.025f*gameSpeed;
                e.y+=dy2*0.025f*gameSpeed;
                e.y-=e.spd*0.3f*gameSpeed;
                break;
            }
            case AI_KAMIKAZE:
                // Dive straight at player
                {float dx2=sx-e.x, dy2=sy-e.y;
                float len=sqrtf(dx2*dx2+dy2*dy2)+0.001f;
                float kSpd=e.spd*(1+e.aiTimer*0.004f);
                e.x+=dx2/len*kSpd*gameSpeed;
                e.y+=dy2/len*kSpd*gameSpeed;
                e.ang=atan2f(dx2,dy2)*180/PI; }
                break;
        }

        if(e.y<-40){ e.on=false; if(--lives<=0){ saveHighScore(); state=OVER; } continue; }
        if(e.x<-60||e.x>W+60) e.on=false;

        // Bullet hit
        for(int i=0;i<MB;i++){
            auto& b=bul[i];
            if(!b.on) continue;
            float hitR=b.isLaser?30:22;
            if(dist(b.x,b.y,e.x,e.y)<hitR){
                b.on=false;
                e.flashTimer=8;
                spawnHitRing(e.x,e.y);
                if(--e.hp<=0){
                    spawnExplosion(e.x,e.y,1.0f);
                    // Combo scoring
                    comboCount++; comboPop=1.0f;
                    comboTimer=120;
                    int baseScore=100;
                    if(comboCount>=10)     baseScore*=5;
                    else if(comboCount>=5) baseScore*=3;
                    else if(comboCount>=2) baseScore*=comboCount;
                    score+=baseScore; scoreAnim=1.0f;
                    // Chance to drop power-up
                    if(rand()%5==0) spawnPowerUp(e.x,e.y);
                    e.on=false;
                }
            }
        }
        // Player collision
        if(!shieldOn&&dist(sx,sy,e.x,e.y)<30){
            spawnExplosion(e.x,e.y,0.8f,1.0f,0.4f,0.1f);
            e.on=false; comboCount=0;
            triggerScreenShake(6,15);
            if(--lives<=0){ saveHighScore(); state=OVER; }
        }
    }

    // Asteroids
    for(auto& a:ast){
        if(!a.on) continue;
        a.y-=a.spd*gameSpeed; a.rot+=a.rotSpd*gameSpeed;
        if(a.flashTimer>0) a.flashTimer-=gameSpeed;
        if(a.y<-50){ a.on=false; continue; }
        for(int i=0;i<MB;i++){
            auto& b=bul[i];
            if(!b.on) continue;
            if(dist(b.x,b.y,a.x,a.y)<a.r){
                b.on=false;
                a.flashTimer=8;
                spawnHitRing(a.x,a.y,0.8f,0.6f,0.4f);
                spawnExplosion(a.x,a.y,0.9f,0.7f,0.5f,0.3f);
                a.on=false;
                comboCount++; comboPop=1.0f; comboTimer=120;
                score+=50+(comboCount>2?50:0); scoreAnim=1.0f;
            }
        }
        if(!shieldOn&&dist(sx,sy,a.x,a.y)<a.r+18){
            spawnExplosion(a.x,a.y,0.9f,0.7f,0.5f,0.3f);
            a.on=false; comboCount=0;
            triggerScreenShake(5,12);
            if(--lives<=0){ saveHighScore(); state=OVER; }
        }
    }

    // Power-ups
    for(auto& p:pups){
        if(!p.on) continue;
        p.y+=p.vy*gameSpeed;
        p.animT+=gameSpeed;
        p.bob+=0.06f*gameSpeed;
        if(p.y<-20){ p.on=false; continue; }
        if(dist(sx,sy,p.x,p.y)<28){
            // Collect
            switch(p.type){
                case PU_RAPIDFIRE: puRapidFire=300; break;
                case PU_SHIELD:    shieldOn=120; shieldCool=0; break;
                case PU_SLOWMO:    puSlowMo=180; break;
                case PU_LASER:     puLaser=180; break;
                case PU_HOMING:    puHoming=300; break;
                case PU_BOMB:      activateBomb(); break;
            }
            spawnHitRing(p.x,p.y,0.5f,1.0f,0.5f);
            p.on=false;
            score+=25; scoreAnim=1.0f;
        }
    }

    // Boss
    updateBoss();

    // Wave complete check
    if(!boss.on){
        bool anyAlive=false;
        for(auto& e:ene) if(e.on){ anyAlive=true; break; }
        if(!anyAlive){
            bool anyAst=false;
            for(auto& a:ast) if(a.on){ anyAst=true; break; }
            if(!anyAst && !boss.on){
                wave++;
                if(score>highScore) saveHighScore();
                spawnWave();
            }
        }
    }

    // Shield
    shieldPulse++;
    if(shieldOn>0)   shieldOn-=(int)ceil(gameSpeed);
    if(shieldCool>0) shieldCool-=(int)ceil(gameSpeed);

    // Explosions — physics update
    for(auto& ex:exps){
        if(!ex.on) continue;
        ex.scale+=0.055f*gameSpeed;
        ex.alpha-=0.028f*gameSpeed;
        if(ex.alpha<=0){ ex.on=false; continue; }
        for(auto& p:ex.pts){
            if(!p.on) continue;
            p.x+=p.vx*gameSpeed; p.y+=p.vy*gameSpeed;
            p.vy+=p.gravity*gameSpeed;     // gravity
            p.vx*=pow(p.friction,gameSpeed); // friction
            p.vy*=pow(p.friction,gameSpeed);
            p.life-=0.022f*gameSpeed;
            if(p.life<=0) p.on=false;
        }
    }

    glutPostRedisplay();
}

// ─────────────────────────────────────────────────────────────────────────────
//  DISPLAY CALLBACK
// ─────────────────────────────────────────────────────────────────────────────
void display(){
    glClear(GL_COLOR_BUFFER_BIT);

    if(state==SPLASH){ drawSplash(); glutSwapBuffers(); return; }
    if(state==MENU)  { drawMenu();   glutSwapBuffers(); return; }
    if(state==SETTINGS){ drawSettings(); glutSwapBuffers(); return; }

    // Apply screen shake offset
    glPushMatrix();
    glTranslatef(shakeX,shakeY,0);

    drawBackground();
    drawBlackHole();
    drawNebula();
    drawStars();
    drawRings();
    drawPlanet();

    // Bullet trails
    if(showTrails){
        for(int i=0;i<MB;i++){
            for(auto& n:bulletTrails[i]){
                glColor4f(0.3f,0.9f,1.0f,n.alpha*0.4f);
                glPointSize(n.sz);
                glBegin(GL_POINTS); glVertex2f(n.x,n.y); glEnd();
            }
        }
        glPointSize(1);
    }

    drawPlayerTrail();
    drawExps();
    updateDrawHitRings();

    // Asteroids
    for(auto& a:ast) if(a.on) drawAst(a);

    // Enemies
    for(auto& e:ene) if(e.on) drawEnemy(e);

    // Enemy bullets
    for(auto& b:ebul) if(b.on) drawEnemyBullet(b);

    // Boss
    drawBoss();

    // Power-ups
    for(auto& p:pups) if(p.on) drawPowerUp(p);

    // Player bullets
    for(auto& b:bul) if(b.on) drawBullet(b);

    // Shield
    if(shieldOn>0) drawShield(sx,sy);

    // Player ship
    drawShip(sx,sy,sang,sscale);

    // Slow-mo overlay
    drawSlowMoOverlay();

    // HUD
    drawHUD();
    drawRadar();
    drawCombo();
    drawWaveBanner();

    glPopMatrix(); // end shake

    // Overlays (no shake)
    if(state==PAUSE) drawPauseOverlay();
    if(state==OVER)  drawOver();

    drawTransitionFade();

    glutSwapBuffers();
}

// ─────────────────────────────────────────────────────────────────────────────
//  INPUT
// ─────────────────────────────────────────────────────────────────────────────
void reshape(int w,int h){
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0,W,0,H);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}

void keyDown(unsigned char k,int,int){
    if(k==27){
        if(state==SETTINGS){ state=MENU; return; }
        exit(0);
    }

    // Splash — any key skips
    if(state==SPLASH){ state=MENU; splashT=180; return; }

    // Settings
    if(state==SETTINGS){
        if(k=='\t' || k==9){ state=MENU; return; }
        return;
    }

    if(k==' '&&state==MENU){ initGame(); return; }
    if((k=='\t'||k==9)&&state==MENU){ state=SETTINGS; return; }
    if((k=='\t'||k==9)&&state==PLAY){ state=PAUSE; return; }
    if((k=='\t'||k==9)&&state==PAUSE){ state=SETTINGS; return; }

    if(k==13&&state==PLAY) firePlayer();
    if(k=='r'||k=='R'){ initGame(); return; }
    if(k=='p'||k=='P'){
        if(state==PLAY)  state=PAUSE;
        else if(state==PAUSE) state=PLAY;
    }
    if((k=='s'||k=='S')&&state==PLAY&&!shieldCool&&!shieldOn){
        shieldOn=90; shieldCool=300;
    }
    if(k=='w'||k=='W') mU=1;
    if(k=='a'||k=='A') mL=1;
    if(k=='d'||k=='D') mR=1;
    if(k=='x'||k=='X'||k=='s'||k=='S') { if(k=='x'||k=='X') mD=1; }
    // Use S for down in gameplay (no conflict when shield is on cooldown)
    if(k=='s'&&shieldCool>0) mD=1;
}

void keyUp(unsigned char k,int,int){
    if(k=='w'||k=='W') mU=0;
    if(k=='a'||k=='A') mL=0;
    if(k=='d'||k=='D') mR=0;
    if(k=='x'||k=='X') mD=0;
    if(k=='s'&&shieldCool>0) mD=0;
}

void spDown(int k,int,int){
    if(state==SETTINGS){
        if(k==GLUT_KEY_UP)   settingsSel=max(0,settingsSel-1);
        if(k==GLUT_KEY_DOWN) settingsSel=min(3,settingsSel+1);
        if(k==GLUT_KEY_LEFT||k==GLUT_KEY_RIGHT){
            int dir=(k==GLUT_KEY_RIGHT)?1:-1;
            switch(settingsSel){
                case 0: difficulty=clamp(difficulty+dir,0,2); break;
                case 1: showRadar=!showRadar; break;
                case 2: showTrails=!showTrails; break;
                case 3: state=MENU; break;
            }
        }
        return;
    }
    if(k==GLUT_KEY_UP)    mU=1;
    if(k==GLUT_KEY_DOWN)  mD=1;
    if(k==GLUT_KEY_LEFT)  mL=1;
    if(k==GLUT_KEY_RIGHT) mR=1;
    if(k==GLUT_KEY_F1 && state==PLAY) firePlayer();
}

void spUp(int k,int,int){
    if(state==SETTINGS) return;
    if(k==GLUT_KEY_UP)    mU=0;
    if(k==GLUT_KEY_DOWN)  mD=0;
    if(k==GLUT_KEY_LEFT)  mL=0;
    if(k==GLUT_KEY_RIGHT) mR=0;
}

void timer(int){
    update();
    glutTimerFunc(16,timer,0);  // ~60 FPS
}

// ─────────────────────────────────────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc,char** argv){
    srand(time(0));
    loadHighScore();

    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(W,H);
    glutInitWindowPosition(100,60);
    glutCreateWindow("PARTH & SPACE — ULTIMATE EDITION");

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