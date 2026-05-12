// =============================================================================
// PRIORITIZER ATC — Simulador de Triagem de Pouso com 11 Algoritmos de Ordenacao
// Autores: Patrick Anderson (patrickacs), Vinicius Castelo
// Disciplina: EDA 2 — Estruturas de Dados e Algoritmos (2026.1)
//
// CONCEITO: Avioes se aproximam da pista (centro do radar). O sistema ordena
// a fila de pouso por DISTANCIA — quem esta mais perto pousa primeiro.
// Se o algoritmo nao ordenar a tempo, o aviao chega sem priorizacao e CRASHA.
//
// ALGORITMOS IMPLEMENTADOS:
//   Comparativos:     Bubble, Selection, Insertion, Shell, Merge, Quick, Heap
//   Nao-comparativos: Counting, Radix LSD, Radix MSD, Bucket
//
// CONTROLES: [1]-[0,-] seleciona algoritmo | [R] reset | [SPACE] pausa
//            [UP/DOWN] velocidade | [TAB] benchmark | [B] rodar benchmark
// =============================================================================

#include <patrickacs.h>     // raylib.h renomeado (renderizacao grafica 2D)
#include <raymath.h>        // funcoes matematicas do raylib (DEG2RAD, etc)
#include <vector>
#include <string>
#include <algorithm>        // std::swap, std::sort, std::is_sorted, std::min
#include <cstdlib>          // rand, srand
#include <ctime>            // time (seed do random)
#include <cstdio>           // snprintf
#include <cstring>          // memset, strncpy
#include <cmath>            // cosf, sinf, sqrtf, fminf
#include <chrono>           // medicao de tempo no benchmark
#include <functional>       // std::function (usado no Radix MSD recursivo)

// =============================================================================
// CONSTANTES GLOBAIS
// =============================================================================
const int SW = 1280, SH = 720;         // resolucao do canvas interno (16:9)
const float RCX = (SW-380)/2.0f;       // centro X do radar (descontando painel lateral)
const float RCY = SH/2.0f;            // centro Y do radar
const float RR = 310.0f;              // raio maximo do radar
const float RP = 22.0f;               // raio da pista (zona de pouso)
const int NV = 20;                     // numero de avioes na simulacao

// =============================================================================
// SISTEMA DE CORES — cada estado do aviao tem uma cor distinta no radar
// =============================================================================
enum CorE { C_N, C_CMP, C_PIV, C_OK, C_CRASH, C_LAND };
//   C_N     = Neutro (ciano)        — aviao aguardando, sem interacao
//   C_CMP   = Comparando (amarelo)  — sendo comparado neste passo do algoritmo
//   C_PIV   = Candidato (rosa)      — pivot ou elemento-chave do algoritmo
//   C_OK    = Ordenado (verde)      — ja esta na posicao correta
//   C_CRASH = Crash (vermelho)      — colidiu ou nao foi priorizado a tempo
//   C_LAND  = Pousou (verde claro)  — aterrissou com seguranca

Color CorDe(CorE e) {
    switch(e) {
        case C_CMP:   return {255, 200, 0,   255};  // amarelo
        case C_PIV:   return {255, 60,  90,  255};  // rosa/vermelho
        case C_OK:    return {40,  255, 120, 255};  // verde
        case C_CRASH: return {255, 30,  30,  255};  // vermelho
        case C_LAND:  return {40,  255, 120, 150};  // verde translucido
        default:      return {0,   210, 255, 255};  // ciano (neutro)
    }
}

// =============================================================================
// SISTEMA DE PARTICULAS — explosoes visuais quando um aviao crasha
// Cada explosao gera 35 particulas com direcao aleatoria que desaceleram
// e desvanecem ao longo de ~1 segundo (efeito visual puramente estetico)
// =============================================================================
struct Part { Vector2 p, v; float life; Color c; };  // particula individual
struct Boom { float x, y, t; std::vector<Part> ps; }; // explosao (conjunto de particulas)
std::vector<Boom> booms;

// Cria uma nova explosao na posicao (x,y) com 35 particulas
void MkBoom(float x, float y) {
    Boom b; b.x = x; b.y = y; b.t = 2;
    for (int i = 0; i < 35; i++) {
        Part p; p.p = {x, y};
        float a = (rand() % 360) * DEG2RAD;
        float s = 50 + rand() % 100;
        p.v = {cosf(a)*s, sinf(a)*s};
        p.life = 1;
        p.c = (rand() % 2) ? Color{255,80,20,255} : Color{255,200,0,255};
        b.ps.push_back(p);
    }
    booms.push_back(b);
}

// Atualiza fisica das particulas: move, desacelera (0.96x) e reduz vida
void UpdBoom(float dt) {
    for (auto& b : booms) {
        b.t -= dt;
        for (auto& p : b.ps) {
            p.p.x += p.v.x * dt;
            p.p.y += p.v.y * dt;
            p.v.x *= .96f;  // friccao/desaceleracao
            p.v.y *= .96f;
            p.life -= dt * 1.2f;
            if (p.life < 0) p.life = 0;
        }
    }
    // Remove explosoes que ja acabaram
    booms.erase(std::remove_if(booms.begin(), booms.end(),
        [](const Boom& b) { return b.t <= 0; }), booms.end());
}

// Renderiza todas as particulas ativas com alpha proporcional a vida restante
void DrBoom() {
    for (auto& b : booms) {
        for (auto& p : b.ps) {
            if (p.life <= 0) continue;
            unsigned char a = (unsigned char)(p.life * 255);
            DrawCircle((int)p.p.x, (int)p.p.y, 2 + p.life*3, {p.c.r, p.c.g, p.c.b, a});
        }
        // Flash inicial da explosao (primeiros 0.5s)
        if (b.t > 1.5f) {
            float f = (b.t - 1.5f) * 2;
            DrawCircleGradient((int)b.x, (int)b.y, 30*f,
                Fade({255,150,0,255}, f*.5f), Fade({255,0,0,255}, 0));
        }
    }
}

// =============================================================================
// ESTRUTURAS DE DADOS PRINCIPAIS
// =============================================================================

// Representa um voo/aviao no radar
struct Voo {
    int id;            // identificador numerico (ex: 342)
    char cs[7];        // callsign do voo (ex: "GOL42")
    float ang;         // angulo atual no radar (radianos)
    float raio;        // distancia atual ate a pista (diminui com o tempo)
    float vel;         // velocidade de aproximacao (pixels/segundo)
    float distInicial; // distancia no momento da geracao (CRITERIO DE ORDENACAO)
    CorE est;          // estado visual atual
    bool chegou;       // true se ja atingiu a pista
    bool ok;           // true se pousou com seguranca, false se crashou
};

// Registro de pouso/crash para o log do dashboard
struct Log { int id; char cs[7]; bool ok; };

// Converte distancia float em chave inteira para algoritmos nao-comparativos
// Multiplicacao por 10 preserva 1 casa decimal de precisao
int DistKey(float d) { return (int)(d * 10); }

std::vector<Voo> frota;   // todos os avioes ativos
std::vector<Log> logs;    // historico de pousos e crashes

// =============================================================================
// ESTADO GLOBAL DA SIMULACAO
// =============================================================================

// Modos de operacao — cada algoritmo tem seu proprio modo
enum Modo {
    M_LIVRE,   // aguardando selecao de algoritmo
    M_BUBBLE, M_SEL, M_INSERT, M_SHELL,
    M_MERGE, M_QUICK, M_HEAP,
    M_COUNT, M_RADLSD, M_RADMSD, M_BUCKET
};

Modo modo = M_LIVRE;
const char* nModo[] = {
    "AGUARDANDO", "BUBBLE SORT", "SELECTION SORT", "INSERTION SORT", "SHELL SORT",
    "MERGE SORT", "QUICK SORT", "HEAP SORT", "COUNTING SORT",
    "RADIX SORT LSD", "RADIX SORT MSD", "BUCKET SORT"
};

float dAnim = 0.08f;   // delay entre passos da animacao (segundos)
double tU = 0;          // timestamp do ultimo passo executado
int nComp = 0;          // contador de comparacoes do algoritmo atual
int nTroc = 0;          // contador de trocas/movimentacoes
bool sDone = false;     // true quando o algoritmo terminou
float tDec = 0;         // tempo decorrido desde o inicio do algoritmo
bool pausado = true;    // simulacao comeca pausada
bool showBench = false; // true = mostra tela de benchmark ao inves do radar

// ---------------------------------------------------------------------------
// Variaveis de estado de cada algoritmo (maquina de estados step-by-step)
// Cada algoritmo avanca UM PASSO por frame para permitir animacao visual
// ---------------------------------------------------------------------------

// BUBBLE SORT: bI = passada atual, bJ = posicao na passada
int bI = 0, bJ = 0;

