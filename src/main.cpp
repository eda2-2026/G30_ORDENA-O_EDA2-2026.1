// =============================================================================
// PRIORITIZER ATC — Bubble Sort vs Selection Sort
// Autor: patrickacs
// Projeto EDA 2 — Estruturas de Dados e Algoritmos
// Prioridade = distância. Mais perto = pousa primeiro.
// Selection acha o mínimo rápido (salva). Bubble joga máximo pro fim (crash).
// =============================================================================
#include <patrickacs.h>
#include <raymath.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <cstring>

const int SW=1280, SH=720;
const float RCX=(SW-380)/2.0f, RCY=SH/2.0f, RR=310.0f, RP=22.0f;
const int NV=20;

enum CorE{C_N,C_CMP,C_PIV,C_OK,C_CRASH,C_LAND};
Color CorDe(CorE e){
    switch(e){
        case C_CMP:return{255,200,0,255};case C_PIV:return{255,60,90,255};
        case C_OK:return{40,255,120,255};case C_CRASH:return{255,30,30,255};
        case C_LAND:return{40,255,120,150};default:return{0,210,255,255};
    }
}

// Explosão
struct Part{Vector2 p,v;float life;Color c;};
struct Boom{float x,y,t;std::vector<Part>ps;};
std::vector<Boom> booms;
void MkBoom(float x,float y){
    Boom b;b.x=x;b.y=y;b.t=2;
    for(int i=0;i<35;i++){
        Part p;p.p={x,y};float a=(rand()%360)*DEG2RAD;float s=50+rand()%100;
        p.v={cosf(a)*s,sinf(a)*s};p.life=1;
        p.c=(rand()%2)?Color{255,80,20,255}:Color{255,200,0,255};
        b.ps.push_back(p);
    }booms.push_back(b);
}
void UpdBoom(float dt){
    for(auto&b:booms){b.t-=dt;for(auto&p:b.ps){
        p.p.x+=p.v.x*dt;p.p.y+=p.v.y*dt;p.v.x*=.96f;p.v.y*=.96f;
        p.life-=dt*1.2f;if(p.life<0)p.life=0;}}
    booms.erase(std::remove_if(booms.begin(),booms.end(),
        [](const Boom&b){return b.t<=0;}),booms.end());
}
void DrBoom(){
    for(auto&b:booms){for(auto&p:b.ps){if(p.life<=0)continue;
        unsigned char a=(unsigned char)(p.life*255);
        DrawCircle((int)p.p.x,(int)p.p.y,2+p.life*3,{p.c.r,p.c.g,p.c.b,a});}
        if(b.t>1.5f){float f=(b.t-1.5f)*2;
        DrawCircleGradient((int)b.x,(int)b.y,30*f,Fade({255,150,0,255},f*.5f),Fade({255,0,0,255},0));}}
}

struct Voo{
    int id;char cs[7];
    float ang,raio,vel;
    float distInicial;
    CorE est;bool chegou,ok;
};
struct Log{int id;char cs[7];bool ok;};

std::vector<Voo> frota;
std::vector<Log> logs;

enum Modo{M_LIVRE,M_BUBBLE,M_SEL};
Modo modo=M_LIVRE;
const char*nModo[]={"AGUARDANDO","BUBBLE SORT","SELECTION SORT"};
float dAnim=0.08f;double tU=0;
int nComp=0,nTroc=0;bool sDone=false;float tDec=0;
bool pausado=true;
int bI=0,bJ=0,sI=0,sMin=0,sJ=0;

void Gerar(){
    frota.clear();logs.clear();booms.clear();
    srand((unsigned)time(0));
    const char*pf[]={"AZU","GOL","TAM","LAT","VRG","PTB"};
    for(int i=0;i<NV;i++){
        Voo v;v.id=100+rand()%900;
        snprintf(v.cs,7,"%s%02d",pf[rand()%6],v.id%100);
        v.ang=(rand()%3600)/10.0f*DEG2RAD;
        v.raio=80+(float)(rand()%(int)(RR-90));
        v.vel=3.0f+(rand()%8);
        v.distInicial=v.raio; // CRITÉRIO: distância
        v.est=C_N;v.chegou=false;v.ok=false;
        frota.push_back(v);
    }
    modo=M_LIVRE;sDone=false;nComp=0;nTroc=0;tDec=0;
}

