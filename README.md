# PARTH&SPACE 🚀

> A 2D Space Shooter built with OpenGL / FreeGLUT in C++  
> Computer Graphics (CSE422) — University Project

---

## Overview

**PARTH&SPACE** is an interactive 2D space shooter where you pilot a fighter craft through endless waves of alien enemies and rotating asteroid fields. The project demonstrates core computer graphics concepts — manual rasterization algorithms, 2D transformations, animation, and real-time user interaction — all implemented from scratch using OpenGL primitives.

---

## Features

| Category | Implementation |
|---|---|
| **Graphics Primitives** | Points (star-field), Lines (lasers, HUD), Polygons (ship/enemy hulls), Circles (planet, bullets, explosions) |
| **DDA Line Algorithm** | Laser beams, engine exhaust trails, wing detail lines |
| **Bresenham Line Algorithm** | Shield grid segments, HUD separator bar, enemy wing stripes |
| **Midpoint Circle Algorithm** | Planet body & atmosphere, bullet glow rings, explosion burst rings |
| **Translation** | Ship movement, enemy drift, bullet travel |
| **Rotation** | Ship banking on turn, enemy wobble, asteroid spin |
| **Scaling** | Ship spawn-in pulse, explosion ring grow-out |
| **Animation** | Parallax star-field scroll, sine-wave enemy drift, continuous asteroid rotation, engine flicker |
| **Keyboard Input** | Full 8-direction ship movement + fire + shield + pause/restart |

---

## Gameplay

You start with **3 lives** and face waves of enemy ships descending from above. Enemies move in a sine-wave pattern — they get faster and more numerous with each wave. Asteroids appear from Wave 2 onward. Your goal is to survive as long as possible and maximise your score.

**Lose a life when:**
- An enemy ship reaches the bottom of the screen (flies past you)
- An enemy or asteroid collides with your ship

**Game over** when all 3 lives are lost.

### Scoring

| Event | Points |
|---|---|
| Destroy an enemy ship | +100 |
| Destroy an asteroid | +50 |
| Clear a full wave | +250 |

### Controls

| Key | Action |
|---|---|
| `W` / `↑` | Move Up |
| `X` / `↓` | Move Down |
| `A` / `←` | Move Left |
| `D` / `→` | Move Right |
| `SPACE` | Fire laser |
| `S` | Activate shield (~1.5 s, 5 s cooldown) |
| `P` | Pause / Resume |
| `R` | Restart |
| `ESC` | Quit |

> **Shield tip:** The shield absorbs all collisions while active. Watch the bottom status bar — it shows when the shield is ready.

---

## Build & Run

### Linux

```bash
# Install dependency
sudo apt install freeglut3-dev

# Compile
g++ -o parthnspace parthnspace.cpp -lGL -lGLU -lglut -lm

# Run
./parthnspace
```

### Windows (MinGW + FreeGLUT)

1. Install [MinGW-w64](https://www.mingw-w64.org/) and add it to PATH
2. Download [FreeGLUT for MinGW](https://www.transmissionzero.co.uk/software/freeglut-devel/)
3. Place `freeglut.h` in `MinGW/include/GL/`, `libfreeglut.a` in `MinGW/lib/`, and `freeglut.dll` next to the `.cpp` file
4. Compile:

```bash
g++ -o parthnspace.exe parthnspace.cpp -lfreeglut -lopengl32 -lglu32 -lm
parthnspace.exe
```

### macOS

```bash
# Install FreeGLUT via Homebrew
brew install freeglut

# Compile
g++ -o parthnspace parthnspace.cpp \
    -I/opt/homebrew/include \
    -L/opt/homebrew/lib \
    -lfreeglut -lGL -lGLU -lm

# Run
./parthnspace
```

> On newer macOS (Apple Silicon), replace `/opt/homebrew` with the output of `brew --prefix` if paths differ.

---

## Project Structure

```
parthnspace.cpp     # Single-file implementation (all logic, algorithms, rendering)
README.md
```

---

## Algorithm Details

### DDA Line Drawing
Computes increments `(dx/steps, dy/steps)` in floating point and steps through each pixel. Simple and clean, used for smooth beam rendering.

### Bresenham Line Drawing
Uses only integer arithmetic with an error accumulator. More efficient than DDA; used for grid lines and HUD elements where precision matters.

### Midpoint Circle Drawing
Uses a decision parameter `p = 1 - r` and 8-way symmetry to plot all octants simultaneously in O(r) steps. All circles in the game — planet, bullet glow, explosions — are drawn this way with no `sin`/`cos` involved.

---

## Requirements

- C++17 or later
- OpenGL 2.x
- FreeGLUT 3.x

---

## Author

**Shuvo Singh Partho** — CSE422 Computer Graphics Lab, Project, openGL