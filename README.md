# YenkaVR 🧱🕹️

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.4%2B-blue?logo=unrealengine)](https://www.unrealengine.com/)
[![Physics](https://img.shields.io/badge/Physics-Chaos%20Engine-orange)](https://docs.unrealengine.com/5.0/en-US/chaos-physics-in-unreal-engine/)
[![Platform](https://img.shields.io/badge/Platforms-SteamVR%20%7C%20OpenXR%20%7C%20PC%20Desktop-green)](#plataformas-soportadas)
[![Cross-Play](https://img.shields.io/badge/Cross--Play-VR%20%2F%20No--VR-brightgreen)](#cross-play)
[![Multiplayer](https://img.shields.io/badge/Multiplayer-Steam%20Lobbies%20%26%20Relay-blueviolet)](https://partner.steamgames.com/)
[![License](https://img.shields.io/badge/License-MIT-lightgrey)](LICENSE)

> **YenkaVR** es una experiencia multijugador fotorrealista de torre de bloques de equilibrio (*Jenga / Yenka*) desarrollada en **Unreal Engine 5**. Permite juego cruzado (*cross-play*) completo entre usuarios de **Realidad Mixta / AR (Passthrough)**, **Realidad Virtual pura (SteamVR / OpenXR)** y **PC de Escritorio (No-VR)** sobre una mesa calibrada en el mundo real o un entorno virtual fotorrealista.

---

## 🌟 Características Principales

* 🧱 **Simulación Física Hiperrealista:** Impulsado por **UE5 Chaos Physics** con detección continua de colisiones (CCD), coeficientes de fricción calibrados para madera real y microfricciones dinámicas.
* 🌐 **Cross-Play Total (VR & No-VR):**
  * **VR / MR con Passthrough:** Juega sobre tu propia mesa física real mediante detección de superficies OpenXR o calibración manual táctil.
  * **VR Inmersivo:** Entorno virtual 3D fotorrealista con iluminación dinámica en tiempo real vía **Lumen**.
  * **PC Desktop (No-VR):** Control con ratón/teclado y representación visual mediante una **mano virtual 3D** visible para todos los jugadores en la sala.
* ⚡ **Arquitectura de Red "Turn-Owner Physics":** Latencia cero al interactuar gracias a la transferencia dinámica de la autoridad física al jugador de turno, con sincronización de estado y estabilización de reposo (*Sleep Freeze*) al cambiar de turno.
* ⏱️ **Modo de Juego por Turnos Competitivo:**
  * Temporizador de **45 segundos** por jugada.
  * Flujo de jugada: extracción de bloque $\rightarrow$ colocación válida en la cima $\rightarrow$ validación de estabilidad $\rightarrow$ traspaso de turno.
  * Sistema de **puntuación acumulativa** con penalizaciones y condición de derrota por colapso.
* 🔍 **Modo Espectador Interactivo:**
  * **En VR:** Gestos bimanuales de pellizco (*pinch-to-zoom*) para escalar la vista de la mesa, y agarre en el aire para rotar y desplazarse libremente alrededor de la torre.
  * **En Desktop:** Control orbital de cámara 360°, zoom milimétrico y paneo suave.
* 👥 **Lobbies de Steam & Códigos de Sala:**
  * Conexión directa mediante **Steam Lobbies / SteamVR Friends** o mediante **códigos alfanuméricos cortos** (ej. `YK-4891`).
* 🎙️ **VoIP 3D Espacializado Integrado:** Chat de voz posicional con atenuación acústica realista y reverberación espacial.

---

## 🎮 Controles y Modos de Juego

### En Realidad Virtual (SteamVR / OpenXR Controllers / Hand Tracking)
| Acción | Control VR |
| :--- | :--- |
| **Agarrar / Tirar Bloque** | Gatillo / Grip (Grip button / Trigger) con la mano |
| **Rotar Mano / Orientar** | Orientación natural de la muñeca (6DOF) |
| **Zoom / Escalar Mesa (Espectador)** | Agarrar con ambas manos y separar/juntar (*Pinch Zoom*) |
| **Moverse / Rotar Mesa** | Agarrar el espacio con 1 mano y arrastrar (*World Drag / Rotate*) |
| **Abrir Menú de Sala / Calibrar** | Botón Menú / Y / B |

### En PC Escritorio (No-VR)
| Acción | Control Teclado / Ratón |
| :--- | :--- |
| **Seleccionar Bloque** | Puntero + Clic Izquierdo (Mano virtual se posiciona) |
| **Tirar / Empujar Bloque** | Arrastre de ratón (X/Y) + Rueda del ratón para profundidad (Z) |
| **Girar Cámara Orbital** | Clic Derecho + Arrastrar |
| **Pan / Mover Vista** | Clic Central (Rueda) + Arrastrar |
| **Zoom de Cámara** | Rueda del Ratón (Scroll) |
| **Puntualizar / Pasar Turno** | Tecla `Espacio` / Botón UI en pantalla |

---

## 📐 Calibración de Mesa y Realidad Aumentada (MR)

El juego cuenta con dos métodos para situar la torre:
1. **Detección Automática de Planos (OpenXR `XR_EXT_plane_detection`):** Detecta mesas y superficies planas en el entorno físico del usuario y sitúa la torre sobre ellas de forma automática.
2. **Calibración Manual por Puntos:** El jugador toca con su mando dos esquinas de su mesa física para alinear altura $Z$, posición $XY$ y rotación de la mesa virtual.
3. **Entorno Virtual Fotorrealista (No-VR / VR Puro):** Para jugadores en PC o visores sin passthrough, el juego renderiza una habitación 3D fotorrealista con iluminación Lumen.

---

## 🏗️ Arquitectura del Proyecto

```text
yenka-vr/
├── Config/                      # Configuraciones de Motor, Plugins e Input
├── Content/                     # Modelos 3D, Texturas, Materiales PBR, Audio y Blueprints
├── Source/                      # Código fuente en C++
│   └── YenkaVR/
│       ├── Core/                # GameMode, GameState, PlayerState, TurnManager
│       ├── Physics/             # YenkaBlock, YenkaTowerManager, Chaos Replicator
│       ├── Interaction/         # YenkaVRPawn, YenkaDesktopPawn, YenkaHandAvatar
│       ├── Spatial/             # SurfaceCalibrator, PassthroughManager
│       └── Network/             # SteamLobbyManager, YenkaVoiceComponent
└── docs/                        # Documentación Técnica Completa
    ├── REQUIREMENTS.md          # Especificación de Requisitos y Trazabilidad Histórica
    ├── ARCHITECTURE.md          # Diagramas de Arquitectura, Físicas y Replicación
    └── DECISION_LOG.md          # Registro de Decisiones de Arquitectura (ADR)
```

---

## 📚 Documentación Técnica

* 📋 [**Especificación de Requisitos (REQUIREMENTS.md)**](docs/REQUIREMENTS.md): Listado formal de requisitos funcionales (`REQ-CORE`, `REQ-PHYS`, `REQ-XR`, `REQ-CROSS`, `REQ-NET`) e históricos.
* 🏛️ [**Arquitectura de Sistemas (ARCHITECTURE.md)**](docs/ARCHITECTURE.md): Diagramas de red, ciclo de vida de físicas Chaos, calibración OpenXR y audio espacial.
* 📜 [**Histórico de Decisiones / ADRs (DECISION_LOG.md)**](docs/DECISION_LOG.md): Registro cronológico de todas las decisiones arquitectónicas y de diseño acordadas.

---

## 🚀 Requisitos de Desarrollo e Instalación

### Prerrequisitos
* **Unreal Engine 5.4 o superior**.
* **Visual Studio 2022** (con soporte para *Desarrollo de juegos con C++* y *Windows 10/11 SDK*).
* **Steam Client** iniciado (para probar la integración con Steam Lobbies y Steam Sockets).
* Para VR: Dispositivo compatible con **SteamVR / OpenXR** (Meta Quest 2/3/Pro vía Link/AirLink/Virtual Desktop, Valve Index, HTC Vive, Pico 4, etc.).

### Configuración del Repositorio
```bash
# Clonar el repositorio
git clone https://github.com/tu-usuario/yenka-vr.git
cd yenka-vr

# Generar archivos de solución de Visual Studio
# (Hacer clic derecho en YenkaVR.uproject -> "Generate Visual Studio project files")
```

---

## 📄 Licencia

Este proyecto está distribuido bajo la licencia **MIT**. Consulta el archivo `LICENSE` para más información.
