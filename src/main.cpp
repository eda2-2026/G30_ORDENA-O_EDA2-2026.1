// =============================================================================
// PRIORITIZER ATC — Bubble | Selection | Merge | Counting Sort
// Autor: patrickacs, Vinicius Castelo
// Projeto EDA 2 — Estruturas de Dados e Algoritmos
// Prioridade = distância. Mais perto = pousa primeiro.
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
#include <cmath>

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

// Explosao
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

enum Modo{M_LIVRE,M_BUBBLE,M_SEL,M_MERGE,M_COUNT};
Modo modo=M_LIVRE;
const char*nModo[]={"AGUARDANDO","BUBBLE SORT","SELECTION SORT","MERGE SORT","COUNTING SORT"};
float dAnim=0.08f;double tU=0;
int nComp=0,nTroc=0;bool sDone=false;float tDec=0;
bool pausado=true;

// Bubble Sort state
int bI=0,bJ=0;

// Selection Sort state
int sI=0,sMin=0,sJ=0;

// Merge Sort state (bottom-up iterativo)
int mWidth=1,mLeft=0,mI=0,mJ=0,mK=0,mMid=0,mRight=0;
bool mInMerge=false;
std::vector<Voo> mTmp;

// Counting Sort state
const int CNT_MAX=320;
int cntArr[CNT_MAX+1];
Voo cntOutput[NV];
int cPhase=0; // 0=contando  1=prefix sum  2=colocando
int cIdx=0,cPrefixIdx=0;

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
        v.distInicial=v.raio;
        v.est=C_N;v.chegou=false;v.ok=false;
        frota.push_back(v);
    }
    modo=M_LIVRE;sDone=false;nComp=0;nTroc=0;tDec=0;
}

void RVis(){for(auto&v:frota)if(!v.chegou)v.est=C_N;}

bool EstaOrdenado(int idx){
    if(sDone) return true;
    if(modo==M_SEL)    return idx<sI;
    if(modo==M_BUBBLE) return idx>=(NV-bI);
    // Merge e Counting so consideram seguro apos concluir (sDone)
    return false;
}

// =============================================================================
// BUBBLE SORT
// =============================================================================
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

// =============================================================================
// SELECTION SORT
// =============================================================================
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

// =============================================================================
// MERGE SORT (bottom-up iterativo, um elemento por passo)
// =============================================================================
void InitMerge(){
    RVis();modo=M_MERGE;sDone=false;nComp=0;nTroc=0;
    mWidth=1;mLeft=0;mInMerge=false;
    tU=GetTime();
}

void StepMerge(){
    if(sDone)return;if(GetTime()-tU<dAnim)return;tU=GetTime();
    RVis();

    if(!mInMerge){
        // Avanca mLeft ate achar par valido ou fim de passagem
        while(mLeft<NV && mLeft+mWidth>=NV) mLeft+=2*mWidth;

        if(mLeft>=NV){
            mWidth*=2; mLeft=0;
            if(mWidth>=NV){
                sDone=true;
                for(auto&v:frota)if(!v.chegou)v.est=C_OK;
                return;
            }
            while(mLeft<NV && mLeft+mWidth>=NV) mLeft+=2*mWidth;
            if(mLeft>=NV) return;
        }

        mMid  =mLeft+mWidth;
        mRight=std::min(mLeft+2*mWidth,NV);
        mTmp.assign(frota.begin()+mLeft,frota.begin()+mRight);
        mI=0; mJ=mWidth; mK=mLeft;
        mInMerge=true;
    }

    // Destaca janela atual
    for(int k=mLeft;k<mRight;k++)if(!frota[k].chegou)frota[k].est=C_CMP;

    int leftEnd =(int)mTmp.size()<mWidth?(int)mTmp.size():mWidth;
    int rightEnd=(int)mTmp.size();

    if(mI<leftEnd && mJ<rightEnd){
        nComp++;
        if(mTmp[mI].distInicial<=mTmp[mJ].distInicial){
            frota[mK]=mTmp[mI];frota[mK].est=C_PIV;mI++;
        }else{
            frota[mK]=mTmp[mJ];frota[mK].est=C_PIV;mJ++;nTroc++;
        }
        mK++;
    }else if(mI<leftEnd){
        frota[mK]=mTmp[mI];frota[mK].est=C_OK;mI++;mK++;
    }else if(mJ<rightEnd){
        frota[mK]=mTmp[mJ];frota[mK].est=C_OK;mJ++;mK++;
    }else{
        mInMerge=false;
        mLeft+=2*mWidth;
    }
}