void RVis(){for(auto&v:frota)if(!v.chegou)v.est=C_N;}

// Checa se idx está na região JÁ ORDENADA pelo algoritmo
bool EstaOrdenado(int idx){
    if(sDone) return true;
    if(modo==M_SEL) return idx<sI;    // Selection ordena do INÍCIO
    if(modo==M_BUBBLE) return idx>=(NV-bI); // Bubble ordena do FIM
    return false;
}

// --- Bubble Sort ---
void InitBub(){RVis();modo=M_BUBBLE;bI=0;bJ=0;sDone=false;nComp=0;nTroc=0;tU=GetTime();}
void StepBub(){
    if(sDone)return;if(GetTime()-tU<dAnim)return;tU=GetTime();
    RVis();
    for(int k=NV-bI;k<NV;k++)if(!frota[k].chegou)frota[k].est=C_OK;
    if(bI<NV-1){
        if(bJ<NV-bI-1){
            if(!frota[bJ].chegou)frota[bJ].est=C_CMP;
            if(!frota[bJ+1].chegou)frota[bJ+1].est=C_CMP;
            nComp++;
            if(frota[bJ].distInicial>frota[bJ+1].distInicial){
                std::swap(frota[bJ],frota[bJ+1]);nTroc++;}
            bJ++;
        }else{bJ=0;bI++;}
    }else{sDone=true;for(auto&v:frota)if(!v.chegou)v.est=C_OK;}
}

// --- Selection Sort ---
void InitSel(){RVis();modo=M_SEL;sI=0;sMin=0;sJ=1;sDone=false;nComp=0;nTroc=0;tU=GetTime();}
void StepSel(){
    if(sDone)return;if(GetTime()-tU<dAnim)return;tU=GetTime();
    RVis();
    for(int k=0;k<sI;k++)if(!frota[k].chegou)frota[k].est=C_OK;
    if(sI<NV-1){
        if(sJ<NV){
            if(!frota[sMin].chegou)frota[sMin].est=C_PIV;
            if(!frota[sJ].chegou)frota[sJ].est=C_CMP;
            nComp++;
            if(frota[sJ].distInicial<frota[sMin].distInicial)sMin=sJ;
            sJ++;
        }else{
            if(sMin!=sI){std::swap(frota[sI],frota[sMin]);nTroc++;}
            if(!frota[sI].chegou)frota[sI].est=C_OK;
            sI++;sMin=sI;sJ=sI+1;
        }
    }else{sDone=true;for(auto&v:frota)if(!v.chegou)v.est=C_OK;}
}

// --- Radar ---
void DrRadar(){
    DrawCircleGradient((int)RCX,(int)RCY,RR,{0,20,30,255},{5,5,10,0});
    for(int i=1;i<=4;i++)DrawCircleLines((int)RCX,(int)RCY,RR*i/4.0f,{0,150,200,35});
    for(int i=0;i<12;i++){float a=i*30*DEG2RAD;
        DrawLineEx({RCX,RCY},{RCX+cosf(a)*RR,RCY+sinf(a)*RR},1,{0,150,200,20});}
    float sw=GetTime()*1.5f;
    for(int i=0;i<50;i++){float a=sw-i*.012f,al=1-i/50.0f;
        DrawLineEx({RCX,RCY},{RCX+cosf(a)*RR,RCY+sinf(a)*RR},2,Fade({0,255,150,255},al*.3f));}
    DrawCircleLines((int)RCX,(int)RCY,RP,{255,255,255,60});
    DrawCircle((int)RCX,(int)RCY,4,{0,255,150,200});
    DrawText("PISTA",(int)RCX-14,(int)RCY+8,8,{0,255,150,100});
}

