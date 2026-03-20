/*
 * ================================================================
 *  PARTH&SPACE — 2D Space Shooter
 *  OpenGL / FreeGLUT | C++
 *  Author : Shuvo Singh Partho
 *  Gmail : partho23105101445@diu.edu.bd
 * ================================================================
 */

#include <GL/glut.h>
#include <bits/stdc++.h>
using namespace std;

const int W = 900, H = 700;
const float PI = acos(-1.0f);
struct Star  { float x, y, spd, bright, sz; };
struct Bullet{ float x, y; bool on; };

struct Enemy {
    float x, y, baseY, spd, ang;
    bool on; int hp;
};

struct Asteroid {
    float x, y, spd, rot, rotSpd, r;
    bool on;
};

struct Particle { float x,y,vx,vy,life,r,g,b; };

struct Explosion {
    float x, y, scale, alpha;
    bool on;
    vector<Particle> pts;
};

enum State { MENU, PLAY, PAUSE, OVER };
State state = MENU;

float sx = W/2, sy = 80, sang = 0, sscale = 1;
bool mL=0,mR=0,mU=0,mD=0;
const float SPDS = 4.5f;

int score=0, lives=3, wave=1, fc=0, bcool=0;
int shieldOn=0, shieldCool=0;
float shieldPulse=0;

const int MB=10, ME=8, MA=6, NS=180;
Bullet   bul[MB];
Enemy    ene[ME];
Asteroid ast[MA];
Star     stars[NS];
vector<Explosion> exps;

float frand(float lo,float hi){ return lo+(hi-lo)*(rand()/(float)RAND_MAX); }
float dist(float ax,float ay,float bx,float by){ return hypotf(ax-bx,ay-by); }

void txt(float x,float y,const string& s,void* f=GLUT_BITMAP_HELVETICA_18){
    glRasterPos2f(x,y);
    for(char c:s) glutBitmapCharacter(f,c);
}

//  DDA Line — increments by (dx/steps, dy/steps) each step
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

//  Bresenham Line — integer-only, adjusts error term each step
void bres(int x1,int y1,int x2,int y2){
    int dx=abs(x2-x1), dy=abs(y2-y1);
    int sx=(x1<x2)?1:-1, sy=(y1<y2)?1:-1, err=dx-dy;
    glBegin(GL_POINTS);
    while(true){
        glVertex2f(x1,y1);
        if(x1==x2&&y1==y2) break;
        int e2=2*err;
        if(e2>-dy){err-=dy;x1+=sx;}
        if(e2< dx){err+=dx;y1+=sy;}
    }
    glEnd();
}


//  Midpoint Circle — uses decision parameter p = 1 - r
void mpc(int cx,int cy,int r){
    int x=0,y=r,p=1-r;
    auto p8=[&](int px,int py){
        glVertex2f(cx+px,cy+py); glVertex2f(cx-px,cy+py);
        glVertex2f(cx+px,cy-py); glVertex2f(cx-px,cy-py);
        glVertex2f(cx+py,cy+px); glVertex2f(cx-py,cy+px);
        glVertex2f(cx+py,cy-px); glVertex2f(cx-py,cy-px);
    };
    glBegin(GL_POINTS);
    p8(x,y);
    while(x<y){
        x++;
        p = (p<0) ? p+2*x+1 : p+2*(x- --y)+1;
        p8(x,y);
    }
    glEnd();
}

void mpcFill(int cx,int cy,int r){
    for(int i=1;i<=r;i++) mpc(cx,cy,i);
}

// ── Init helpers
void initStars(){
    for(int i=0;i<NS;i++){
        int l=i%3;
        stars[i]={frand(0,W),frand(0,H),
                  l==0?0.3f:l==1?0.8f:1.8f,
                  l==0?0.4f:l==1?0.7f:1.0f,
                  l==0?1.0f:l==1?1.5f:2.5f};
    }
}

void spawnEnemy(){
    for(auto& e:ene) if(!e.on){
        e={frand(60,W-60),frand(H+20,H+150),0,
           frand(0.6f,1.2f)+wave*0.15f,0,true,(wave>=3)?2:1};
        e.baseY=e.y; break;
    }
}

void spawnAst(){
    for(auto& a:ast) if(!a.on){
        a={frand(40,W-40),(float)(H+30),frand(0.5f,1.5f),
           frand(0,360),frand(-2.5f,2.5f),frand(14,28),true};
        break;
    }
}