// SELECTION SORT: sI = posicao sendo preenchida, sMin = indice do minimo, sJ = busca
int sI = 0, sMin = 0, sJ = 0;

// INSERTION SORT: insI = elemento sendo inserido, insJ = posicao de comparacao
int insI = 1, insJ = 0;
float insKey = 0;
Voo insKeyVoo;
bool insShifting = false;

// SHELL SORT: gaps decrescentes (sequencia de Knuth: 1, 4, 13, 40...)
int shGap = 1, shI = 0, shJ = 0;
float shKey = 0;
Voo shKeyVoo;
bool shShifting = false;

// MERGE SORT (bottom-up iterativo): mescla blocos de tamanho crescente
int mWidth = 1, mLeft = 0, mI = 0, mJ = 0, mK = 0, mMid = 0, mRight = 0;
bool mInMerge = false;
std::vector<Voo> mTmp;  // buffer temporario para merge

// QUICK SORT: pilha explicita substitui recursao
struct QFrame { int lo, hi; };
std::vector<QFrame> qStack;
int qLo = 0, qHi = 0, qPivIdx = 0, qI = 0, qJ = 0;
bool qInPartition = false;

// HEAP SORT: constroi max-heap e extrai maximo repetidamente
int hBuildIdx = 0, hEnd = 0;
bool hBuilding = true;
int hSiftRoot = 0;
bool hSifting = false;

// COUNTING SORT: ordena por contagem de ocorrencias (sem comparacoes)
const int CNT_MAX = 3200;       // valor maximo esperado de DistKey
int cntArr[CNT_MAX + 1];        // array de contagem
std::vector<Voo> cntOutput;     // copia dos dados para reposicionamento
int cPhase = 0, cIdx = 0, cPrefixIdx = 0;  // fase e indices da animacao

// RADIX SORT LSD: ordena digito a digito do MENOS para o MAIS significativo
int rlExp = 1, rlPhase = 0, rlIdx = 0;
int rlCount[10];                 // contagem para cada digito (0-9)
std::vector<Voo> rlOutput;

// RADIX SORT MSD: ordena digito a digito do MAIS para o MENOS significativo
struct RMFrame { int lo, hi, exp; };  // frame da pilha (simula recursao)
std::vector<RMFrame> rmStack;
int rmPhase = 0, rmIdx = 0;
RMFrame rmCur;
std::vector<std::vector<Voo>> rmBuck;  // 10 buckets (um por digito)

// BUCKET SORT: distribui em baldes por faixa de valor e ordena cada balde
int bktPhase = 0, bktIdx = 0, bktB = 0, bktI = 0;
std::vector<std::vector<Voo>> bktBuckets;
int bktCount = 0;

// =============================================================================
// BENCHMARK — compara performance real de todos os 11 algoritmos
// Testa com arrays de 100, 1000, 5000 e 10000 elementos
// Mede: comparacoes, trocas e tempo em milissegundos
// =============================================================================

struct BenchResult {
    const char* name;       // nome do algoritmo
    const char* complex;    // complexidade teorica
    long long comps, swaps; // contadores
    double timeMs;          // tempo de execucao
    bool valid;             // false se foi pulado (muito lento)
};
std::vector<BenchResult> benchResults;
bool benchRan = false;

// Versoes standalone de cada algoritmo para benchmark (sem animacao)
// Recebem vetor por referencia e contadores de comparacoes/trocas

void BenchBubble(std::vector<float>& a, long long& c, long long& s) {
    int n = a.size();
    for (int i = 0; i < n-1; i++)
        for (int j = 0; j < n-i-1; j++) {
            c++;
            if (a[j] > a[j+1]) { std::swap(a[j], a[j+1]); s++; }
        }
}

void BenchSelection(std::vector<float>& a, long long& c, long long& s) {
    int n = a.size();
    for (int i = 0; i < n-1; i++) {
        int m = i;
        for (int j = i+1; j < n; j++) { c++; if (a[j] < a[m]) m = j; }
        if (m != i) { std::swap(a[i], a[m]); s++; }
    }
}

void BenchInsertion(std::vector<float>& a, long long& c, long long& s) {
    int n = a.size();
    for (int i = 1; i < n; i++) {
        float k = a[i]; int j = i-1;
        while (j >= 0 && (c++, a[j] > k)) { a[j+1] = a[j]; s++; j--; }
        a[j+1] = k;
    }
}

void BenchShell(std::vector<float>& a, long long& c, long long& s) {
    int n = a.size();
    int g = 1; while (g < n/3) g = g*3 + 1;  // sequencia de Knuth
    while (g >= 1) {
        for (int i = g; i < n; i++) {
            float k = a[i]; int j = i;
            while (j >= g && (c++, a[j-g] > k)) { a[j] = a[j-g]; s++; j -= g; }
            a[j] = k;
        }
        g /= 3;
    }
}

void BenchMerge(std::vector<float>& a, long long& c, long long& s) {
    int n = a.size(); std::vector<float> t(n);
    for (int w = 1; w < n; w *= 2)
        for (int l = 0; l < n; l += 2*w) {
            int m = std::min(l+w, n), r = std::min(l+2*w, n);
            int i = l, j = m, k = l;
            while (i < m && j < r) { c++; if (a[i] <= a[j]) t[k++] = a[i++]; else { t[k++] = a[j++]; s++; } }
            while (i < m) t[k++] = a[i++];
            while (j < r) t[k++] = a[j++];
            for (int x = l; x < r; x++) a[x] = t[x];
        }
}

void BenchQuick(std::vector<float>& a, long long& c, long long& s) {
    std::vector<std::pair<int,int>> st;
    st.push_back({0, (int)a.size()-1});
    while (!st.empty()) {
        auto [lo, hi] = st.back(); st.pop_back();
        if (lo >= hi) continue;
        float p = a[hi]; int i = lo;
        for (int j = lo; j < hi; j++) { c++; if (a[j] < p) { std::swap(a[i], a[j]); s++; i++; } }
        std::swap(a[i], a[hi]); s++;
        st.push_back({lo, i-1});
        st.push_back({i+1, hi});
    }
}

void BenchHeap(std::vector<float>& a, long long& c, long long& s) {
    int n = a.size();
    auto sift = [&](int i, int end) {
        while (2*i+1 <= end) {
            int ch = 2*i+1, sw = i;
            c++; if (a[sw] < a[ch]) sw = ch;
            if (ch+1 <= end) { c++; if (a[sw] < a[ch+1]) sw = ch+1; }
            if (sw == i) return;
            std::swap(a[i], a[sw]); s++; i = sw;
        }
    };
    for (int i = n/2-1; i >= 0; i--) sift(i, n-1);       // build max-heap
    for (int e = n-1; e > 0; e--) { std::swap(a[0], a[e]); s++; sift(0, e-1); }  // extract max
}

void BenchCounting(std::vector<float>& a, long long& c, long long& s) {
    int n = a.size(); if (!n) return;
    int mx = 0; for (auto& v : a) { int k = (int)(v*10); if (k > mx) mx = k; }
    std::vector<int> cnt(mx+1, 0); std::vector<float> out(n);
    for (int i = 0; i < n; i++) { cnt[(int)(a[i]*10)]++; c++; }
    for (int i = 1; i <= mx; i++) cnt[i] += cnt[i-1];     // prefix sum
    for (int i = n-1; i >= 0; i--) { out[--cnt[(int)(a[i]*10)]] = a[i]; s++; }
    for (int i = 0; i < n; i++) a[i] = out[i];
}

void BenchRadixLSD(std::vector<float>& a, long long& c, long long& s) {
    int n = a.size(); if (!n) return;
    std::vector<int> keys(n); int mx = 0;
    for (int i = 0; i < n; i++) { keys[i] = (int)(a[i]*10); if (keys[i] > mx) mx = keys[i]; }
    std::vector<float> out(n); std::vector<int> ko(n);
    for (int exp = 1; mx/exp > 0; exp *= 10) {
        int cnt[10] = {};
        for (int i = 0; i < n; i++) { cnt[(keys[i]/exp)%10]++; c++; }
        for (int i = 1; i < 10; i++) cnt[i] += cnt[i-1];
        for (int i = n-1; i >= 0; i--) {
            int d = (keys[i]/exp) % 10; int p = --cnt[d];
            out[p] = a[i]; ko[p] = keys[i]; s++;
        }
        for (int i = 0; i < n; i++) { a[i] = out[i]; keys[i] = ko[i]; }
    }
}

