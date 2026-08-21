# YenkaVR 🧱🕹️

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.4%2B-blue?logo=unrealengine)](https://www.unrealengine.com/)
[![Physics](https://img.shields.io/badge/Physics-Chaos%20Engine-orange)](https://docs.unrealengine.com/5.0/en-US/chaos-physics-in-unreal-engine/)
[![Platform](https://img.shields.io/badge/Platforms-SteamVR%20%7C%20OpenXR%20%7C%20PC%20Desktop-green)](#supported-platforms)
[![Cross-Play](https://img.shields.io/badge/Cross--Play-VR%20%2F%20No--VR-brightgreen)](#cross-play)
[![Multiplayer](https://img.shields.io/badge/Multiplayer-Steam%20Lobbies%20%26%20Relay-blueviolet)](https://partner.steamgames.com/)
[![License](https://img.shields.io/badge/License-MIT-lightgrey)](LICENSE)

> **YenkaVR** is a photorealistic multiplayer block-balancing tower game (*Jenga / Yenka*) built in **Unreal Engine 5**. It enables seamless cross-play across **Mixed Reality / AR (Passthrough)**, **pure Virtual Reality (SteamVR / OpenXR)**, and **Desktop PC (No-VR)**, played over a calibrated real-world tabletop or inside a photorealistic virtual environment.

---

## 🌟 Key Features

* 🧱 **Hyper-Realistic Physics Simulation:** Powered by **UE5 Chaos Physics** with Continuous Collision Detection (CCD), calibrated wood friction coefficients, and dynamic micro-clearances.
* 🌐 **Full Cross-Play Support (VR & No-VR):**
  * **VR / MR with Passthrough:** Play directly on your real-world table via OpenXR surface plane detection or manual tactile calibration.
  * **Immersive VR:** Photorealistic 3D virtual environment with real-time dynamic global illumination powered by **Lumen**.
  * **Desktop PC (No-VR):** Full mouse/keyboard controls with visual representation via an articulated **3D Virtual Hand Avatar** visible to everyone in the session.
* ⚡ **"Turn-Owner Physics" Network Architecture:** 0 ms interaction lag by dynamically handing physical network ownership to the active turn player, paired with state synchronization and sleep-freeze stabilization between turns.
* ⏱️ **Competitive Turn-Based Gameplay:**
  * **45-second** turn timer per move.
  * Turn gameplay loop: extract block $\rightarrow$ valid placement on top $\rightarrow$ stability check $\rightarrow$ hand off turn.
  * **Accumulative scoring system** with penalties and tower collapse defeat conditions.
* 🔍 **Interactive Spectator Mode:**
  * **In VR:** Bimanual pinch-to-zoom gestures to scale the tabletop view, and air-grab navigation (*World Drag / Rotate*) to orbit around the tower freely.
  * **In Desktop:** 360° orbital camera control, smooth zoom, and precision panning.
* 👥 **Steam Lobbies & Session Room Codes:**
  * Direct matchmaking through **Steam Lobbies / SteamVR Friends** or short alphanumeric **room join codes** (e.g., `YK-4891`).
* 🎙️ **Integrated 3D Spatial Audio VoIP:** Positional voice chat with realistic distance attenuation and spatial room acoustics.

---

## 🎮 Controls & Gameplay Modes

### In Virtual Reality (SteamVR / OpenXR Controllers / Hand Tracking)
| Action | VR Input |
| :--- | :--- |
| **Grab / Pull Block** | Trigger / Grip button on controller |
| **Rotate Hand / Orient** | Natural wrist orientation (6DOF) |
| **Zoom / Scale Table (Spectator)** | Grab with both hands and pull apart / push together (*Pinch Zoom*) |
| **Move / Rotate Table** | Grab space with 1 hand and drag (*World Drag / Rotate*) |
| **Open Session Menu / Calibrate** | Menu / Y / B Button |

### On Desktop PC (No-VR)
| Action | Keyboard / Mouse Input |
| :--- | :--- |
| **Select / Hover Block** | Mouse pointer (Virtual hand moves horizontally to target) |
| **Grab / Extract Block** | Left Mouse Button (Hold & Drag) via Physics Handle |
| **👉 Finger Poke / Push** | **Hold `E`** (Extends index finger to poke & push block forward) |
| **🔄 Toggle Poke Mode** | **Press `F`** (Toggle between Grab Pinch & Finger Poke mode) |
| **360° Orbital Camera** | Right Mouse Button + Drag |
| **Pan / Move View** | `W`, `A`, `S`, `D` Keys |
| **Camera Zoom** | Mouse Wheel (Scroll) |
| **Confirm / End Turn** | `Spacebar` / On-screen UI Button |

---

## 📐 Table Calibration & Augmented Reality (MR)

The game provides multiple methods to place and anchor the tower:
1. **Automatic Plane Detection (OpenXR `XR_EXT_plane_detection`):** Automatically scans tables and flat surfaces in the physical room and anchors the tower on top.
2. **Tactile Manual Calibration:** The player touches two opposing corners of their physical table with the controller to align height $Z$, position $XY$, and table yaw.
3. **Photorealistic Virtual Environment (No-VR / Pure VR):** For desktop players or headsets without passthrough, the game renders a high-fidelity 3D loft/lounge illuminated with Lumen.

---

## 🏗️ Project Architecture

```text
yenka-vr/
├── Config/                      # Engine, Plugin, and Input configurations
├── Content/                     # 3D Meshes, Textures, PBR Materials, Audio & Blueprints
├── Source/                      # C++ Source Code
│   └── YenkaVR/
│       ├── Core/                # GameMode, GameState, PlayerState, TurnManager
│       ├── Physics/             # YenkaBlock, YenkaTowerManager, Chaos Replicator
│       ├── Interaction/         # YenkaVRPawn, YenkaDesktopPawn, YenkaHandAvatar
│       ├── Spatial/             # SurfaceCalibrator, PassthroughManager
│       └── Network/             # SteamLobbyManager, YenkaVoiceComponent
└── docs/                        # Complete Technical Documentation
    ├── REQUIREMENTS.md          # System Requirements Specification & Traceability
    ├── ARCHITECTURE.md          # Architecture Diagrams, Physics & Networking
    └── DECISION_LOG.md          # Architectural Decision Records (ADR)
```

---

## 📚 Technical Documentation

* 📋 [**Requirements Specification (REQUIREMENTS.md)**](docs/REQUIREMENTS.md): Formal list of functional requirements (`REQ-CORE`, `REQ-PHYS`, `REQ-XR`, `REQ-CROSS`, `REQ-NET`) and version agreements.
* 🏛️ [**System Architecture (ARCHITECTURE.md)**](docs/ARCHITECTURE.md): Network flow diagrams, Chaos physics lifecycle, OpenXR spatial calibration, and spatial VoIP audio.
* 📜 [**Architecture Decision Records (DECISION_LOG.md)**](docs/DECISION_LOG.md): Chronological registry of all design, physics, and architectural decisions agreed upon.

---

## 🚀 Development & Setup

### Prerequisites
* **Unreal Engine 5.4 or higher**.
* **Visual Studio 2022** (with *Game development with C++* and *Windows 10/11 SDK* workloads).
* **Steam Client** running (for testing Steam Lobbies and Steam Relay Sockets).
* For VR: Any **SteamVR / OpenXR** compatible device (Meta Quest 2/3/Pro via Link/AirLink/Virtual Desktop, Valve Index, HTC Vive, Pico 4, etc.).

### Repository Setup
```bash
# Clone the repository
git clone https://github.com/your-username/yenka-vr.git
cd yenka-vr

# Generate Visual Studio project files
# (Right-click YenkaVR.uproject -> "Generate Visual Studio project files")
```

---

## 📄 License

This project is licensed under the **MIT License**. See the `LICENSE` file for details.