void spawnWave(){
    for(int i=0;i<3+wave;i++) spawnEnemy();
    if(wave>=2) spawnAst();
}

void spawnExp(float x,float y){
    Explosion ex{x,y,0.1f,1.0f,true,{}};
    for(int i=0;i<20;i++){
        float a=frand(0,2*PI), s=frand(1.5f,4.5f);
        ex.pts.push_back({x,y,s*cosf(a),s*sinf(a),1.0f,
                          frand(0.8f,1.0f),frand(0.3f,0.8f),frand(0,0.3f)});
    }
    exps.push_back(ex);
}

void initGame(){
    sx=W/2; sy=80; sang=0; sscale=0.1f;
    score=0; lives=3; wave=1; fc=0; bcool=0;
    shieldOn=shieldCool=0;
    mL=mR=mU=mD=false;
    for(auto& b:bul) b.on=false;
    for(auto& e:ene) e.on=false;
    for(auto& a:ast) a.on=false;
    exps.clear();
    spawnWave();
    state=PLAY;
}

void fire(){
    if(bcool>0) return;
    for(auto& b:bul) if(!b.on){ b={sx,sy+38,true}; bcool=12; break; }
}

// ── Draw functions
void drawShip(float x,float y,float ang,float sc){
    glPushMatrix();
    glTranslatef(x,y,0);      // Translation: ship position
    glRotatef(ang,0,0,1);     // Rotation: banking on turn
    glScalef(sc,sc,1);        // Scaling: spawn-in pulse

    glColor4f(0.2f,0.6f,1.0f,0.3f);
    mpc(-10,-26,7); mpc(10,-26,7);

    glColor3f(0.55f,0.85f,1.0f);
    glBegin(GL_POLYGON);
        glVertex2f(0,40); glVertex2f(-14,10); glVertex2f(-18,-20);
        glVertex2f(0,-10); glVertex2f(18,-20); glVertex2f(14,10);
    glEnd();

    glColor3f(0.1f,0.3f,0.8f);
    glBegin(GL_POLYGON);
        glVertex2f(0,28); glVertex2f(-7,14); glVertex2f(0,10); glVertex2f(7,14);
    glEnd();

    glColor3f(0.2f,0.9f,0.9f);
    dda(-18,-20,-28,-30); dda(-28,-30,-10,-26);
    dda( 18,-20, 28,-30); dda( 28,-30, 10,-26);

    glColor3f(1.0f,0.5f,0.1f);
    bres(-13,-26,-7,-26); bres(7,-26,13,-26);

    // Flicker exhaust
    bool flk=(fc%6<3);
    glColor3f(1,flk?0.7f:0.3f,0.1f);
    dda(-10,-27,-12,flk?-40:-35);
    dda( 10,-27, 12,flk?-40:-35);

    glPopMatrix();
}

void drawShield(float x,float y){
    float p=0.5f+0.5f*sinf(shieldPulse*0.2f);
    glColor4f(0.3f,0.8f,1.0f,0.15f+0.1f*p);
    mpcFill((int)x,(int)y,38);
    glColor4f(0.4f,1.0f,1.0f,0.6f+0.3f*p);
    for(int i=0;i<12;i++){
        float a1=i*30*PI/180, a2=(i+1)*30*PI/180;
        bres((int)(x+42*cosf(a1)),(int)(y+42*sinf(a1)),
             (int)(x+42*cosf(a2)),(int)(y+42*sinf(a2)));
    }
}

void drawEnemy(const Enemy& e){
    glPushMatrix();
    glTranslatef(e.x,e.y,0);  // Translation
    glRotatef(e.ang,0,0,1);   // Rotation: wobble

    glColor3f(1.0f,0.25f,0.35f);
    glBegin(GL_POLYGON);
        glVertex2f(0,-30); glVertex2f(-16,-5); glVertex2f(-20,20);
        glVertex2f(0,12);  glVertex2f(20,20);  glVertex2f(16,-5);
    glEnd();

    glColor3f(1.0f,0.6f,0.1f);
    mpcFill(0,0,8);

    glColor3f(0.8f,0.1f,0.2f);
    dda(-20,20,-30,28); dda(20,20,30,28);
    bres(-10,15,10,15);

    glPopMatrix();
}