// =============================================================================
// COUNTING SORT (passo a passo em 3 fases)
// =============================================================================
void InitCount(){
    RVis();modo=M_COUNT;sDone=false;nComp=0;nTroc=0;
    memset(cntArr,0,sizeof(cntArr));
    cPhase=0;cIdx=0;cPrefixIdx=1;
    tU=GetTime();
}

void StepCount(){
    if(sDone)return;if(GetTime()-tU<dAnim)return;tU=GetTime();
    RVis();

    if(cPhase==0){
        // Fase 1: conta ocorrencias
        if(cIdx<NV){
            int val=(int)frota[cIdx].distInicial;
            if(val>CNT_MAX)val=CNT_MAX;
            cntArr[val]++;
            if(!frota[cIdx].chegou)frota[cIdx].est=C_CMP;
            nComp++;
            cIdx++;
        }else{
            for(int i=0;i<NV;i++)cntOutput[i]=frota[i];
            cPhase=1;cPrefixIdx=1;
        }
    }else if(cPhase==1){
        // Fase 2: prefix sum
        if(cPrefixIdx<=CNT_MAX){
            cntArr[cPrefixIdx]+=cntArr[cPrefixIdx-1];
            cPrefixIdx++;
        }else{
            cPhase=2;cIdx=NV-1;
        }
    }else if(cPhase==2){
        // Fase 3: posiciona cada elemento
        if(cIdx>=0){
            int val=(int)cntOutput[cIdx].distInicial;
            if(val>CNT_MAX)val=CNT_MAX;
            int pos=--cntArr[val];
            if(pos>=0&&pos<NV){
                frota[pos]=cntOutput[cIdx];
                if(!frota[pos].chegou)frota[pos].est=C_PIV;
                nTroc++;
            }
            cIdx--;
        }else{
            sDone=true;
            for(auto&v:frota)if(!v.chegou)v.est=C_OK;
        }
    }
}

// =============================================================================
// RADAR
// =============================================================================
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

// =============================================================================
// UPDATE / DRAW AVIOES
// =============================================================================
void UpdAv(float dt){
    for(int i=0;i<(int)frota.size();i++){
        Voo&v=frota[i];if(v.chegou)continue;
        v.raio-=v.vel*dt;
        v.ang+=.003f*dt*60;
        if(v.raio<=RP){
            v.raio=RP;v.chegou=true;
            float px=RCX+cosf(v.ang)*v.raio,py=RCY+sinf(v.ang)*v.raio;
            bool safe=EstaOrdenado(i);
            v.ok=safe;v.est=safe?C_LAND:C_CRASH;
            Log l;l.id=v.id;strncpy(l.cs,v.cs,7);l.ok=safe;
            logs.push_back(l);
            if(!safe)MkBoom(px,py);
        }
    }
}

void DrAv(){
    for(auto&v:frota){
        float px=RCX+cosf(v.ang)*v.raio,py=RCY+sinf(v.ang)*v.raio;
        if(v.chegou&&v.est==C_CRASH)continue;
        Color c=CorDe(v.est);
        if(v.chegou&&v.est==C_LAND){DrawCircle((int)px,(int)py,3,Fade(c,.4f));continue;}
        DrawLineEx({px,py},{RCX,RCY},1.0f,Fade(c,0.08f));
        float mx=(px+RCX)/2,my=(py+RCY)/2;
        DrawText(TextFormat("%.0f",v.raio),(int)mx-8,(int)my-4,8,Fade(c,0.25f));
        DrawCircleGradient((int)px,(int)py,14,Fade(c,.35f),Fade(c,0));
        DrawCircle((int)px,(int)py,3,WHITE);DrawCircle((int)px,(int)py,2,c);
        if(v.est==C_CMP||v.est==C_PIV||v.est==C_OK){
            float p=7+sinf(GetTime()*8)*2;DrawCircleLines((int)px,(int)py,p,Fade(c,.5f));}
        DrawText(TextFormat("%d",v.id),(int)px+8,(int)py-10,9,c);
    }
}

