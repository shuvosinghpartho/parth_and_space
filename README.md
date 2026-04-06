<div align="center">

```
██████╗  █████╗ ██████╗ ████████╗██╗  ██╗    ██╗
██╔══██╗██╔══██╗██╔══██╗╚══██╔══╝██║  ██║   ██╔╝
██████╔╝███████║██████╔╝   ██║   ███████║  ██╔╝
██╔═══╝ ██╔══██║██╔══██╗   ██║   ██╔══██║ ██╔╝
██║     ██║  ██║██║  ██║   ██║   ██║  ██║██╔╝
╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝  & SPACE
```

**A 2D Space Shooter built entirely from scratch with OpenGL / FreeGLUT in C++**

[![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue?style=flat-square&logo=cplusplus)](https://isocpp.org/)
[![Graphics](https://img.shields.io/badge/Graphics-OpenGL%202.x%20%2F%20FreeGLUT-orange?style=flat-square)](https://www.opengl.org/)
[![Course](https://img.shields.io/badge/Course-CSE422%20Computer%20Graphics-purple?style=flat-square)](https://github.com)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-green?style=flat-square)](https://github.com)
[![License](https://img.shields.io/badge/License-MIT-lightgrey?style=flat-square)](LICENSE)

<br/>

*Pilot a fighter craft through endless waves of alien enemies, rotating asteroid fields,*
*and multi-phase boss encounters — all rendered with hand-implemented rasterization algorithms.*

</div>

---

## ✨ Highlights

> Every pixel, circle, and line on screen is drawn using **DDA**, **Bresenham**, or **Midpoint Circle** algorithms — no geometry shortcuts, no GPU shortcuts.

- 🚀 **4 distinct combat zones** — Deep Space, Asteroid Belt, Nebula, and Boss Arena
- 🤖 **5 enemy AI archetypes** — Sine-wave, Zigzag, Tracker, Swarm, and Kamikaze
- 👾 **3-phase boss encounters** that escalate with shield orbs and attack patterns
- 💥 **Particle explosion system** with real physics — gravity, friction, velocity
- ⚡ **6 power-up types** — Rapid Fire, Shield, Slow-Mo, Laser, Homing Missiles, Bomb
- 🎯 **Combo multiplier system** — chain kills to multiply your score up to 5×
- 🌌 **Parallax star field** with 3 depth layers and zone-adaptive tinting
- 📡 **HUD radar**, trail renderer, screen shake, and neon glow effects
- 💾 **Persistent high score** saved to disk across sessions

---

## 🎮 Gameplay

You start with **3 lives** and face escalating waves of enemy ships. Every 5 waves summons a **multi-phase Boss** with shield orbs, burst fire patterns, and an enraged mode. Your goal: survive as long as possible and maximize your score.

**You lose a life when:**
- An enemy reaches the bottom of the screen
- An enemy or asteroid collides with your ship (without shield)

**Game Over** when all 3 lives are lost.

### 🏆 Scoring

| Event | Base Points | Combo Bonus |
|---|---|---|
| Destroy enemy ship | `+100` | Up to **×5** with combos |
| Destroy asteroid | `+50` | Up to **×2** with combos |
| Collect power-up | `+25` | — |
| Clear full wave | `+250` | — |
| Boss defeated | `+1000` | — |

> **Combo tip:** Chain kills without getting hit to stack your multiplier. The combo counter resets on collision.

---

## 🕹️ Controls

| Key | Action |
|---|---|
| `W` / `↑` | Move Up |
| `X` / `↓` | Move Down |
| `A` / `←` | Move Left |
| `D` / `→` | Move Right |
| `ENTER` / `F1` | Fire laser |
| `S` | Activate shield (~1.5 s active, 5 s cooldown) |
| `P` | Pause / Resume |
| `TAB` | Settings menu |
| `R` | Restart |
| `ESC` | Quit |

> **Shield tip:** Absorbs all collisions while active. Watch the bottom status bar — it shows the cooldown state. A shield power-up instantly recharges it.

---

## ⚙️ Power-Ups

Enemies randomly drop glowing power-ups on death. Each one visually tints your ship and trail.

| Icon | Type | Effect | Duration |
|---|---|---|---|
| 🟠 | **Rapid Fire** | Doubles fire rate; orange trail | 5 s |
| 🔵 | **Shield** | Instant shield recharge | Instant |
| 🟣 | **Slow-Mo** | Halves game speed; purple trail | 3 s |
| 🩷 | **Laser** | Widens hit detection; pink tint | 3 s |
| 🟢 | **Homing** | Bullets track nearest enemy; green trail | 5 s |
| 💣 | **Bomb** | Clears all enemies and asteroids on screen | Instant |

---

## 🌌 Combat Zones

The zone transitions automatically as waves progress, each with a unique background palette, nebula color, and ring tint.

| Zone | Waves | Unique Hazard |
|---|---|---|
| **Deep Space** | 1–4 | Animated planet with orbiting moon |
| **Asteroid Belt** | 5–9 | Dense rotating asteroids |
| **Nebula** | 10–14 | Reduced visibility, fast enemies |
| **Boss Arena** | Every 5th wave | Animated black hole; boss spawn |

---

## 🤖 Enemy AI Types

| AI Mode | Behaviour |
|---|---|
| `AI_SINE` | Classic sine-wave drift downward |
| `AI_ZIGZAG` | Sharp horizontal direction reversals |
| `AI_TRACKER` | Actively homes toward the player |
| `AI_SWARM` | Moves in coordinated group formation |
| `AI_KAMIKAZE` | High-speed direct dive at player |

All enemies have an **HP bar**, a flash-on-hit effect, and can return fire in later waves.

---

## 🛠️ Algorithm Details

All rendering is done using **custom implementations of classic rasterization algorithms** — no `glDrawCircle`, no trigonometric shortcuts for lines.

### DDA Line Algorithm
Computes increments `(dx/steps, dy/steps)` in floating-point and steps through each pixel. Used for laser beams, engine exhaust trails, and ship wing detail lines. A `glowLine()` wrapper stacks multiple offset passes to produce neon bloom.

### Bresenham's Line Algorithm
Pure integer arithmetic with an error accumulator — zero floating-point math. Used for HUD separator bars, shield grid segments, enemy wing stripes, planetary ring bands, and all UI borders where hard-edged precision matters.

### Midpoint Circle Algorithm
Uses a decision parameter `p = 1 − r` and **8-way symmetry** to plot all octants in O(r) time. Zero `sin`/`cos` involved. Every circle in the game — planet body, atmosphere halo, bullet glow rings, explosion bursts, shield bubble — is drawn this way. A `glowCircle()` wrapper layers semi-transparent rings outward to simulate neon bloom.

### Particle Physics System
Each explosion spawns 20–38 particles with randomised velocity vectors, per-particle gravity, and a friction coefficient. Particles fade and shrink over their lifetime — no sprite sheets, all computed per frame.

---

## 📐 Transformations Used

| Transform | Applied To |
|---|---|
| **Translation** | Ship movement, enemy drift, bullet travel, power-up bobbing |
| **Rotation** | Ship banking on turn, enemy wobble, asteroid spin, boss entry sweep |
| **Scaling** | Ship spawn-in pulse, explosion ring grow-out, boss scale animation |

---

## 🚀 Build & Run

### Prerequisites

| Platform | Dependency |
|---|---|
| Linux | `freeglut3-dev` |
| Windows | MinGW-w64 + FreeGLUT DLL |
| macOS | FreeGLUT via Homebrew |

---

### 🐧 Linux

```bash
# Install dependency
sudo apt install freeglut3-dev

# Compile
g++ -o parthnspace parthnspace.cpp -lGL -lGLU -lglut -lm

# Run
./parthnspace
```

---

### 🪟 Windows (MinGW + FreeGLUT)

1. Install [MinGW-w64](https://www.mingw-w64.org/) and add it to your `PATH`
2. Download [FreeGLUT for MinGW](https://www.transmissionzero.co.uk/software/freeglut-devel/)
3. Place the headers and libs:
   - `freeglut.h` → `MinGW/include/GL/`
   - `libfreeglut.a` → `MinGW/lib/`
   - `freeglut.dll` → same folder as the `.cpp` file

```bash
g++ -o parthnspace.exe parthnspace.cpp -lfreeglut -lopengl32 -lglu32 -lm
parthnspace.exe
```

---

### 🍎 macOS

```bash
# Install FreeGLUT via Homebrew
brew install freeglut

# Compile (Apple Silicon)
g++ -o parthnspace parthnspace.cpp \
    -I$(brew --prefix)/include \
    -L$(brew --prefix)/lib \
    -lfreeglut -lGL -lGLU -lm

# Run
./parthnspace
```

> On Intel Macs, replace `$(brew --prefix)` with `/usr/local` if needed.

---

## 📁 Project Structure

```
parthnspace/
├── parthnspace.cpp     # Single-file implementation — all logic, algorithms, rendering
└── README.md
```

The entire game — graphics primitives, game logic, AI, UI, particle system, and audio-free SFX feedback — lives in a single `.cpp` file (~2000 lines), intentionally kept as a self-contained academic submission.

---

## 📋 Requirements

| Component | Minimum Version |
|---|---|
| C++ Standard | C++17 |
| OpenGL | 2.x |
| FreeGLUT | 3.x |
| Compiler | GCC 9+ / Clang 10+ / MSVC 2019+ |

---

## 👨‍💻 Author

**Shuvo Singh Partho**
CSE422 — Computer Graphics Lab & Project
OpenGL / FreeGLUT / C++

---

<div align="center">

*Built from scratch. No game engine. No shortcuts. Just math.*

⭐ If you found this useful or interesting, consider starring the repository!

</div>