void drawAst(const Asteroid& a){
    glPushMatrix();
    glTranslatef(a.x,a.y,0);  // Translation
    glRotatef(a.rot,0,0,1);   // Rotation: spin

    glColor3f(0.55f,0.48f,0.38f);
    glBegin(GL_POLYGON);
    for(int i=0;i<10;i++){
        float ang=i*2*PI/10;
        float rad=a.r*(0.75f+0.25f*sinf(ang*3+a.rot*0.01f));
        glVertex2f(rad*cosf(ang),rad*sinf(ang));
    }
    glEnd();

    glColor3f(0.35f,0.30f,0.22f);
    mpc((int)(a.r/3),(int)(a.r/4),(int)(a.r/4));
    mpc(-(int)(a.r/4),-(int)(a.r/3),(int)(a.r/5));

    glPopMatrix();
}

void drawBullet(const Bullet& b){
    glColor4f(0.2f,1.0f,0.5f,0.4f);
    mpc((int)b.x,(int)b.y,5);
    glColor3f(0.4f,1.0f,0.6f);
    dda((int)b.x,(int)b.y-2,(int)b.x,(int)b.y+14);
    glColor3f(1,1,1);
    dda((int)b.x,(int)b.y,(int)b.x,(int)b.y+10);
}

void drawPlanet(){
    int px=720,py=560,pr=90;
    glColor3f(0.05f,0,0.12f);
    mpcFill(px,py,pr);
    for(int r=pr;r>0;r-=2){
        float t=1.0f-r/(float)pr;
        glColor3f(0.1f+0.4f*t,0.15f*t,0.3f+0.4f*t);
        mpc(px-10,py+10,r);
    }
    glColor4f(0.5f,0.2f,1.0f,0.3f);
    for(int r=pr+1;r<=pr+6;r++) mpc(px,py,r);
    glColor4f(0.3f,0.05f,0.6f,0.5f);
    for(int b=-3;b<=3;b++){
        int off=(int)sqrtf((float)(pr*pr-(b*14)*(b*14)));
        bres(px-off,py+b*14,px+off,py+b*14);
    }
}

void drawStars(){
    for(auto& s:stars){
        glColor3f(s.bright,s.bright,s.bright*1.1f);
        glPointSize(s.sz);
        glBegin(GL_POINTS); glVertex2f(s.x,s.y); glEnd();
    }
    glPointSize(1);
}

void drawExps(){
    for(auto& ex:exps){
        if(!ex.on) continue;
        glPushMatrix();
        glTranslatef(ex.x,ex.y,0);
        glScalef(ex.scale,ex.scale,1); // Scaling: explosion grow
        glColor4f(1.0f,0.6f,0.1f,ex.alpha*0.5f); mpc(0,0,28);
        glColor4f(1.0f,0.9f,0.3f,ex.alpha);       mpc(0,0,16);
        glColor4f(1,1,1,ex.alpha);                 mpcFill(0,0,8);
        glPopMatrix();
        for(auto& p:ex.pts){
            glColor4f(p.r,p.g,p.b,p.life);
            glPointSize(2.5f);
            glBegin(GL_POINTS); glVertex2f(p.x,p.y); glEnd();
        }
    }
    glPointSize(1);
}

void drawHUD(){
    glColor4f(0,0.05f,0.15f,0.85f);
    glBegin(GL_QUADS);
        glVertex2f(0,0);glVertex2f(W,0);glVertex2f(W,44);glVertex2f(0,44);
    glEnd();
    glColor3f(0.3f,0.6f,1.0f);
    bres(0,44,W,44);

    glColor3f(0.4f,1.0f,0.8f);
    txt(20,16,"SCORE: "+to_string(score));
    glColor3f(0.55f,0.85f,1.0f);
    txt(W/2-40,16,"LIVES:");
    for(int i=0;i<lives;i++){
        float hx=W/2+20.0f+i*26, hy=22;
        glBegin(GL_TRIANGLES);
            glVertex2f(hx,hy+10); glVertex2f(hx-7,hy-8); glVertex2f(hx+7,hy-8);
        glEnd();
    }
    glColor3f(1.0f,0.7f,0.2f);
    txt(W-130,16,"WAVE: "+to_string(wave));

    if(shieldCool>0){ glColor3f(0.3f,0.5f,0.8f); txt(W/2-90,H-14,"SHIELD CHARGING...",GLUT_BITMAP_HELVETICA_12); }
    else            { glColor3f(0.3f,1.0f,0.9f); txt(W/2-60,H-14,"SHIELD READY [S]",  GLUT_BITMAP_HELVETICA_12); }
}