void BenchRadixMSD(std::vector<float>& a, long long& c, long long& s) {
    int n = a.size(); if (!n) return;
    int mx = 0; for (auto& v : a) { int k = (int)(v*10); if (k > mx) mx = k; }
    int exp = 1; while (exp*10 <= mx) exp *= 10;
    std::function<void(int,int,int)> go = [&](int lo, int hi, int ex) {
        if (lo >= hi || ex <= 0) return;
        std::vector<std::vector<float>> bk(10);
        for (int i = lo; i <= hi; i++) {
            int d = ((int)(a[i]*10) / ex) % 10;
            bk[d].push_back(a[i]); c++;
        }
        int idx = lo;
        for (int d = 0; d < 10; d++) {
            int st = idx;
            for (auto& v : bk[d]) { a[idx++] = v; s++; }
            if ((int)bk[d].size() > 1) go(st, st + (int)bk[d].size()-1, ex/10);
        }
    };
    go(0, n-1, exp);
}

void BenchBucket(std::vector<float>& a, long long& c, long long& s) {
    int n = a.size(); if (!n) return;
    float mx = *std::max_element(a.begin(), a.end()) + 1;
    int bc = std::max(5, (int)sqrtf((float)n));  // num baldes = sqrt(n)
    std::vector<std::vector<float>> bk(bc);
    for (int i = 0; i < n; i++) {
        int bi = (int)(a[i] * bc / mx); if (bi >= bc) bi = bc-1;
        bk[bi].push_back(a[i]); c++;
    }
    int idx = 0;
    for (auto& b : bk) {
        // Insertion sort dentro de cada balde
        for (int i = 1; i < (int)b.size(); i++) {
            float k = b[i]; int j = i-1;
            while (j >= 0 && (c++, b[j] > k)) { b[j+1] = b[j]; s++; j--; }
            b[j+1] = k;
        }
        for (auto& v : b) a[idx++] = v;
    }
}

// Executa benchmark completo: roda todos os 11 algoritmos com 4 tamanhos
void RunBenchmark() {
    benchResults.clear();
    struct {
        const char* name; const char* cx;
        std::function<void(std::vector<float>&, long long&, long long&)> fn;
    } algos[] = {
        {"Bubble Sort",    "O(n^2)",       BenchBubble},
        {"Selection Sort", "O(n^2)",       BenchSelection},
        {"Insertion Sort", "O(n^2)",       BenchInsertion},
        {"Shell Sort",     "O(n log^2 n)", BenchShell},
        {"Merge Sort",     "O(n log n)",   BenchMerge},
        {"Quick Sort",     "O(n log n)",   BenchQuick},
        {"Heap Sort",      "O(n log n)",   BenchHeap},
        {"Counting Sort",  "O(n + k)",     BenchCounting},
        {"Radix LSD",      "O(d*(n+k))",   BenchRadixLSD},
        {"Radix MSD",      "O(d*(n+k))",   BenchRadixMSD},
        {"Bucket Sort",    "O(n + k)",     BenchBucket},
    };
    int sizes[] = {100, 1000, 5000, 10000};
    srand(42);  // seed fixa para benchmark reproduzivel
    for (int sz : sizes) {
        std::vector<float> base(sz);
        for (int i = 0; i < sz; i++) base[i] = 80.0f + (float)(rand() % 2200) / 10.0f;
        for (auto& al : algos) {
            // Pula O(n^2) em n=10000 — muito lento
            if (sz >= 10000 && (al.name == std::string("Bubble Sort") ||
                al.name == std::string("Selection Sort") ||
                al.name == std::string("Insertion Sort"))) {
                benchResults.push_back({al.name, al.cx, 0, 0, 0, false});
                continue;
            }
            std::vector<float> a = base;
            long long c = 0, s = 0;
            auto t0 = std::chrono::high_resolution_clock::now();
            al.fn(a, c, s);
            auto t1 = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
            bool ok = std::is_sorted(a.begin(), a.end());
            benchResults.push_back({al.name, al.cx, c, s, ms, ok});
        }
    }
    benchRan = true;
}

// =============================================================================
// GERACAO DA FROTA — cria NV avioes com posicoes e velocidades aleatorias
// =============================================================================
void Gerar() {
    frota.clear(); logs.clear(); booms.clear();
    srand((unsigned)time(0));
    const char* pf[] = {"AZU","GOL","TAM","LAT","VRG","PTB"};
    const float MIN_DIST = 30.0f;  // distancia minima entre avioes no radar (pixels)
    for (int i = 0; i < NV; i++) {
        Voo v;
        v.id = 100 + rand() % 900;
        snprintf(v.cs, 7, "%s%02d", pf[rand()%6], v.id % 100);
        // Gera posicao garantindo distancia minima dos avioes ja criados
        int tentativas = 0;
        bool valido;
        do {
            v.ang = (rand() % 3600) / 10.0f * DEG2RAD;
            v.raio = 80 + (float)(rand() % (int)(RR - 90));
            valido = true;
            float px = RCX + cosf(v.ang) * v.raio;
            float py = RCY + sinf(v.ang) * v.raio;
            for (auto& outro : frota) {
                float ox = RCX + cosf(outro.ang) * outro.raio;
                float oy = RCY + sinf(outro.ang) * outro.raio;
                float dx = px - ox, dy = py - oy;
                if (dx*dx + dy*dy < MIN_DIST * MIN_DIST) { valido = false; break; }
            }
            tentativas++;
        } while (!valido && tentativas < 200);
        v.vel = 3.0f + (rand() % 8);
        v.distInicial = v.raio;
        v.est = C_N; v.chegou = false; v.ok = false;
        frota.push_back(v);
    }
    modo = M_LIVRE; sDone = false; nComp = 0; nTroc = 0; tDec = 0;
}

// Reseta cores de todos os avioes para neutro
void RVis() { for (auto& v : frota) if (!v.chegou) v.est = C_N; }

// Verifica se o aviao na posicao idx ja foi ordenado pelo algoritmo atual
// Isso determina se ele pousa com seguranca ou crasha ao atingir a pista
bool EstaOrdenado(int idx) {
    if (sDone) return true;                    // algoritmo terminou = todos seguros
    if (modo == M_SEL)    return idx < sI;     // Selection ordena do INICIO
    if (modo == M_BUBBLE) return idx >= (NV - bI);  // Bubble ordena do FIM
    if (modo == M_INSERT) return idx < insI;   // Insertion ordena do INICIO
    return false;  // demais algoritmos so consideram seguro apos terminar (sDone)
}

// =============================================================================
// BUBBLE SORT — O(n^2) comparativo
// Estrategia: percorre o array comparando pares adjacentes e empurra o MAIOR
// para o final a cada passada. No contexto ATC, o aviao mais perto fica "preso"
// no inicio enquanto os maiores sao empurrados — causa CRASHES.
// =============================================================================
void InitBub() {
    RVis(); modo = M_BUBBLE; bI = 0; bJ = 0;
    sDone = false; nComp = 0; nTroc = 0; tU = GetTime();
}

void StepBub() {
    if (sDone) return;
    if (GetTime() - tU < dAnim) return;  // controle de velocidade da animacao
    tU = GetTime();
    RVis();
    // Marca elementos ja ordenados (final do array)
    for (int k = NV - bI; k < NV; k++) if (!frota[k].chegou) frota[k].est = C_OK;
    if (bI < NV-1) {
        if (bJ < NV - bI - 1) {
            if (!frota[bJ].chegou)   frota[bJ].est = C_CMP;
            if (!frota[bJ+1].chegou) frota[bJ+1].est = C_CMP;
            nComp++;
            if (frota[bJ].distInicial > frota[bJ+1].distInicial) {
                std::swap(frota[bJ], frota[bJ+1]); nTroc++;
            }
            bJ++;
        } else { bJ = 0; bI++; }
    } else {
        sDone = true;
        for (auto& v : frota) if (!v.chegou) v.est = C_OK;
    }
}

// =============================================================================
// SELECTION SORT — O(n^2) comparativo
// Estrategia: a cada iteracao, busca o MENOR elemento (menor distancia) e
// coloca na proxima posicao. No ATC, prioriza imediatamente o mais perto
// — resultado: POUSOS seguros.
// =============================================================================
void InitSel() {
    RVis(); modo = M_SEL; sI = 0; sMin = 0; sJ = 1;
    sDone = false; nComp = 0; nTroc = 0; tU = GetTime();
}

void StepSel() {
    if (sDone) return;
    if (GetTime() - tU < dAnim) return;
    tU = GetTime();
    RVis();
    for (int k = 0; k < sI; k++) if (!frota[k].chegou) frota[k].est = C_OK;
    if (sI < NV-1) {
        if (sJ < NV) {
            if (!frota[sMin].chegou) frota[sMin].est = C_PIV;  // candidato a minimo
            if (!frota[sJ].chegou)   frota[sJ].est = C_CMP;    // sendo comparado
            nComp++;
            if (frota[sJ].distInicial < frota[sMin].distInicial) sMin = sJ;
            sJ++;
        } else {
            if (sMin != sI) { std::swap(frota[sI], frota[sMin]); nTroc++; }
            if (!frota[sI].chegou) frota[sI].est = C_OK;
            sI++; sMin = sI; sJ = sI + 1;
        }
    } else {
        sDone = true;
        for (auto& v : frota) if (!v.chegou) v.est = C_OK;
    }
}

