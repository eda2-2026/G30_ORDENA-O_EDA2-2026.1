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

---

## Roteiro de Apresentacao

### PESSOA 1 — Patrick (Algoritmos Comparativos e Estrutura do Sistema)

**Duracao estimada: ~10 minutos**

**1. Introducao e Contexto (2 min)**
- Apresentar o problema: controle de trafego aereo precisa priorizar pousos por distancia
- Explicar a metafora: radar ATC onde algoritmos de ordenacao determinam se avioes pousam com seguranca ou crasham
- Mostrar a arquitetura geral: loop principal do raylib, RenderTexture2D para escala, sistema de estados (enum Modo)
- Explicar a estrutura `Voo` (id, callsign, angulo, raio, velocidade, distancia inicial, estado)

**2. Algoritmos Quadraticos O(n^2) (3 min)**
- **Bubble Sort:** empurra o maior para o final a cada passada. No contexto ATC, o aviao mais perto fica "preso" no inicio do array enquanto os maiores sao empurrados — resultado: crashes porque nao prioriza o mais perto rapido o suficiente
- **Selection Sort:** busca o minimo (menor distancia) a cada iteracao e coloca na posicao correta. No ATC, isso prioriza o mais perto imediatamente — resultado: pousos seguros
- **Insertion Sort:** insere cada elemento na posicao correta da sub-lista ja ordenada. Eficiente para dados quase ordenados. Usa shifting ao inves de swaps
- **Shell Sort:** melhoria do Insertion usando gaps decrescentes (sequencia de Knuth: gap = gap*3+1). Permite que elementos distantes se aproximem da posicao final rapidamente. Complexidade O(n log^2 n)
- Demonstrar ao vivo: rodar Bubble vs Selection no simulador, mostrar a diferenca de crashes

**3. Algoritmos Eficientes O(n log n) (3 min)**
- **Merge Sort:** implementacao bottom-up iterativa (sem recursao). Divide em blocos de tamanho crescente (1, 2, 4, 8...) e mescla pares adjacentes. Usa vetor temporario `mTmp`. No ATC: so considera seguro apos terminar completamente (nao tem ordenacao parcial)
- **Quick Sort:** particiona o array em torno de um pivot (ultimo elemento). Usa pilha explicita (`qStack`) ao inves de recursao. Elementos menores que o pivot vao para a esquerda, maiores para a direita
- **Heap Sort:** constroi um max-heap (fase build) e depois extrai o maximo repetidamente (fase extract). Usa sift-down iterativo. Duas fases visiveis na animacao: construcao do heap e extracao

**4. Benchmark e Analise Comparativa (2 min)**
- Mostrar a tela de benchmark (TAB + B)
- Comparar os tempos de execucao com arrays de 100 a 10000 elementos
- Destacar: algoritmos O(n^2) sao pulados em n=10000 por serem muito lentos
- Mostrar o ranking automatico (#1, #2, #3) por tamanho de array

---

### PESSOA 2 — Vinicius (Algoritmos Nao-Comparativos e Visualizacao)

**Duracao estimada: ~10 minutos**

**1. Algoritmos Nao-Comparativos O(n+k) (4 min)**
- **Counting Sort:** ordena sem comparar elementos. Tres fases animadas separadamente:
  - Fase 1: conta quantas vezes cada distancia aparece (array de contagem)
  - Fase 2: prefix sum — acumula as contagens para determinar posicoes finais
  - Fase 3: posiciona cada aviao na posicao correta usando as contagens
  - Usa `DistKey(d) = (int)(d*10)` para converter distancia float em chave inteira
  - Limitacao: precisa de espaco proporcional ao valor maximo (CNT_MAX=3200)

- **Radix Sort LSD (Least Significant Digit):** processa digito a digito do MENOR para o MAIOR significativo. Em cada passada, distribui os elementos em 10 buckets (digitos 0-9), acumula contagens e reposiciona. Variavel `rlExp` controla qual digito esta sendo processado (1, 10, 100...)

- **Radix Sort MSD (Most Significant Digit):** inverso do LSD — processa do MAIOR para o MENOR digito. Usa pilha explicita (`rmStack`) com frames {lo, hi, exp} para processar recursivamente cada bucket que tem mais de 1 elemento. Mais eficiente para dados com distribuicao desigual

- **Bucket Sort:** distribui elementos em baldes baseado na faixa de valores, ordena cada balde internamente com Insertion Sort, e concatena. Tres fases: distribuicao, ordenacao interna, concatenacao. Numero de baldes = 5

**2. Sistema Visual e Interface (3 min)**
- Explicar o sistema de cores e estados visuais:
  - Ciano (C_N): neutro, aguardando
  - Amarelo (C_CMP): sendo comparado neste passo
  - Vermelho/Rosa (C_PIV): candidato/pivot do algoritmo
  - Verde (C_OK): ja esta na posicao correta
  - Verde translucido (C_LAND): pousou com seguranca
  - Vermelho (C_CRASH): colidiu/nao foi priorizado a tempo
- Sistema de particulas para explosoes: struct `Part` com posicao, velocidade, vida util e cor. Funcoes `MkBoom` (gera 35 particulas com direcao aleatoria), `UpdBoom` (atualiza fisica) e `DrBoom` (renderiza com alpha decay)
- Radar: circulos concentricos, varredura animada (sweep), linhas de grade, escala de distancia
- Dashboard: painel lateral com modo atual, delay, comparacoes, trocas, tempo, explicacao do algoritmo, legenda, fila de prioridade e registro de pousos/crashes

**3. Arquitetura Tecnica e Escalabilidade (3 min)**
- **RenderTexture2D:** canvas fixo 1280x720 renderizado em textura, depois escalado para o tamanho da janela mantendo proporcao 16:9. Permite redimensionar sem distorcer
- **Maquina de estados:** cada algoritmo tem funcoes Init (reseta estado) e Step (avanca um passo). O delay `dAnim` controla a velocidade da animacao
- **Colisao entre avioes:** deteccao por distancia euclidiana entre posicoes no radar (limiar de 10 pixels)
- Sistema de benchmark: roda todos os 11 algoritmos com arrays reais, mede com `chrono::high_resolution_clock`, verifica corretude com `is_sorted`, e exibe tabela comparativa com ranking

---