void drawMenu(){
    glBegin(GL_QUADS);
        glColor3f(0,0,0.06f);    glVertex2f(0,0); glVertex2f(W,0);
        glColor3f(0.02f,0,0.12f);glVertex2f(W,H); glVertex2f(0,H);
    glEnd();
    drawStars(); drawPlanet();

    glColor4f(0.1f,0.3f,0.7f,0.25f);
    glBegin(GL_QUADS);
        glVertex2f(180,385);glVertex2f(720,385);glVertex2f(720,470);glVertex2f(180,470);
    glEnd();
    glColor3f(0.3f,0.7f,1.0f);
    bres(180,385,720,385); bres(180,470,720,470);

    glColor3f(0.5f,0.95f,1.0f);
    txt(240,435,"P A R T H & S P A C E",GLUT_BITMAP_TIMES_ROMAN_24);
    glColor3f(0.8f,0.8f,1.0f);
    txt(350,400,"2D Space Shooter",GLUT_BITMAP_HELVETICA_18);

    glColor3f(0.6f,1.0f,0.7f);
    txt(320,345,"Press  ENTER  to Launch",GLUT_BITMAP_HELVETICA_18);

    glColor3f(0.7f,0.7f,0.9f);
    txt(305,310,"WASD / Arrow Keys : Move",            GLUT_BITMAP_HELVETICA_12);
    txt(305,293,"SPACE             : Fire Laser",       GLUT_BITMAP_HELVETICA_12);
    txt(305,276,"S                 : Activate Shield",  GLUT_BITMAP_HELVETICA_12);
    txt(305,259,"P / R / ESC       : Pause/Restart/Quit",GLUT_BITMAP_HELVETICA_12);
}

void drawOver(){
    glColor4f(0,0,0,0.75f);
    glBegin(GL_QUADS);
        glVertex2f(0,0);glVertex2f(W,0);glVertex2f(W,H);glVertex2f(0,H);
    glEnd();
    glColor3f(1.0f,0.2f,0.3f);
    txt(310,405,"G A M E   O V E R",GLUT_BITMAP_TIMES_ROMAN_24);
    glColor3f(1.0f,0.85f,0.3f);
    txt(340,365,"Score: "+to_string(score),GLUT_BITMAP_HELVETICA_18);
    glColor3f(0.5f,1.0f,0.7f);
    txt(315,315,"Press R to Restart",GLUT_BITMAP_HELVETICA_18);
    txt(325,288,"Press ESC to Quit", GLUT_BITMAP_HELVETICA_18);
}

// ── Game update
void update(){
    if(state!=PLAY) return;
    fc++;

    float tAng=0;
    if(mL){sx-=SPDS; tAng= 10;}
    if(mR){sx+=SPDS; tAng=-10;}
    if(mU) sy+=SPDS*0.8f;
    if(mD) sy-=SPDS*0.8f;
    sang+=(tAng-sang)*0.12f; // smooth lerp toward target bank angle

    sx=max(28.0f,min((float)W-28,sx));
    sy=max(55.0f,min((float)H-55,sy));
    sscale=min(1.0f,sscale+0.04f);

    for(auto& s:stars){ s.y-=s.spd; if(s.y<0){s.y=H;s.x=frand(0,W);} }

    if(bcool>0) bcool--;
    for(auto& b:bul) if(b.on){ b.y+=9; if(b.y>H+10) b.on=false; }

    int alive=0;
    for(auto& e:ene){
        if(!e.on) continue;
        alive++;
        e.y-=e.spd;
        e.x+=sinf(fc*0.025f+(&e-ene))*1.2f;  // sine-wave horizontal drift
        e.ang=sinf(fc*0.05f+(&e-ene))*12.0f;

        if(e.y<-40){ e.on=false; if(--lives<=0) state=OVER; continue; }

        for(auto& b:bul) if(b.on&&dist(b.x,b.y,e.x,e.y)<22){
            b.on=false;
            if(--e.hp<=0){spawnExp(e.x,e.y);e.on=false;score+=100;}
        }
        if(!shieldOn&&dist(sx,sy,e.x,e.y)<30){
            spawnExp(e.x,e.y);e.on=false;
            if(--lives<=0) state=OVER;
        }
    }

    for(auto& a:ast){
        if(!a.on) continue;
        a.y-=a.spd;
        a.rot+=a.rotSpd;
        if(a.y<-50){a.on=false;continue;}
        for(auto& b:bul) if(b.on&&dist(b.x,b.y,a.x,a.y)<a.r){
            b.on=false;spawnExp(a.x,a.y);a.on=false;score+=50;
        }
        if(!shieldOn&&dist(sx,sy,a.x,a.y)<a.r+18){
            spawnExp(a.x,a.y);a.on=false;
            if(--lives<=0) state=OVER;
        }
    }

    if(!alive){ wave++;score+=250;spawnWave();if(wave%2==0)spawnAst(); }

    shieldPulse++;
    if(shieldOn>0)  shieldOn--;
    if(shieldCool>0)shieldCool--;

    for(auto& ex:exps){
        if(!ex.on) continue;
        ex.scale+=0.06f; ex.alpha-=0.03f;
        if(ex.alpha<=0){ex.on=false;continue;}
        for(auto& p:ex.pts){p.x+=p.vx;p.y+=p.vy;p.vy-=0.05f;p.life-=0.025f;}
    }

    glutPostRedisplay();
}