// =============================================================================
// INSERTION SORT — O(n^2) comparativo
// Estrategia: para cada elemento, insere-o na posicao correta da sub-lista
// ja ordenada (a esquerda). Usa shifting (deslocamento) ao inves de swaps.
// Eficiente para dados quase ordenados (melhor caso O(n)).
// =============================================================================
void InitInsert() {
    RVis(); modo = M_INSERT; insI = 1; insShifting = false;
    sDone = false; nComp = 0; nTroc = 0; tU = GetTime();
}

void StepInsert() {
    if (sDone) return;
    if (GetTime() - tU < dAnim) return;
    tU = GetTime();
    RVis();
    for (int k = 0; k < insI; k++) if (!frota[k].chegou) frota[k].est = C_OK;
    if (insI >= NV) { sDone = true; for (auto& v : frota) if (!v.chegou) v.est = C_OK; return; }
    if (!insShifting) {
        insKeyVoo = frota[insI]; insKey = insKeyVoo.distInicial;
        insJ = insI - 1; insShifting = true;
        if (!frota[insI].chegou) frota[insI].est = C_PIV;
    }
    if (insJ >= 0) {
        nComp++;
        if (!frota[insJ].chegou) frota[insJ].est = C_CMP;
        if (frota[insJ].distInicial > insKey) {
            frota[insJ+1] = frota[insJ]; nTroc++; insJ--;
        } else { frota[insJ+1] = insKeyVoo; insShifting = false; insI++; }
    } else { frota[0] = insKeyVoo; insShifting = false; insI++; }
}

// =============================================================================
// SHELL SORT — O(n log^2 n) comparativo
// Melhoria do Insertion Sort usando GAPS decrescentes (sequencia de Knuth).
// Permite que elementos distantes se aproximem da posicao final rapidamente.
// Gap sequence: 1, 4, 13, 40, 121... (gap = gap*3 + 1)
// =============================================================================
void InitShell() {
    RVis(); modo = M_SHELL; sDone = false; nComp = 0; nTroc = 0;
    shGap = 1; while (shGap < NV/3) shGap = shGap*3 + 1;
    shI = shGap; shShifting = false; tU = GetTime();
}

void StepShell() {
    if (sDone) return;
    if (GetTime() - tU < dAnim) return;
    tU = GetTime();
    RVis();
    if (shGap < 1) { sDone = true; for (auto& v : frota) if (!v.chegou) v.est = C_OK; return; }
    if (shI >= NV) {
        shGap /= 3;
        if (shGap < 1) { sDone = true; for (auto& v : frota) if (!v.chegou) v.est = C_OK; return; }
        shI = shGap; shShifting = false; return;
    }
    if (!shShifting) {
        shKeyVoo = frota[shI]; shKey = shKeyVoo.distInicial;
        shJ = shI; shShifting = true;
        if (!frota[shI].chegou) frota[shI].est = C_PIV;
    }
    if (shJ >= shGap) {
        nComp++;
        if (!frota[shJ-shGap].chegou) frota[shJ-shGap].est = C_CMP;
        if (frota[shJ-shGap].distInicial > shKey) {
            frota[shJ] = frota[shJ-shGap]; nTroc++; shJ -= shGap;
        } else { frota[shJ] = shKeyVoo; shShifting = false; shI++; }
    } else { frota[shJ] = shKeyVoo; shShifting = false; shI++; }
}

// =============================================================================
// MERGE SORT — O(n log n) comparativo (implementacao bottom-up iterativa)
// Estrategia: mescla blocos de tamanho crescente (1, 2, 4, 8...) sem recursao.
// Usa vetor temporario mTmp. No ATC: so considera seguro apos terminar
// completamente — nao oferece priorizacao parcial durante a execucao.
// =============================================================================
void InitMerge() {
    RVis(); modo = M_MERGE; sDone = false; nComp = 0; nTroc = 0;
    mWidth = 1; mLeft = 0; mInMerge = false; tU = GetTime();
}

void StepMerge() {
    if (sDone) return;
    if (GetTime() - tU < dAnim) return;
    tU = GetTime();
    RVis();
    if (!mInMerge) {
        // Avanca ate achar par de blocos valido
        while (mLeft < NV && mLeft + mWidth >= NV) mLeft += 2 * mWidth;
        if (mLeft >= NV) {
            mWidth *= 2; mLeft = 0;
            if (mWidth >= NV) { sDone = true; for (auto& v : frota) if (!v.chegou) v.est = C_OK; return; }
            while (mLeft < NV && mLeft + mWidth >= NV) mLeft += 2 * mWidth;
            if (mLeft >= NV) return;
        }
        mMid = mLeft + mWidth;
        mRight = std::min(mLeft + 2*mWidth, NV);
        mTmp.assign(frota.begin() + mLeft, frota.begin() + mRight);
        mI = 0; mJ = mWidth; mK = mLeft; mInMerge = true;
    }
    // Destaca janela de merge atual
    for (int k = mLeft; k < mRight; k++) if (!frota[k].chegou) frota[k].est = C_CMP;
    int leftEnd = (int)mTmp.size() < mWidth ? (int)mTmp.size() : mWidth;
    int rightEnd = (int)mTmp.size();
    if (mI < leftEnd && mJ < rightEnd) {
        nComp++;
        if (mTmp[mI].distInicial <= mTmp[mJ].distInicial) {
            frota[mK] = mTmp[mI]; frota[mK].est = C_PIV; mI++;
        } else {
            frota[mK] = mTmp[mJ]; frota[mK].est = C_PIV; mJ++; nTroc++;
        }
        mK++;
    } else if (mI < leftEnd) { frota[mK] = mTmp[mI]; frota[mK].est = C_OK; mI++; mK++; }
    else if (mJ < rightEnd) { frota[mK] = mTmp[mJ]; frota[mK].est = C_OK; mJ++; mK++; }
    else { mInMerge = false; mLeft += 2 * mWidth; }
}

// =============================================================================
// QUICK SORT — O(n log n) medio, O(n^2) pior caso — comparativo
// Estrategia: escolhe o ultimo elemento como PIVOT e particiona o array —
// elementos menores que o pivot vao para a esquerda, maiores para a direita.
// Usa pilha explicita (qStack) ao inves de recursao para permitir animacao.
// =============================================================================
void InitQuick() {
    RVis(); modo = M_QUICK; sDone = false; nComp = 0; nTroc = 0;
    qStack.clear(); qStack.push_back({0, NV-1});
    qInPartition = false; tU = GetTime();
}

void StepQuick() {
    if (sDone) return;
    if (GetTime() - tU < dAnim) return;
    tU = GetTime();
    RVis();
    if (!qInPartition) {
        if (qStack.empty()) { sDone = true; for (auto& v : frota) if (!v.chegou) v.est = C_OK; return; }
        QFrame f = qStack.back(); qStack.pop_back();
        qLo = f.lo; qHi = f.hi;
        if (qLo >= qHi) {
            if (qLo >= 0 && qLo < NV && !frota[qLo].chegou) frota[qLo].est = C_OK;
            return;
        }
        qPivIdx = qHi; qI = qLo; qJ = qLo; qInPartition = true;
    }
    if (!frota[qPivIdx].chegou) frota[qPivIdx].est = C_PIV;
    if (qJ < qHi) {
        nComp++;
        if (!frota[qJ].chegou) frota[qJ].est = C_CMP;
        if (frota[qJ].distInicial < frota[qPivIdx].distInicial) {
            std::swap(frota[qI], frota[qJ]); nTroc++; qI++;
        }
        qJ++;
    } else {
        std::swap(frota[qI], frota[qHi]); nTroc++;
        if (!frota[qI].chegou) frota[qI].est = C_OK;
        if (qI-1 > qLo) qStack.push_back({qLo, qI-1});
        else if (qLo >= 0 && qLo < NV && !frota[qLo].chegou) frota[qLo].est = C_OK;
        if (qI+1 < qHi) qStack.push_back({qI+1, qHi});
        else if (qHi >= 0 && qHi < NV && !frota[qHi].chegou) frota[qHi].est = C_OK;
        qInPartition = false;
    }
}

