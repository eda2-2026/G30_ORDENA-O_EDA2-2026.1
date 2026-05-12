# Prioritizer ATC — Simulador de Triagem de Pouso

**Autores:** Patrick Anderson Carvalho dos Santos, Vinicius Castelo Ferreira  
**Projeto:** EDA 2 — Estruturas de Dados e Algoritmos 2026.1

Simulador de Radar de Controle de Trafego Aereo (ATC) focado na demonstracao visual de **11 algoritmos de ordenacao**. Desenvolvido em C++17 com raylib 5.0 para renderizacao grafica em tempo real.

## Alunos

| Nome | Matricula |
|------|-----------|
| Patrick Anderson Carvalho dos Santos | 211030620 |
| Vinicius Castelo Ferreira | 200062883 |

## Conceito

Avioes se aproximam da pista de pouso (centro do radar). O sistema precisa ordenar a fila de pouso por **distancia** — quem esta mais perto pousa primeiro. Se o algoritmo nao conseguir ordenar a tempo, o aviao chega sem priorizacao e **crasha**.

O simulador permite visualizar, passo a passo, como cada algoritmo de ordenacao se comporta nesse cenario critico de tempo real, evidenciando as diferencas praticas entre complexidades O(n^2), O(n log n) e O(n+k).

## Algoritmos Implementados

| # | Algoritmo | Complexidade | Tipo | Tecla |
|---|-----------|-------------|------|-------|
| 1 | **Bubble Sort** | O(n^2) | Comparacao | `[1]` |
| 2 | **Selection Sort** | O(n^2) | Comparacao | `[2]` |
| 3 | **Insertion Sort** | O(n^2) | Comparacao | `[3]` |
| 4 | **Shell Sort** | O(n log^2 n) | Comparacao | `[4]` |
| 5 | **Merge Sort** | O(n log n) | Comparacao | `[5]` |
| 6 | **Quick Sort** | O(n log n) | Comparacao | `[6]` |
| 7 | **Heap Sort** | O(n log n) | Comparacao | `[7]` |
| 8 | **Counting Sort** | O(n+k) | Nao-comparativo | `[8]` |
| 9 | **Radix Sort LSD** | O(d*(n+k)) | Nao-comparativo | `[9]` |
| 10 | **Radix Sort MSD** | O(d*(n+k)) | Nao-comparativo | `[0]` |
| 11 | **Bucket Sort** | O(n+k) | Distribuicao | `[-]` |

## Controles

| Tecla | Acao |
|---|---|
| `[1]` a `[-]` | Iniciar algoritmo correspondente |
| `[R]` | Resetar e embaralhar a frota |
| `[SPACE]` | Pausar / Despausar simulacao |
| `[UP]` | Aumentar velocidade da animacao |
| `[DOWN]` | Diminuir velocidade da animacao |
| `[TAB]` | Alternar para tela de Benchmark |
| `[B]` | Executar benchmark (na tela de benchmark) |

A janela e redimensionavel — o conteudo escala proporcionalmente (16:9).

## Como Compilar e Executar (Step-by-Step)

### Pre-requisitos

**Windows**

1. Instale o [Git para Windows](https://git-scm.com/download/win)
2. Instale o [MSYS2](https://www.msys2.org/) e dentro do terminal MSYS2 rode:
   ```bash
   pacman -S mingw-w64-x86_64-gcc make
   ```
3. Adicione `C:\msys64\mingw64\bin` e `C:\msys64\usr\bin` ao PATH do sistema

**Linux (Debian/Ubuntu)**

```bash
sudo apt update
sudo apt install build-essential git \
  libasound2-dev mesa-common-dev \
  libx11-dev libxrandr-dev libxi-dev \
  xorg-dev libgl1-mesa-dev libglu1-mesa-dev \
  libxcursor-dev libxinerama-dev
```

**macOS**

```bash
xcode-select --install
```

### Passo a Passo para Executar

```bash
# 1. Clone o repositorio com os submodules
git clone --recurse-submodules <url-do-repositorio>
cd G30_ORDENA-O_EDA2-2026.1

# 2. Primeira vez: baixa e compila a biblioteca raylib
make setup

# 3. Compila e executa o simulador
make
```

Se os submodules ja estiverem presentes mas a lib nao compilada:

```bash
make lib    # compila apenas a raylib
make        # compila e executa o projeto
```

Se quiser apenas compilar sem executar:

```bash
make bin/app
```

### Estrutura de Pastas

```
.
├── src/
│   └── main.cpp          # codigo-fonte principal (~1008 linhas)
├── include/              # headers (gerados pelo make setup)
│   └── patrickacs.h      # raylib.h renomeado
├── lib/
│   └── <plataforma>/     # biblioteca estatica raylib
├── vendor/
│   ├── raylib/           # submodule: raylib 5.0
│   └── raylib-cpp/       # submodule: raylib-cpp bindings
├── bin/
│   └── app               # executavel gerado
├── docs/
│   ├── InstallingDependencies.md
│   └── MakefileExplanation.md
├── Makefile
└── README.md
```

## O Que Foi Desenvolvido (Evolucao do Projeto)

### Commit 1 — Base (Bubble Sort + Selection Sort)
- Estrutura base do radar ATC com renderizacao raylib
- Sistema de particulas para explosoes (crash)
- Bubble Sort e Selection Sort com animacao passo a passo
- Dashboard lateral com estatisticas (comparacoes, trocas, tempo)
- Legenda visual por cores (neutro, comparando, candidato, ordenado, crash)
- Fila de prioridade visual e registro de pousos/crashes
- Apenas 2 algoritmos, janela com tamanho fixo

### Commit 2 — Merge Sort + Counting Sort
- Adicionados Merge Sort (bottom-up iterativo) e Counting Sort (3 fases)
- Janela agora e redimensionavel com escala proporcional 16:9
- Canvas fixo 1280x720 renderizado via RenderTexture2D
- Dashboard expandido com explicacoes especificas de cada algoritmo
- Correcoes de bugs que impediam a execucao

### Estado Atual — 11 Algoritmos + Benchmark
- **7 novos algoritmos:** Insertion Sort, Shell Sort, Quick Sort, Heap Sort, Radix LSD, Radix MSD, Bucket Sort
- **Sistema de benchmark completo:** tela dedicada (TAB) que compara todos os 11 algoritmos com arrays de 100, 1000, 5000 e 10000 elementos, medindo comparacoes, trocas e tempo em ms
- **Ranking automatico** por performance em cada tamanho de array
- Funcao `DistKey()` para converter distancia float em chave inteira (precisao x10)
- Sistema de colisao entre avioes no radar
- Cada algoritmo tem sua propria maquina de estados para animacao passo a passo
- Dashboard com explicacoes detalhadas para cada um dos 11 algoritmos

## Tecnologias

- **C++17** — linguagem principal
- **raylib 5.0** — renderizacao grafica 2D
- **raylib-cpp** — bindings orientados a objeto para raylib
- **Makefile** — build system multiplataforma (Windows, Linux, macOS)

