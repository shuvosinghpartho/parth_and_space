<div align="center">

# 🚀 PARTH & SPACE — 2D Space Shooter

**A fully hand-crafted 2D Space Shooter built from scratch using OpenGL / FreeGLUT in C++**

[![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue?style=flat-square&logo=cplusplus)](https://isocpp.org/)
[![Graphics](https://img.shields.io/badge/Graphics-OpenGL%202.x%20%2F%20FreeGLUT-orange?style=flat-square)](https://www.opengl.org/)
[![Lines of Code](https://img.shields.io/badge/Lines%20of%20Code-~3500-brightgreen?style=flat-square)]()
[![Course](https://img.shields.io/badge/Course-CSE422%20Computer%20Graphics-purple?style=flat-square)](https://github.com)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightblue?style=flat-square)](https://github.com)
[![License](https://img.shields.io/badge/License-MIT-lightgrey?style=flat-square)](LICENSE)

<br/>

*Pilot a fighter craft through endless alien waves, rotating asteroid fields,*  
*and multi-phase boss encounters — every pixel drawn by hand with classic rasterization algorithms.*

<br/>

> ⚠️ **No game engine. No geometry shortcuts. Just math, C++, and OpenGL.**

</div>

---

## 📸 Preview

> _Screenshot / GIF can be placed here_

---

## ✨ Feature Highlights

| Feature | Details |
|---|---|
| 🎮 **Game States** | Splash → Menu → Settings → Play → Pause → Game Over → Transition |
| 🌌 **4 Combat Zones** | Deep Space · Asteroid Belt · Nebula · Boss Arena |
| 🤖 **5 Enemy AI Types** | Sine-wave · Zigzag · Tracker · Swarm · Kamikaze |
| 👾 **3-Phase Boss** | Shield orbs · burst-fire patterns · enraged mode |
| 💥 **Particle Physics** | 20–38 particles per explosion with gravity + friction |
| ⚡ **6 Power-Up Types** | Rapid Fire · Shield · Slow-Mo · Laser · Homing · Bomb |
| 🎯 **Combo System** | Chain kills to multiply score up to **5×** |
| 🌠 **Parallax Starfield** | 3 depth layers with zone-adaptive colour tinting |
| 📡 **HUD Extras** | Radar · trail renderer · screen shake · neon glow effects |
| 💾 **Persistent Save** | High score written to disk and restored on launch |

---

## 🛠️ Rendering Algorithms

Every visual element is drawn using **hand-implemented rasterization** — no `glBegin(GL_CIRCLE)`, no trig shortcuts.

### DDA Line Algorithm
Computes `(dx/steps, dy/steps)` increments in floating-point and steps pixel by pixel.  
Used for: laser beams, engine exhaust trails, ship wing detail lines.  
A `glowLine()` wrapper stacks multiple offset passes to produce a neon bloom effect.

### Bresenham's Line Algorithm
Pure integer arithmetic with an error accumulator — zero floating-point math.  
Used for: HUD separator bars, shield grid segments, enemy wing stripes, planetary ring bands, and all UI borders where hard-edged precision matters.

### Midpoint Circle Algorithm
Uses decision parameter `p = 1 − r` and **8-way symmetry** to plot all octants in O(r) time — no `sin` or `cos`.  
Used for: planet body, atmosphere halo, bullet glow rings, explosion bursts, shield bubble.  
A `glowCircle()` wrapper layers semi-transparent rings outward to simulate neon bloom.

### Particle Physics System
Each explosion spawns 20–38 particles with:
- Randomised velocity vectors `(vx, vy)`
- Per-particle gravity coefficient
- Friction deceleration per frame
- Alpha + size fade over particle lifetime

No sprite sheets. All computed per frame.

---

## 📐 OpenGL Transformations

| Transform | Applied To |
|---|---|
| **Translation** | Ship movement · enemy drift · bullet travel · power-up bobbing |
| **Rotation** | Ship banking on turn · enemy wobble · asteroid spin · boss entry sweep |
| **Scaling** | Ship spawn-in pulse · explosion ring grow-out · boss scale animation |

---

## 🌌 Combat Zones

Zones transition automatically as waves progress, each with a unique background palette and hazard.

| Zone | Waves | Background Hazard |
|---|---|---|
| **Deep Space** | 1 – 4 | Animated planet with orbiting moon |
| **Asteroid Belt** | 5 – 9 | Dense rotating asteroids |
| **Nebula** | 10 – 14 | Reduced visibility, faster enemies |
| **Boss Arena** | Every 5th wave | Animated black hole + boss spawn |

---

## 🤖 Enemy AI Types

| AI Mode | Behaviour |
|---|---|
| `AI_SINE` | Classic sine-wave drift downward |
| `AI_ZIGZAG` | Sharp horizontal direction reversals |
| `AI_TRACKER` | Actively homes toward the player |
| `AI_SWARM` | Moves in coordinated group formation |
| `AI_KAMIKAZE` | High-speed direct dive at the player |

All enemies have an **HP bar**, a flash-on-hit effect, and can return fire in later waves.

---

## ⚡ Power-Ups

Enemies randomly drop glowing power-ups on death. Each visually tints your ship and trail.

| Type | Effect | Duration |
|---|---|---|
| 🟠 **Rapid Fire** | Doubles fire rate; orange trail | 5 s |
| 🔵 **Shield** | Instantly recharges shield | Instant |
| 🟣 **Slow-Mo** | Halves game speed; purple trail | 3 s |
| 🩷 **Laser** | Widens hit detection; pink tint | 3 s |
| 🟢 **Homing** | Bullets auto-track nearest enemy; green trail | 5 s |
| 💣 **Bomb** | Clears all enemies and asteroids instantly | Instant |

---

## 🏆 Scoring

| Event | Base Points | Combo Bonus |
|---|---|---|
| Destroy enemy ship | +100 | Up to **×5** |
| Destroy asteroid | +50 | Up to **×2** |
| Collect power-up | +25 | — |
| Clear full wave | +250 | — |
| Defeat boss | +1000 | — |

> **Combo tip:** Chain kills without taking damage to stack your multiplier. Any collision resets it.

---

## 🎮 Controls

| Key | Action |
|---|---|
| `W` / `↑` | Move Up |
| `X` / `↓` | Move Down |
| `A` / `←` | Move Left |
| `D` / `→` | Move Right |
| `ENTER` / `F1` | Fire laser |
| `S` | Activate shield (~1.5 s active, 5 s cooldown) |
| `P` | Pause / Resume |
| `TAB` | Open Settings menu |
| `R` | Restart |
| `ESC` | Quit |

> **Shield tip:** Absorbs all collisions while active. The bottom status bar shows the cooldown state. A Shield power-up instantly recharges it.

---

## 🚀 Build & Run

### Prerequisites

| Platform | Required |
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
g++ -std=c++17 -O2 -o parthnspace parthnspace.cpp -lGL -lGLU -lglut -lm

# Run
./parthnspace
```

---

### 🪟 Windows (MinGW + FreeGLUT)

1. Install [MinGW-w64](https://www.mingw-w64.org/) and add it to your `PATH`
2. Download [FreeGLUT for MinGW](https://www.transmissionzero.co.uk/software/freeglut-devel/)
3. Place headers and libs:
   - `freeglut.h` → `MinGW/include/GL/`
   - `libfreeglut.a` → `MinGW/lib/`
   - `freeglut.dll` → same folder as the `.cpp` file

```bash
g++ -std=c++17 -O2 -o parthnspace.exe parthnspace.cpp -lfreeglut -lopengl32 -lglu32 -lm
parthnspace.exe
```

---

### 🍎 macOS

```bash
# Install FreeGLUT via Homebrew
brew install freeglut

# Compile
g++ -std=c++17 -O2 -o parthnspace parthnspace.cpp \
    -I$(brew --prefix)/include \
    -L$(brew --prefix)/lib \
    -lfreeglut -lGL -lGLU -lm

# Run
./parthnspace
```

> On Intel Macs, replace `$(brew --prefix)` with `/usr/local` if needed.

---

## 📋 Requirements

| Component | Minimum |
|---|---|
| C++ Standard | C++17 |
| OpenGL | 2.x |
| FreeGLUT | 3.x |
| Compiler | GCC 9+ / Clang 10+ / MSVC 2019+ |

---

## 📁 Project Structure

```
parthnspace/
├── parthnspace.cpp     # ~3500-line single-file implementation
│                       # Covers: rendering · game logic · enemy AI ·
│                       #         particle system · HUD · power-ups · boss · save
└── README.md
```

The entire game — graphics primitives, AI, UI, particle physics, input handling, and persistence — lives in a **single `.cpp` file**, intentionally structured as a self-contained academic submission.

---

## 🧠 Architecture Overview

```
main()
 └── glutMainLoop()
      ├── display()          — calls all draw* functions per frame
      ├── update()           — game logic tick (~60 fps via glutTimerFunc)
      │    ├── updateBoss()
      │    ├── updateRings()
      │    └── collision detection, wave spawning, power-up timers
      ├── keyDown/keyUp()    — keyboard input (WASD + special keys)
      └── spDown/spUp()      — arrow key + F1 handling
```

---

## 👨‍💻 Author

**Shuvo Singh Partho**  
CSE422 — Computer Graphics Lab & Project  
OpenGL / FreeGLUT / C++17

---

## 🤝 Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

1. Fork the repository
2. Create your branch: `git checkout -b feature/your-feature`
3. Commit your changes: `git commit -m "Add your feature"`
4. Push to the branch: `git push origin feature/your-feature`
5. Open a Pull Request

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).

---

<div align="center">

*Built from scratch. No game engine. No shortcuts. Just math.*

⭐ **If you found this useful or interesting, consider starring the repository!**

</div>