# Prioritizer ATC — Simulador de Triagem de Pouso

**Autores:** patrickacs, Vinicius Castelo  
**Projeto:** EDA 2 — Estruturas de Dados e Algoritmos

Simulador de Radar de Controle de Tráfego Aéreo (ATC) focado na demonstração visual de algoritmos de ordenação: **Bubble Sort** vs **Selection Sort**.

## Conceito

Aviões se aproximam da pista de pouso (centro do radar). O sistema precisa ordenar a fila de pouso por **distância** — quem está mais perto pousa primeiro. Se o algoritmo não conseguir ordenar a tempo, o avião chega sem priorização e **crasha** 💥.

- **Selection Sort** encontra o mais urgente (mínimo) primeiro → salva mais aviões ✅
- **Bubble Sort** empurra o menos urgente (máximo) pro final → deixa urgentes crasharem ❌

## Controles

| Tecla | Ação |
|---|---|
| `[1]` | Iniciar **Bubble Sort** |
| `[2]` | Iniciar **Selection Sort** |
| `[R]` | Resetar e embaralhar a frota |
| `[SPACE]` | Pausar / Despausar simulação |
| `[UP/DOWN]` | Ajustar velocidade da animação |

A janela é redimensionável — o conteúdo escala proporcionalmente (16:9).

## Como Compilar

### Pré-requisitos

**Linux (Debian/Ubuntu)**

Instale o compilador, Make e as dependências gráficas:

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

**Windows**

- [Git para Windows](https://git-scm.com/download/win)
- [MinGW-w64](https://www.mingw-w64.org) (ou via [MSYS2](https://www.msys2.org/))

---

### Build

> **Atenção:** rode `make setup` apenas na primeira vez, ou após limpar os submodules.

```bash
# 1. Clone o repositório (se ainda não tiver feito)
git clone --recurse-submodules <url-do-repositorio>
cd G30_ORDENA-O_EDA2-2026.1

# 2. Primeira vez: baixa e compila a biblioteca raylib
make setup

# 3. Compila e executa o simulador
make
```

Se os submodules já estiverem presentes mas a lib não compilada:

```bash
make lib    # compila apenas a raylib
make        # compila e executa o projeto
```

### Estrutura de Pastas

```
.
├── src/
│   └── main.cpp          # código-fonte principal
├── include/              # headers gerados pelo make setup
├── lib/
│   └── Linux/            # biblioteca estática raylib
├── vendor/
│   ├── raylib/           # submodule: raylib 5.0
│   └── raylib-cpp/       # submodule: raylib-cpp bindings
├── docs/
│   ├── InstallingDependencies.md
│   └── MakefileExplanation.md
├── Makefile
└── README.md
```

## Tecnologias

- **C++17**
- **raylib 5.0** — renderização gráfica ([github.com/raysan5/raylib](https://github.com/raysan5/raylib))
- **raylib-cpp** — bindings C++ ([github.com/robloach/raylib-cpp](https://github.com/robloach/raylib-cpp))