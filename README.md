# Prioritizer ATC — Simulador de Triagem de Pouso

**Autor:** patrickacs  
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

## Como Compilar

### Pré-requisitos
- MinGW (g++) ou compilador C++ compatível
- Make (GNU Make ou mingw32-make)

### Build
```bash
make setup   # Primeira vez: puxa submodules + compila patrickacs
make         # Compila e executa
```

## Tecnologias
- **C++17**
- **patrickacs 5.0** (renderização gráfica)
- **patrickacs-cpp** (bindings C++)