// --- Update aviões ---
void UpdAv(float dt){
    for(int i=0;i<(int)frota.size();i++){
        Voo&v=frota[i];if(v.chegou)continue;
        v.raio-=v.vel*dt;
        v.ang+=.003f*dt*60;
        if(v.raio<=RP){
            v.raio=RP;v.chegou=true;
            float px=RCX+cosf(v.ang)*v.raio, py=RCY+sinf(v.ang)*v.raio;
            bool safe=EstaOrdenado(i);
            v.ok=safe;v.est=safe?C_LAND:C_CRASH;
            Log l;l.id=v.id;strncpy(l.cs,v.cs,7);l.ok=safe;
            logs.push_back(l);
            if(!safe)MkBoom(px,py);
        }
    }
}

// --- Draw aviões ---
void DrAv(){
    for(auto&v:frota){
        float px=RCX+cosf(v.ang)*v.raio, py=RCY+sinf(v.ang)*v.raio;
        if(v.chegou&&v.est==C_CRASH)continue;
        Color c=CorDe(v.est);
        if(v.chegou&&v.est==C_LAND){DrawCircle((int)px,(int)py,3,Fade(c,.4f));continue;}
        // Linha sutil do avião até a pista (centro)
        DrawLineEx({px,py},{RCX,RCY},1.0f,Fade(c,0.08f));
        // Distância no ponto médio da linha
        float mx=(px+RCX)/2, my=(py+RCY)/2;
        DrawText(TextFormat("%.0f",v.raio),(int)mx-8,(int)my-4,8,Fade(c,0.25f));
        // Blip
        DrawCircleGradient((int)px,(int)py,14,Fade(c,.35f),Fade(c,0));
        DrawCircle((int)px,(int)py,3,WHITE);DrawCircle((int)px,(int)py,2,c);
        if(v.est==C_CMP||v.est==C_PIV||v.est==C_OK){
            float p=7+sinf(GetTime()*8)*2;DrawCircleLines((int)px,(int)py,p,Fade(c,.5f));}
        DrawText(TextFormat("%d",v.id),(int)px+8,(int)py-10,9,c);
    }
}