// =============================================================================
// DASHBOARD
// =============================================================================
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

    // Explicacao do algoritmo
    int ey=sy+46;
    if(modo==M_BUBBLE){
        DrawText("BUBBLE: Empurra o MAIOR pro final.",dx+20,ey,9,{255,200,0,200});
        DrawText("O mais perto fica preso no meio -> CRASH!",dx+20,ey+12,9,{255,100,100,180});
    }else if(modo==M_SEL){
        DrawText("SELECTION: Acha o MENOR primeiro.",dx+20,ey,9,{255,200,0,200});
        DrawText("O mais perto eh priorizado rapido -> POUSO!",dx+20,ey+12,9,{40,255,120,180});
    }else if(modo==M_MERGE){
        DrawText("MERGE: Divide e mescla sublistas ordenadas.",dx+20,ey,9,{255,200,0,200});
        DrawText("O(n log n) — ordena em bloco, sem pouso parcial.",dx+20,ey+12,9,{100,180,255,180});
        DrawText(TextFormat("Bloco: %d  |  Inicio: %d",mWidth,mLeft),dx+20,ey+24,9,{100,180,255,140});
    }else if(modo==M_COUNT){
        const char*fases[3]={"Fase 1/3: Contando distancias...","Fase 2/3: Acumulando contagens...","Fase 3/3: Posicionando avioes..."};
        DrawText("COUNTING: Ordena por contagem direta.",dx+20,ey,9,{255,200,0,200});
        DrawText(fases[cPhase<3?cPhase:2],dx+20,ey+12,9,{180,100,255,200});
        DrawText("O(n+k) — zero comparacoes!",dx+20,ey+24,9,{180,100,255,140});
    }

    // Legenda
    int ly=ey+38;
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
    DrawText("[1] Bubble  [2] Selection  [3] Merge  [4] Counting",dx+20,cy,9,{100,100,100,200});
    DrawText("[R] Reset   [UP/DOWN] Velocidade   [SPACE] Pausar",dx+20,cy+12,9,{100,100,100,200});
}

// =============================================================================
// MAIN
// =============================================================================
int main(){
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(SW,SH,"Prioritizer ATC - Bubble | Selection | Merge | Counting Sort");
    SetWindowMinSize(640,360);
    SetTargetFPS(60);
    Gerar();

    // Canvas fixo 1280x720 — escala proporcionalmente com a janela
    RenderTexture2D canvas=LoadRenderTexture(SW,SH);
    SetTextureFilter(canvas.texture,TEXTURE_FILTER_BILINEAR);

    while(!WindowShouldClose()){
        float dt=GetFrameTime();if(!pausado)tDec+=dt;

        if(IsKeyPressed(KEY_R))    Gerar();
        if(IsKeyPressed(KEY_ONE)  &&modo==M_LIVRE)InitBub();
        if(IsKeyPressed(KEY_TWO)  &&modo==M_LIVRE)InitSel();
        if(IsKeyPressed(KEY_THREE)&&modo==M_LIVRE)InitMerge();
        if(IsKeyPressed(KEY_FOUR) &&modo==M_LIVRE)InitCount();
        if(IsKeyPressed(KEY_UP))  {dAnim-=.02f;if(dAnim<.01f)dAnim=.01f;}
        if(IsKeyPressed(KEY_DOWN)){dAnim+=.02f;if(dAnim>1)dAnim=1;}
        if(IsKeyPressed(KEY_SPACE))pausado=!pausado;

        if(!pausado){
            if(modo==M_BUBBLE)StepBub();
            if(modo==M_SEL)   StepSel();
            if(modo==M_MERGE) StepMerge();
            if(modo==M_COUNT) StepCount();
            UpdAv(dt);UpdBoom(dt);
        }

        // Renderiza no canvas fixo
        BeginTextureMode(canvas);
            ClearBackground({5,5,8,255});
            DrRadar();DrAv();DrBoom();DrDash();
        EndTextureMode();

        // Escala canvas para a janela mantendo proporcao 16:9
        int ww=GetScreenWidth(),wh=GetScreenHeight();
        float scale=fminf((float)ww/SW,(float)wh/SH);
        float dw=SW*scale,dh=SH*scale;
        float ox=(ww-dw)/2.0f,oy=(wh-dh)/2.0f;
        Rectangle src={0,0,(float)SW,-(float)SH};
        Rectangle dst={ox,oy,dw,dh};

        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(canvas.texture,src,dst,{0,0},0,WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(canvas);
    CloseWindow();
    return 0;
}