// =============================================================================
// HEAP SORT — O(n log n) comparativo
// Estrategia em 2 fases:
//   1) BUILD: constroi um max-heap a partir do array (sift-down de baixo pra cima)
//   2) EXTRACT: extrai o maximo (raiz) repetidamente, colocando no final
// Usa sift-down iterativo para manter a propriedade do heap.
// =============================================================================
void InitHeap() {
    RVis(); modo = M_HEAP; sDone = false; nComp = 0; nTroc = 0;
    hBuilding = true; hBuildIdx = NV/2 - 1; hEnd = NV - 1;
    hSifting = false; tU = GetTime();
}

void StepHeap() {
    if (sDone) return;
    if (GetTime() - tU < dAnim) return;
    tU = GetTime();
    RVis();
    // Marca elementos ja extraidos (final do array)
    for (int k = hEnd+1; k < NV; k++) if (!frota[k].chegou) frota[k].est = C_OK;
    if (hSifting) {
        // Sift-down: garante propriedade max-heap
        int root = hSiftRoot; int child = 2*root + 1;
        if (child > hEnd) { hSifting = false; return; }
        int sw = root; nComp++;
        if (frota[sw].distInicial < frota[child].distInicial) sw = child;
        if (child+1 <= hEnd) { nComp++; if (frota[sw].distInicial < frota[child+1].distInicial) sw = child+1; }
        if (sw == root) { hSifting = false; return; }
        if (!frota[root].chegou) frota[root].est = C_CMP;
        if (!frota[sw].chegou)   frota[sw].est = C_PIV;
        std::swap(frota[root], frota[sw]); nTroc++;
        hSiftRoot = sw; return;
    }
    if (hBuilding) {
        if (hBuildIdx >= 0) { hSiftRoot = hBuildIdx; hSifting = true; hBuildIdx--; }
        else hBuilding = false;
        return;
    }
    if (hEnd > 0) {
        std::swap(frota[0], frota[hEnd]); nTroc++;
        if (!frota[hEnd].chegou) frota[hEnd].est = C_OK;
        hEnd--; hSiftRoot = 0; hSifting = true;
    } else { sDone = true; for (auto& v : frota) if (!v.chegou) v.est = C_OK; }
}

// =============================================================================
// COUNTING SORT — O(n+k) nao-comparativo
// Ordena SEM comparar elementos diretamente. Funciona em 3 fases:
//   Fase 1: conta quantas vezes cada valor de distancia aparece
//   Fase 2: prefix sum — acumula contagens para determinar posicoes finais
//   Fase 3: posiciona cada aviao na posicao correta usando as contagens
// Usa DistKey() para converter float em inteiro (precisao de 1 casa decimal).
// Limitacao: precisa de espaco proporcional ao valor maximo (CNT_MAX=3200).
// =============================================================================
void InitCount() {
    RVis(); modo = M_COUNT; sDone = false; nComp = 0; nTroc = 0;
    memset(cntArr, 0, sizeof(cntArr));
    cntOutput.resize(NV);
    cPhase = 0; cIdx = 0; cPrefixIdx = 1; tU = GetTime();
}

void StepCount() {
    if (sDone) return;
    if (GetTime() - tU < dAnim) return;
    tU = GetTime();
    RVis();
    if (cPhase == 0) {
        // Fase 1: contagem de ocorrencias
        if (cIdx < NV) {
            int val = DistKey(frota[cIdx].distInicial);
            if (val > CNT_MAX) val = CNT_MAX;
            cntArr[val]++;
            if (!frota[cIdx].chegou) frota[cIdx].est = C_CMP;
            nComp++; cIdx++;
        } else {
            for (int i = 0; i < NV; i++) cntOutput[i] = frota[i];
            cPhase = 1; cPrefixIdx = 1;
        }
    } else if (cPhase == 1) {
        // Fase 2: prefix sum (soma acumulativa) — roda inteiro de uma vez (nada visual pra animar)
        for (int i = 1; i <= CNT_MAX; i++) cntArr[i] += cntArr[i-1];
        cPhase = 2; cIdx = NV-1;
    } else if (cPhase == 2) {
        // Fase 3: posicionamento final (de tras pra frente = estavel)
        if (cIdx >= 0) {
            int val = DistKey(cntOutput[cIdx].distInicial);
            if (val > CNT_MAX) val = CNT_MAX;
            int pos = --cntArr[val];
            if (pos >= 0 && pos < NV) {
                frota[pos] = cntOutput[cIdx];
                if (!frota[pos].chegou) frota[pos].est = C_PIV;
                nTroc++;
            }
            cIdx--;
        } else { sDone = true; for (auto& v : frota) if (!v.chegou) v.est = C_OK; }
    }
}

// =============================================================================
// RADIX SORT LSD — O(d*(n+k)) nao-comparativo
// Processa digito a digito do MENOS significativo para o MAIS significativo.
// Em cada passada: conta digitos, faz prefix sum, reposiciona.
// Variavel rlExp controla qual digito esta sendo processado (1, 10, 100...).
// Usa DistKey() para converter distancias em inteiros.
// =============================================================================
void InitRadixLSD() {
    RVis(); modo = M_RADLSD; sDone = false; nComp = 0; nTroc = 0;
    rlExp = 1; rlPhase = 0; rlIdx = 0;
    rlOutput.resize(NV);
    tU = GetTime();
}

void StepRadixLSD() {
    if (sDone) return;
    if (GetTime() - tU < dAnim) return;
    tU = GetTime();
    RVis();
    int maxKey = 0;
    for (auto& v : frota) { int k = DistKey(v.distInicial); if (k > maxKey) maxKey = k; }
    if (maxKey / rlExp <= 0) { sDone = true; for (auto& v : frota) if (!v.chegou) v.est = C_OK; return; }
    if (rlPhase == 0) {
        // Inicio de uma nova passada: zera contagens
        memset(rlCount, 0, sizeof(rlCount));
        rlPhase = 1; rlIdx = 0;
    } else if (rlPhase == 1) {
        // Conta digitos e reposiciona
        if (rlIdx < NV) {
            int d = (DistKey(frota[rlIdx].distInicial) / rlExp) % 10;
            rlCount[d]++;
            if (!frota[rlIdx].chegou) frota[rlIdx].est = C_CMP;
            nComp++; rlIdx++;
        } else {
            for (int i = 1; i < 10; i++) rlCount[i] += rlCount[i-1];
            for (int i = NV-1; i >= 0; i--) {
                int d = (DistKey(frota[i].distInicial) / rlExp) % 10;
                rlOutput[--rlCount[d]] = frota[i];
            }
            rlPhase = 2; rlIdx = 0;
        }
    } else if (rlPhase == 2) {
        // Copia resultado de volta para frota
        if (rlIdx < NV) {
            frota[rlIdx] = rlOutput[rlIdx];
            if (!frota[rlIdx].chegou) frota[rlIdx].est = C_PIV;
            nTroc++; rlIdx++;
        } else { rlExp *= 10; rlPhase = 0; }  // proximo digito
    }
}

// =============================================================================
// RADIX SORT MSD — O(d*(n+k)) nao-comparativo
// Inverso do LSD: processa do digito MAIS significativo para o MENOS.
// Usa pilha explicita (rmStack) com frames {lo, hi, exp} para simular recursao.
// Mais eficiente para dados com distribuicao desigual de digitos.
// =============================================================================
void InitRadixMSD() {
    RVis(); modo = M_RADMSD; sDone = false; nComp = 0; nTroc = 0;
    rmStack.clear();
    int maxKey = 0;
    for (auto& v : frota) { int k = DistKey(v.distInicial); if (k > maxKey) maxKey = k; }
    int exp = 1; while (exp*10 <= maxKey) exp *= 10;  // encontra digito mais significativo
    rmStack.push_back({0, NV-1, exp});
    rmPhase = 0; tU = GetTime();
}

void StepRadixMSD() {
    if (sDone) return;
    if (GetTime() - tU < dAnim) return;
    tU = GetTime();
    RVis();
    if (rmPhase == 0) {
        if (rmStack.empty()) { sDone = true; for (auto& v : frota) if (!v.chegou) v.est = C_OK; return; }
        rmCur = rmStack.back(); rmStack.pop_back();
        if (rmCur.lo >= rmCur.hi || rmCur.exp <= 0) {
            if (rmCur.lo >= 0 && rmCur.lo < NV && !frota[rmCur.lo].chegou) frota[rmCur.lo].est = C_OK;
            return;
        }
        rmBuck.clear(); rmBuck.resize(10);
        rmIdx = rmCur.lo; rmPhase = 1;
    } else if (rmPhase == 1) {
        // Distribui nos 10 buckets pelo digito atual
        if (rmIdx <= rmCur.hi) {
            int d = (DistKey(frota[rmIdx].distInicial) / rmCur.exp) % 10;
            rmBuck[d].push_back(frota[rmIdx]);
            if (!frota[rmIdx].chegou) frota[rmIdx].est = C_CMP;
            nComp++; rmIdx++;
        } else {
            // Recolhe dos buckets e empilha sub-problemas
            int idx = rmCur.lo;
            for (int d = 0; d < 10; d++) {
                int start = idx;
                for (auto& v : rmBuck[d]) {
                    frota[idx] = v;
                    if (!frota[idx].chegou) frota[idx].est = C_PIV;
                    nTroc++; idx++;
                }
                if ((int)rmBuck[d].size() > 1)
                    rmStack.push_back({start, start + (int)rmBuck[d].size()-1, rmCur.exp/10});
            }
            rmPhase = 0;
        }
    }
}