// ── GLUT callbacks
void display(){
    glClear(GL_COLOR_BUFFER_BIT);

    if(state==MENU){drawMenu();glutSwapBuffers();return;}

    glBegin(GL_QUADS);
        glColor3f(0,0,0.05f);    glVertex2f(0,0);glVertex2f(W,0);
        glColor3f(0.02f,0,0.10f);glVertex2f(W,H);glVertex2f(0,H);
    glEnd();

    drawStars();drawPlanet();drawExps();
    for(auto& a:ast) if(a.on) drawAst(a);
    for(auto& e:ene) if(e.on) drawEnemy(e);
    for(auto& b:bul) if(b.on) drawBullet(b);
    if(shieldOn>0) drawShield(sx,sy);
    drawShip(sx,sy,sang,sscale);
    drawHUD();

    if(state==PAUSE){
        glColor4f(0,0,0,0.55f);
        glBegin(GL_QUADS);
            glVertex2f(0,0);glVertex2f(W,0);glVertex2f(W,H);glVertex2f(0,H);
        glEnd();
        glColor3f(1.0f,0.9f,0.3f);
        txt(365,370,"PAUSED",GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.8f,0.8f,0.9f);
        txt(340,335,"Press P to Resume",GLUT_BITMAP_HELVETICA_18);
    }

    if(state==OVER) drawOver();
    glutSwapBuffers();
}

void reshape(int w,int h){
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();
    gluOrtho2D(0,W,0,H);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();
}

void keyDown(unsigned char k,int,int){
    if(k==27) exit(0);
    if(k==13&&state==MENU) initGame();
    if(k=='r'||k=='R') initGame();
    if(k=='p'||k=='P'){
        if(state==PLAY) state=PAUSE;
        else if(state==PAUSE) state=PLAY;
    }
    if(k==' '&&state==PLAY) fire();
    if((k=='s'||k=='S')&&state==PLAY&&!shieldCool){shieldOn=90;shieldCool=300;}
    if(k=='w'||k=='W') mU=1;
    if(k=='a'||k=='A') mL=1;
    if(k=='d'||k=='D') mR=1;
    if(k=='x'||k=='X') mD=1;
}

void keyUp(unsigned char k,int,int){
    if(k=='w'||k=='W') mU=0;
    if(k=='a'||k=='A') mL=0;
    if(k=='d'||k=='D') mR=0;
    if(k=='x'||k=='X') mD=0;
}

void spDown(int k,int,int){
    if(k==GLUT_KEY_UP)   mU=1;
    if(k==GLUT_KEY_DOWN) mD=1;
    if(k==GLUT_KEY_LEFT) mL=1;
    if(k==GLUT_KEY_RIGHT)mR=1;
}

void spUp(int k,int,int){
    if(k==GLUT_KEY_UP)   mU=0;
    if(k==GLUT_KEY_DOWN) mD=0;
    if(k==GLUT_KEY_LEFT) mL=0;
    if(k==GLUT_KEY_RIGHT)mR=0;
}

void timer(int){update();glutTimerFunc(16,timer,0);}

int main(int argc,char** argv){
    srand(time(0));
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(W,H);
    glutInitWindowPosition(100,60);
    glutCreateWindow("PARTH&SPACE");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0,0,0.05f,1);

    initStars();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(spDown);
    glutSpecialUpFunc(spUp);
    glutTimerFunc(16,timer,0);
    glutMainLoop();
}