// --- Dashboard ---
void DrDash(){
    int dx=SW-380,w=380;
    DrawRectangle(dx,0,w,SH,{10,14,20,245});
    DrawLineEx({(float)dx,0},{(float)dx,(float)SH},2,{0,150,255,120});
    DrawRectangle(dx,0,w,60,{0,80,180,25});
    DrawText("PRIORITIZER ATC",dx+20,12,20,{0,220,255,255});
    DrawText("TRIAGEM POR DISTANCIA (MAIS PERTO = POUSA PRIMEIRO)",dx+20,36,8,{0,220,255,130});
    DrawLine(dx,60,dx+w,60,{0,150,255,80});

    int sy=72;
    DrawText(TextFormat("MODO: %s",nModo[modo]),dx+20,sy,10,YELLOW);
    DrawText(TextFormat("DELAY: %.3fs | COMP: %d | TROCAS: %d",dAnim,nComp,nTroc),dx+20,sy+14,9,LIGHTGRAY);
    DrawText(TextFormat("TEMPO: %.1fs",tDec),dx+20,sy+28,9,{180,180,180,255});

    // Explicação do algoritmo
    int ey=sy+46;
    if(modo==M_BUBBLE){
        DrawText("BUBBLE: Empurra o MAIOR pro final.",dx+20,ey,9,{255,200,0,200});
        DrawText("O mais perto fica preso no meio -> CRASH!",dx+20,ey+12,9,{255,100,100,180});
    }else if(modo==M_SEL){
        DrawText("SELECTION: Acha o MENOR primeiro.",dx+20,ey,9,{255,200,0,200});
        DrawText("O mais perto eh priorizado rapido -> POUSO!",dx+20,ey+12,9,{40,255,120,180});
    }

    // Legenda
    int ly=ey+30;
    DrawCircle(dx+25,ly,4,CorDe(C_N));DrawText("Neutro",dx+35,ly-5,9,LIGHTGRAY);
    DrawCircle(dx+105,ly,4,CorDe(C_CMP));DrawText("Comparando",dx+115,ly-5,9,LIGHTGRAY);
    DrawCircle(dx+210,ly,4,CorDe(C_PIV));DrawText("Candidato",dx+220,ly-5,9,LIGHTGRAY);
    DrawCircle(dx+25,ly+16,4,CorDe(C_OK));DrawText("Ordenado",dx+35,ly+11,9,LIGHTGRAY);
    DrawCircle(dx+105,ly+16,4,{40,255,120,150});DrawText("Pousou",dx+115,ly+11,9,LIGHTGRAY);
    DrawCircle(dx+210,ly+16,4,CorDe(C_CRASH));DrawText("CRASH",dx+220,ly+11,9,LIGHTGRAY);
    DrawLine(dx+15,ly+32,dx+w-15,ly+32,{255,255,255,15});

    // Tabela
    int ty=ly+40;
    DrawText("FILA DE PRIORIDADE (por distancia)",dx+20,ty,10,GRAY);
    int oy=ty+18;
    for(int i=0;i<NV;i++){
        int col=i%2,row=i/2;
        int xp=dx+15+col*178,yp=oy+row*18;
        if(yp>SH-170)break;
        Color c=CorDe(frota[i].est);
        if(row%2==0)DrawRectangle(xp,yp-2,173,16,{255,255,255,4});
        const char*st=frota[i].chegou?(frota[i].ok?"OK":"XX"):"";
        DrawText(TextFormat("[%02d] %d %s %.0f %s",i,frota[i].id,frota[i].cs,frota[i].distInicial,st),xp+2,yp,9,c);
    }

    // LOG
    int lY=SH-160;
    DrawLine(dx+15,lY-5,dx+w-15,lY-5,{255,255,255,15});
    DrawText("REGISTRO",dx+20,lY,10,GRAY);
    int pou=0,cra=0;
    for(auto&l:logs){if(l.ok)pou++;else cra++;}
    DrawText(TextFormat("POUSOS: %d",pou),dx+20,lY+16,12,{40,255,120,255});
    DrawText(TextFormat("CRASHES: %d",cra),dx+160,lY+16,12,{255,50,50,255});
    int ls=(int)logs.size()-5;if(ls<0)ls=0;
    for(int i=ls;i<(int)logs.size();i++){
        int r=i-ls;Color lc=logs[i].ok?Color{40,255,120,200}:Color{255,50,50,200};
        DrawText(TextFormat("%s %d %s",logs[i].ok?"[POUSO]":"[CRASH]",logs[i].id,logs[i].cs),
            dx+20,lY+34+r*14,9,lc);
    }

    // Controles
    int cy=SH-55;
    DrawLine(dx+15,cy-8,dx+w-15,cy-8,{255,255,255,15});
    DrawText("[1] Bubble Sort  [2] Selection Sort  [R] Reset  [UP/DOWN] Vel.  [SPACE] Pausar",
        dx+20,cy,9,{100,100,100,200});
}

int main(){
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SW,SH,"Prioritizer ATC - Bubble Sort vs Selection Sort");
    SetTargetFPS(60);Gerar();
    while(!WindowShouldClose()){
        float dt=GetFrameTime();if(!pausado)tDec+=dt;
        if(IsKeyPressed(KEY_R))Gerar();
        if(IsKeyPressed(KEY_ONE)&&modo==M_LIVRE)InitBub();
        if(IsKeyPressed(KEY_TWO)&&modo==M_LIVRE)InitSel();
        if(IsKeyPressed(KEY_UP)){dAnim-=.02f;if(dAnim<.01f)dAnim=.01f;}
        if(IsKeyPressed(KEY_DOWN)){dAnim+=.02f;if(dAnim>1)dAnim=1;}
        if(IsKeyPressed(KEY_SPACE))pausado=!pausado;
        if(!pausado){
            if(modo==M_BUBBLE)StepBub();
            if(modo==M_SEL)StepSel();
            UpdAv(dt);UpdBoom(dt);
        }
        BeginDrawing();ClearBackground({5,5,8,255});
        DrRadar();DrAv();DrBoom();DrDash();
        EndDrawing();
    }
    CloseWindow();return 0;
}