// =============================================================================
// BUCKET SORT — O(n+k) por distribuicao
// Estrategia em 3 fases:
//   Fase 1: distribui avioes em BALDES baseado na faixa de distancia
//   Fase 2: ordena cada balde internamente com Insertion Sort
//   Fase 3: concatena todos os baldes de volta no array principal
// Numero de baldes = 5 (fixo para visualizacao clara).
// =============================================================================
void InitBucket() {
    RVis(); modo = M_BUCKET; sDone = false; nComp = 0; nTroc = 0;
    bktPhase = 0; bktIdx = 0; bktCount = 5;
    bktBuckets.clear(); bktBuckets.resize(bktCount);
    tU = GetTime();
}

void StepBucket() {
    if (sDone) return;
    if (GetTime() - tU < dAnim) return;
    tU = GetTime();
    RVis();
    if (bktPhase == 0) {
        // Fase 1: distribuicao nos baldes
        if (bktIdx < NV) {
            float maxDist = 0;
            for (auto& v : frota) if (v.distInicial > maxDist) maxDist = v.distInicial;
            int bi = (int)(frota[bktIdx].distInicial * bktCount / (maxDist + 1));
            if (bi >= bktCount) bi = bktCount - 1;
            bktBuckets[bi].push_back(frota[bktIdx]);
            if (!frota[bktIdx].chegou) frota[bktIdx].est = C_CMP;
            nComp++; bktIdx++;
        } else { bktPhase = 1; bktB = 0; bktI = 1; }
    } else if (bktPhase == 1) {
        // Fase 2: insertion sort dentro de cada balde
        if (bktB < bktCount) {
            auto& bk = bktBuckets[bktB];
            if (bktI < (int)bk.size()) {
                Voo key = bk[bktI]; float kd = key.distInicial;
                int j = bktI - 1;
                while (j >= 0 && bk[j].distInicial > kd) { nComp++; bk[j+1] = bk[j]; nTroc++; j--; }
                if (j >= 0) nComp++;
                bk[j+1] = key; bktI++;
            } else { bktB++; bktI = 1; }
        } else { bktPhase = 2; bktIdx = 0; bktB = 0; bktI = 0; }
    } else if (bktPhase == 2) {
        // Fase 3: concatena baldes de volta no array
        if (bktB < bktCount) {
            if (bktI < (int)bktBuckets[bktB].size()) {
                frota[bktIdx] = bktBuckets[bktB][bktI];
                if (!frota[bktIdx].chegou) frota[bktIdx].est = C_PIV;
                nTroc++; bktIdx++; bktI++;
            } else { bktB++; bktI = 0; }
        } else { sDone = true; for (auto& v : frota) if (!v.chegou) v.est = C_OK; }
    }
}

// =============================================================================
// RADAR — renderizacao visual do radar ATC
// Circulos concentricos, linhas de grade, varredura animada (sweep)
// =============================================================================
void DrRadar() {
    // Fundo gradiente do radar
    DrawCircleGradient((int)RCX, (int)RCY, RR, {0,20,30,255}, {5,5,10,0});
    // Circulos de distancia (4 aneis)
    for (int i = 1; i <= 4; i++)
        DrawCircleLines((int)RCX, (int)RCY, RR*i/4.0f, {0,150,200,35});
    // Linhas radiais (12 direcoes, como relogio)
    for (int i = 0; i < 12; i++) {
        float a = i * 30 * DEG2RAD;
        DrawLineEx({RCX, RCY}, {RCX+cosf(a)*RR, RCY+sinf(a)*RR}, 1, {0,150,200,20});
    }
    // Varredura animada (sweep verde rotativo)
    float sw = GetTime() * 1.5f;
    for (int i = 0; i < 50; i++) {
        float a = sw - i*.012f, al = 1 - i/50.0f;
        DrawLineEx({RCX,RCY}, {RCX+cosf(a)*RR, RCY+sinf(a)*RR}, 2,
            Fade({0,255,150,255}, al*.3f));
    }
    // Pista de pouso (centro)
    DrawCircleLines((int)RCX, (int)RCY, RP, {255,255,255,60});
    DrawCircle((int)RCX, (int)RCY, 4, {0,255,150,200});
    DrawText("PISTA", (int)RCX-14, (int)RCY+8, 8, {0,255,150,100});
}

// =============================================================================
// ATUALIZACAO E RENDERIZACAO DOS AVIOES
// =============================================================================

// Atualiza posicao dos avioes e verifica pousos/crashes/colisoes
void UpdAv(float dt) {
    for (int i = 0; i < (int)frota.size(); i++) {
        Voo& v = frota[i]; if (v.chegou) continue;
        v.raio -= v.vel * dt;     // aproxima da pista
        v.ang += .003f * dt * 60; // orbita levemente
        if (v.raio <= RP) {
            // Aviao chegou na pista
            v.raio = RP; v.chegou = true;
            float px = RCX + cosf(v.ang)*v.raio, py = RCY + sinf(v.ang)*v.raio;
            bool safe = EstaOrdenado(i);  // esta na posicao correta?
            v.ok = safe; v.est = safe ? C_LAND : C_CRASH;
            Log l; l.id = v.id; strncpy(l.cs, v.cs, 7); l.ok = safe;
            logs.push_back(l);
            if (!safe) MkBoom(px, py);  // explosao visual no crash
        }
    }
    // Deteccao de colisao entre avioes — so quando nenhum algoritmo esta rodando
    // Durante a ordenacao, colisoes interferem na demonstracao
    if (modo != M_LIVRE) return;
    for (int i = 0; i < (int)frota.size(); i++) {
        if (frota[i].chegou) continue;
        float px1 = RCX + cosf(frota[i].ang)*frota[i].raio;
        float py1 = RCY + sinf(frota[i].ang)*frota[i].raio;
        for (int j = i+1; j < (int)frota.size(); j++) {
            if (frota[j].chegou) continue;
            float px2 = RCX + cosf(frota[j].ang)*frota[j].raio;
            float py2 = RCY + sinf(frota[j].ang)*frota[j].raio;
            float dx = px1-px2, dy = py1-py2;
            if (dx*dx + dy*dy < 10*10) {
                frota[i].chegou = true; frota[i].ok = false; frota[i].est = C_CRASH;
                frota[j].chegou = true; frota[j].ok = false; frota[j].est = C_CRASH;
                MkBoom((px1+px2)/2, (py1+py2)/2);
                Log l1; l1.id = frota[i].id; strncpy(l1.cs, frota[i].cs, 7); l1.ok = false; logs.push_back(l1);
                Log l2; l2.id = frota[j].id; strncpy(l2.cs, frota[j].cs, 7); l2.ok = false; logs.push_back(l2);
            }
        }
    }
}

// Renderiza todos os avioes no radar com cores baseadas no estado
void DrAv() {
    for (auto& v : frota) {
        float px = RCX + cosf(v.ang)*v.raio, py = RCY + sinf(v.ang)*v.raio;
        if (v.chegou && v.est == C_CRASH) continue;  // crashados desaparecem
        Color c = CorDe(v.est);
        if (v.chegou && v.est == C_LAND) {
            DrawCircle((int)px, (int)py, 3, Fade(c, .4f));  // pousado = ponto sutil
            continue;
        }
        // Linha do aviao ate a pista
        DrawLineEx({px,py}, {RCX,RCY}, 1.0f, Fade(c, 0.08f));
        // Distancia no ponto medio
        float mx = (px+RCX)/2, my = (py+RCY)/2;
        DrawText(TextFormat("%.0f", v.raio), (int)mx-8, (int)my-4, 8, Fade(c, 0.25f));
        // Blip do aviao (ponto + glow)
        DrawCircleGradient((int)px, (int)py, 14, Fade(c, .35f), Fade(c, 0));
        DrawCircle((int)px, (int)py, 3, WHITE);
        DrawCircle((int)px, (int)py, 2, c);
        // Pulso animado para avioes sendo processados
        if (v.est == C_CMP || v.est == C_PIV || v.est == C_OK) {
            float p = 7 + sinf(GetTime()*8) * 2;
            DrawCircleLines((int)px, (int)py, p, Fade(c, .5f));
        }
        DrawText(TextFormat("%d", v.id), (int)px+8, (int)py-10, 9, c);
    }
}

// =============================================================================
// DASHBOARD — painel lateral com informacoes do algoritmo e estado
// =============================================================================
void DrDash() {
    int dx = SW - 380, w = 380;
    // Fundo do painel
    DrawRectangle(dx, 0, w, SH, {10,14,20,245});
    DrawLineEx({(float)dx, 0}, {(float)dx, (float)SH}, 2, {0,150,255,120});
    // Titulo
    DrawRectangle(dx, 0, w, 60, {0,80,180,25});
    DrawText("PRIORITIZER ATC", dx+20, 12, 20, {0,220,255,255});
    DrawText("TRIAGEM POR DISTANCIA (MAIS PERTO = POUSA PRIMEIRO)", dx+20, 36, 8, {0,220,255,130});
    DrawLine(dx, 60, dx+w, 60, {0,150,255,80});

    // Status do algoritmo atual
    int sy = 72;
    DrawText(TextFormat("MODO: %s", nModo[modo]), dx+20, sy, 10, YELLOW);
    DrawText(TextFormat("DELAY: %.3fs | COMP: %d | TROCAS: %d", dAnim, nComp, nTroc), dx+20, sy+14, 9, LIGHTGRAY);
    DrawText(TextFormat("TEMPO: %.1fs", tDec), dx+20, sy+28, 9, {180,180,180,255});

    // Explicacao especifica de cada algoritmo
    int ey = sy + 46;
    if (modo == M_BUBBLE) {
        DrawText("BUBBLE: Empurra o MAIOR pro final.", dx+20, ey, 9, {255,200,0,200});
        DrawText("O(n^2) — O mais perto fica preso -> CRASH!", dx+20, ey+12, 9, {255,100,100,180});
    } else if (modo == M_SEL) {
        DrawText("SELECTION: Acha o MENOR primeiro.", dx+20, ey, 9, {255,200,0,200});
        DrawText("O(n^2) — O mais perto eh priorizado -> POUSO!", dx+20, ey+12, 9, {40,255,120,180});
    } else if (modo == M_INSERT) {
        DrawText("INSERTION: Insere na posicao correta.", dx+20, ey, 9, {255,200,0,200});
        DrawText("O(n^2) — Bom para listas quase ordenadas.", dx+20, ey+12, 9, {100,255,200,180});
    } else if (modo == M_SHELL) {
        DrawText("SHELL: Insertion com gaps (Knuth).", dx+20, ey, 9, {255,200,0,200});
        DrawText(TextFormat("O(n log^2 n) — Gap atual: %d", shGap), dx+20, ey+12, 9, {200,200,100,180});
    } else if (modo == M_MERGE) {
        DrawText("MERGE: Divide e mescla sublistas ordenadas.", dx+20, ey, 9, {255,200,0,200});
        DrawText("O(n log n) — ordena em bloco, sem pouso parcial.", dx+20, ey+12, 9, {100,180,255,180});
        DrawText(TextFormat("Bloco: %d  |  Inicio: %d", mWidth, mLeft), dx+20, ey+24, 9, {100,180,255,140});
    } else if (modo == M_QUICK) {
        DrawText("QUICK: Particiona em torno do pivot.", dx+20, ey, 9, {255,200,0,200});
        DrawText("O(n log n) medio — Muito rapido na pratica.", dx+20, ey+12, 9, {255,100,255,180});
    } else if (modo == M_HEAP) {
        DrawText("HEAP: Constroi max-heap e extrai maximo.", dx+20, ey, 9, {255,200,0,200});
        DrawText(TextFormat("O(n log n) — %s", hBuilding ? "Construindo heap..." : "Extraindo..."),
            dx+20, ey+12, 9, {255,200,100,180});
    } else if (modo == M_COUNT) {
        const char* fases[3] = {
            "Fase 1/3: Contando distancias...",
            "Fase 2/3: Acumulando contagens...",
            "Fase 3/3: Posicionando avioes..."
        };
        DrawText("COUNTING: Ordena por contagem direta.", dx+20, ey, 9, {255,200,0,200});
        DrawText(fases[cPhase < 3 ? cPhase : 2], dx+20, ey+12, 9, {180,100,255,200});
        DrawText("O(n+k) — zero comparacoes!", dx+20, ey+24, 9, {180,100,255,140});
    } else if (modo == M_RADLSD) {
        DrawText("RADIX LSD: Digito a digito (menor->maior).", dx+20, ey, 9, {255,200,0,200});
        DrawText(TextFormat("O(d*(n+k)) — Digito: x%d", rlExp), dx+20, ey+12, 9, {255,150,100,180});
    } else if (modo == M_RADMSD) {
        DrawText("RADIX MSD: Digito a digito (maior->menor).", dx+20, ey, 9, {255,200,0,200});
        DrawText("O(d*(n+k)) — Recursivo por bucket.", dx+20, ey+12, 9, {100,200,255,180});
    } else if (modo == M_BUCKET) {
        const char* bf[3] = {
            "Fase 1/3: Distribuindo nos baldes...",
            "Fase 2/3: Ordenando baldes...",
            "Fase 3/3: Concatenando baldes..."
        };
        DrawText("BUCKET: Distribui em baldes e ordena.", dx+20, ey, 9, {255,200,0,200});
        DrawText(bf[bktPhase < 3 ? bktPhase : 2], dx+20, ey+12, 9, {200,200,100,200});
    }

    // Legenda de cores
    int ly = ey + 38;
    DrawCircle(dx+25,  ly, 4, CorDe(C_N));    DrawText("Neutro",     dx+35,  ly-5, 9, LIGHTGRAY);
    DrawCircle(dx+105, ly, 4, CorDe(C_CMP));   DrawText("Comparando", dx+115, ly-5, 9, LIGHTGRAY);
    DrawCircle(dx+210, ly, 4, CorDe(C_PIV));   DrawText("Candidato",  dx+220, ly-5, 9, LIGHTGRAY);
    DrawCircle(dx+25,  ly+16, 4, CorDe(C_OK)); DrawText("Ordenado",   dx+35,  ly+11, 9, LIGHTGRAY);
    DrawCircle(dx+105, ly+16, 4, {40,255,120,150}); DrawText("Pousou", dx+115, ly+11, 9, LIGHTGRAY);
    DrawCircle(dx+210, ly+16, 4, CorDe(C_CRASH));   DrawText("CRASH",  dx+220, ly+11, 9, LIGHTGRAY);
    DrawLine(dx+15, ly+32, dx+w-15, ly+32, {255,255,255,15});

    // Fila de prioridade (tabela com todos os avioes)
    int ty = ly + 40;
    DrawText("FILA DE PRIORIDADE (por distancia)", dx+20, ty, 10, GRAY);
    int oy = ty + 18;
    for (int i = 0; i < NV; i++) {
        int col = i % 2, row = i / 2;
        int xp = dx + 15 + col*178, yp = oy + row*18;
        if (yp > SH - 170) break;
        Color c = CorDe(frota[i].est);
        if (row % 2 == 0) DrawRectangle(xp, yp-2, 173, 16, {255,255,255,4});
        const char* st = frota[i].chegou ? (frota[i].ok ? "OK" : "XX") : "";
        DrawText(TextFormat("[%02d] %d %s %.0f %s", i, frota[i].id, frota[i].cs, frota[i].distInicial, st),
            xp+2, yp, 9, c);
    }

    // Registro de pousos e crashes
    int lY = SH - 160;
    DrawLine(dx+15, lY-5, dx+w-15, lY-5, {255,255,255,15});
    DrawText("REGISTRO", dx+20, lY, 10, GRAY);
    int pou = 0, cra = 0;
    for (auto& l : logs) { if (l.ok) pou++; else cra++; }
    DrawText(TextFormat("POUSOS: %d", pou), dx+20, lY+16, 12, {40,255,120,255});
    DrawText(TextFormat("CRASHES: %d", cra), dx+160, lY+16, 12, {255,50,50,255});
    int ls = (int)logs.size() - 5; if (ls < 0) ls = 0;
    for (int i = ls; i < (int)logs.size(); i++) {
        int r = i - ls;
        Color lc = logs[i].ok ? Color{40,255,120,200} : Color{255,50,50,200};
        DrawText(TextFormat("%s %d %s", logs[i].ok ? "[POUSO]" : "[CRASH]", logs[i].id, logs[i].cs),
            dx+20, lY+34+r*14, 9, lc);
    }

    // Atalhos de teclado
    int cy = SH - 55;
    DrawLine(dx+15, cy-8, dx+w-15, cy-8, {255,255,255,15});
    DrawText("[1]Bub [2]Sel [3]Ins [4]Shell [5]Merge [6]Quick", dx+20, cy, 9, {100,100,100,200});
    DrawText("[7]Heap [8]Count [9]RadLSD [0]RadMSD [-]Bucket", dx+20, cy+12, 9, {100,100,100,200});
    DrawText("[R]Reset [UP/DOWN]Vel [SPACE]Pausa [TAB]Bench", dx+20, cy+24, 9, {100,100,100,200});
}

// =============================================================================
// TELA DE BENCHMARK — tabela comparativa de performance
// Mostra resultados para N = 100, 1000, 5000, 10000
// Colunas: algoritmo, complexidade, comparacoes, trocas, tempo(ms), ranking
// =============================================================================
void DrBench() {
    DrawRectangle(0, 0, SW, SH, {5,8,15,250});
    DrawText("BENCHMARK — TABELA DE PERFORMANCE", 40, 30, 22, {0,220,255,255});
    DrawText("Comparacao de todos os 11 algoritmos com arrays de floats (distancias simuladas)", 40, 58, 10, {0,180,255,150});
    DrawText("[TAB] Voltar ao radar    [B] Rodar benchmark novamente", 40, SH-30, 10, {100,100,100,200});

    if (!benchRan) {
        DrawText("Pressione [B] para executar o benchmark...", 40, SH/2-10, 16, YELLOW);
        return;
    }

    int sizes[] = {100, 1000, 5000, 10000};
    int nAlgos = 11;
    const char* algoNames[] = {
        "Bubble", "Selection", "Insertion", "Shell", "Merge",
        "Quick", "Heap", "Counting", "RadixLSD", "RadixMSD", "Bucket"
    };
    Color algoColors[] = {
        {255,80,80,255}, {255,160,60,255}, {80,255,120,255}, {0,255,255,255},
        {100,140,255,255}, {255,80,255,255}, {255,255,80,255}, {80,255,200,255},
        {255,140,140,255}, {140,220,255,255}, {255,255,150,255}
    };

    int tabY = 90;
    for (int si = 0; si < 4; si++) {
        int sz = sizes[si];
        int baseIdx = si * nAlgos;
        int tx = 40, ty = tabY + si*155;

        DrawText(TextFormat("N = %d", sz), tx, ty, 14, {0,220,255,255});
        DrawLine(tx, ty+18, SW-40, ty+18, {0,150,255,40});

        // Cabecalho da tabela
        int hdrY = ty + 22;
        DrawText("Algoritmo",    tx,     hdrY, 10, {0,180,255,200});
        DrawText("Complexidade", tx+130, hdrY, 10, {0,180,255,200});
        DrawText("Comparacoes",  tx+270, hdrY, 10, {0,180,255,200});
        DrawText("Trocas",       tx+400, hdrY, 10, {0,180,255,200});
        DrawText("Tempo(ms)",    tx+510, hdrY, 10, {0,180,255,200});
        DrawText("Status",       tx+620, hdrY, 10, {0,180,255,200});
        DrawLine(tx, hdrY+14, SW-40, hdrY+14, {0,150,255,20});

        // Calcula ranking por tempo
        std::vector<std::pair<double,int>> ranking;
        for (int i = 0; i < nAlgos; i++) {
            auto& r = benchResults[baseIdx + i];
            if (r.valid) ranking.push_back({r.timeMs, i});
        }
        std::sort(ranking.begin(), ranking.end());

        // Linha de cada algoritmo
        for (int i = 0; i < nAlgos; i++) {
            auto& r = benchResults[baseIdx + i];
            int ry = hdrY + 16 + i*11;
            if (i % 2 == 0) DrawRectangle(tx, ry-1, SW-80, 12, {255,255,255,5});

            int rank = -1;
            for (int k = 0; k < (int)ranking.size(); k++)
                if (ranking[k].second == i) { rank = k; break; }

            Color tc = algoColors[i];
            if (!r.valid) tc = Fade(tc, 0.3f);

            DrawText(algoNames[i], tx, ry, 9, tc);
            DrawText(r.complex, tx+130, ry, 9, Fade(tc, 0.7f));
            if (r.valid) {
                DrawText(TextFormat("%lld", r.comps), tx+270, ry, 9, {255,255,0,200});
                DrawText(TextFormat("%lld", r.swaps), tx+400, ry, 9, {255,180,0,200});
                DrawText(TextFormat("%.3f", r.timeMs), tx+510, ry, 9, {255,100,255,200});
                if (rank == 0)      DrawText("#1", tx+620, ry, 9, {0,255,0,255});
                else if (rank == 1) DrawText("#2", tx+620, ry, 9, {0,220,255,255});
                else if (rank == 2) DrawText("#3", tx+620, ry, 9, {255,200,0,255});
                else DrawText(TextFormat("#%d", rank+1), tx+620, ry, 9, {150,150,150,200});
            } else {
                DrawText("(pulado — lento)", tx+270, ry, 9, {100,100,100,150});
            }
        }
    }
}

// =============================================================================
// MAIN — ponto de entrada do programa
// Configura janela, cria canvas fixo 1280x720, e roda o game loop
// O canvas e escalado proporcionalmente para qualquer tamanho de janela (16:9)
// =============================================================================
int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(SW, SH, "Prioritizer ATC - 11 Algoritmos de Ordenacao");
    SetWindowMinSize(640, 360);
    SetTargetFPS(60);
    Gerar();

    // Canvas fixo — renderiza tudo aqui, depois escala para a janela
    RenderTexture2D canvas = LoadRenderTexture(SW, SH);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (!pausado && !showBench) tDec += dt;

        // Tecla TAB alterna entre radar e benchmark
        if (IsKeyPressed(KEY_TAB)) showBench = !showBench;
        if (showBench && IsKeyPressed(KEY_B)) RunBenchmark();

        if (!showBench) {
            // Input de selecao de algoritmo
            if (IsKeyPressed(KEY_R))     Gerar();
            if (IsKeyPressed(KEY_ONE)   && modo == M_LIVRE) InitBub();
            if (IsKeyPressed(KEY_TWO)   && modo == M_LIVRE) InitSel();
            if (IsKeyPressed(KEY_THREE) && modo == M_LIVRE) InitInsert();
            if (IsKeyPressed(KEY_FOUR)  && modo == M_LIVRE) InitShell();
            if (IsKeyPressed(KEY_FIVE)  && modo == M_LIVRE) InitMerge();
            if (IsKeyPressed(KEY_SIX)   && modo == M_LIVRE) InitQuick();
            if (IsKeyPressed(KEY_SEVEN) && modo == M_LIVRE) InitHeap();
            if (IsKeyPressed(KEY_EIGHT) && modo == M_LIVRE) InitCount();
            if (IsKeyPressed(KEY_NINE)  && modo == M_LIVRE) InitRadixLSD();
            if (IsKeyPressed(KEY_ZERO)  && modo == M_LIVRE) InitRadixMSD();
            if (IsKeyPressed(KEY_MINUS) && modo == M_LIVRE) InitBucket();
            if (IsKeyPressed(KEY_UP))   { dAnim -= .02f; if (dAnim < .01f) dAnim = .01f; }
            if (IsKeyPressed(KEY_DOWN)) { dAnim += .02f; if (dAnim > 1) dAnim = 1; }
            if (IsKeyPressed(KEY_SPACE)) pausado = !pausado;

            // Avanca simulacao
            if (!pausado) {
                if (modo == M_BUBBLE) StepBub();
                if (modo == M_SEL)    StepSel();
                if (modo == M_INSERT) StepInsert();
                if (modo == M_SHELL)  StepShell();
                if (modo == M_MERGE)  StepMerge();
                if (modo == M_QUICK)  StepQuick();
                if (modo == M_HEAP)   StepHeap();
                if (modo == M_COUNT)  StepCount();
                if (modo == M_RADLSD) StepRadixLSD();
                if (modo == M_RADMSD) StepRadixMSD();
                if (modo == M_BUCKET) StepBucket();
                UpdAv(dt); UpdBoom(dt);
            }
        }

        // Renderiza no canvas fixo (1280x720)
        BeginTextureMode(canvas);
            ClearBackground({5,5,8,255});
            if (showBench) DrBench();
            else { DrRadar(); DrAv(); DrBoom(); DrDash(); }
        EndTextureMode();

        // Escala canvas para a janela mantendo proporcao 16:9
        int ww = GetScreenWidth(), wh = GetScreenHeight();
        float scale = fminf((float)ww/SW, (float)wh/SH);
        float dw = SW*scale, dh = SH*scale;
        float ox = (ww - dw) / 2.0f, oy = (wh - dh) / 2.0f;
        Rectangle src = {0, 0, (float)SW, -(float)SH};  // Y invertido (textura OpenGL)
        Rectangle dst = {ox, oy, dw, dh};

        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(canvas.texture, src, dst, {0,0}, 0, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(canvas);
    CloseWindow();
    return 0;
}
