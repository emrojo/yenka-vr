# Especificación de Requisitos del Sistema - YenkaVR

**Proyecto:** YenkaVR  
**Versión:** 1.0.0  
**Estado:** Aprobado  
**Última Actualización:** 2026-08-21  

---

## 1. Visión General del Proyecto

**YenkaVR** es un videojuego multijugador fotorrealista basado en la física de equilibrio de bloques de madera (*Yenka / Jenga*), desarrollado en **Unreal Engine 5**. Proporciona una experiencia híbrida *cross-play* que une a jugadores en Realidad Mixta (AR/MR), Realidad Virtual pura (VR) y PC de Escritorio (No-VR), con interacción física en tiempo real, gestión de turnos sincronizada y audio espacial.

---

## 2. Histórico de Versiones y Registro de Acuerdos

Este apartado mantiene el historial inmutable de todos los requisitos definidos y acordados entre el equipo de desarrollo y los stakeholders.

| Versión | Fecha | Autor / Stakeholder | Descripción del Cambio / Hitos Acordados |
| :--- | :--- | :--- | :--- |
| **v1.0.0** | 2026-08-21 | Usuario & Antigravity | Definición inicial de requisitos: UE5 + Chaos Physics, autoridad de físicas por turno, MR Passthrough con calibración doble (auto/manual), soporte No-VR con mano virtual, espectador con manipulación de zoom bimanual/drag, Lobbies de Steam + Códigos y VoIP 3D. |
| **v1.1.0** | 2026-08-22 | Usuario & Antigravity | Estabilización física de la torre con micro-holgura lateral de $0.3\text{ mm}$, configuración de Chaos Solver (16 iteraciones), extracción por `PhysicsHandle`, materiales dinámicos de madera realista para bloques/mesa y avatar 3D de mano articulada con cinemática de agarre. |

> [!NOTE]
> Cualquier nuevo requisito o modificación que se acuerde en el futuro deberá añadirse a esta tabla con una versión incremental (`v1.2.0`, etc.) y referenciarse en el archivo [`docs/DECISION_LOG.md`](DECISION_LOG.md).

---

## 3. Requisitos Funcionales (FR)

### 3.1. Núcleo de Juego y Sistema de Turnos (`[REQ-CORE]`)

* **`[REQ-CORE-01]` Temporizador de Turno:**
  * Cada turno activo tendrá un límite estricto de **45 segundos** (configurable en ajustes de sala).
  * Se mostrará un temporizador visual 3D cerca de la torre y un aviso sonoro sutil al entrar en los últimos 10 segundos.
* **`[REQ-CORE-02]` Flujo Secuencial de Jugada:**
  1. El jugador activo extrae un único bloque de cualquier piso excepto del piso superior actual.
  2. El jugador traslada el bloque a la cima de la torre y lo suelta en la orientación ortogonal correspondiente a la capa superior.
  3. La torre debe mantenerse en equilibrio durante al menos **3 segundos** de estabilización tras soltar el bloque.
  4. El jugador pulsa "Confirmar Turno" o el turno pasa automáticamente tras la estabilización.
* **`[REQ-CORE-03]` Sistema de Puntuación Acumulativa:**
  * Los jugadores ganan puntos por cada extracción exitosa basada en la altura del bloque extraído y la estabilidad final.
  * Penalización por rozar o mover excesivamente bloques adyacentes.
  * Bonificaciones por jugadas rápidas y limpias.
* **`[REQ-CORE-04]` Condición de Derrota y Fin de Partida:**
  * Si la torre colapsa (caída de más de 2 bloques no colocados en la cima o desplazamiento del centro de masa que haga caer la estructura), el jugador activo pierde la ronda y se otorgan las puntuaciones acumuladas.

---

### 3.2. Motor Físico y Replicación en Red (`[REQ-PHYS]`)

* **`[REQ-PHYS-01]` Simulación Chaos Physics:**
  * La torre constará de 54 bloques individuales simulados mediante Chaos Rigid Bodies con Continuous Collision Detection (CCD).
  * Dimensiones reglamentarias de bloque de Yenka: $7.5\text{ cm} \times 2.5\text{ cm} \times 1.5\text{ cm}$ con masa de $85\text{ g}$.
  * Material físico de madera realista con fricción estática ($\approx 0.65$), fricción dinámica ($\approx 0.45$) y amortiguación física (`LinearDamping 1.0`, `AngularDamping 1.0`).
* **`[REQ-PHYS-02]` Modelo de Autoridad de Físicas Transferible (*Turn-Owner Authority*):**
  * El cliente del jugador cuyo turno está activo posee la autoridad física (`Network Ownership`) sobre los bloques de la torre para garantizar manipulación a 0 ms de lag.
  * El jugador activo replica periódicamente (30-60 Hz) las posiciones, rotaciones y velocidades lineales/angulares al servidor/host.
  * Los demás clientes interpolan suavemente el estado cinemático de los bloques.
* **`[REQ-PHYS-03]` Congelación de Reposo (*Sleep State Stabilization*):**
  * Al nacer la torre y al finalizar cada turno tras confirmarse el equilibrio, los bloques entran en estado *Sleep* con simulación física y gravedad siempre habilitadas para eliminar derivas de coma flotante.
* **`[REQ-PHYS-04]` Micro-Holgura y Solucionador de Contactos:**
  * Separación lateral de seguridad de $0.3\text{ mm}$ entre los 3 bloques de cada piso (`Offset = (i - 1) * 2.53f`) para impedir tensiones o impulsos expansivos en el frame 0.
  * Separación vertical de $0.3\text{ mm}$ por piso (`CurrentZ = BaseZ + Layer * 1.503f + 0.75f`).
  * Solucionador Chaos calibrado a 16 iteraciones de posición y 8 iteraciones de velocidad (`PositionSolverIterationCount = 16`, `VelocitySolverIterationCount = 8`).
  * Despertar dinámico y colapso por gravedad: al retirar un bloque de apoyo o despejar un piso, los bloques superiores caen y se derrumban por física y peso natural.

---

### 3.3. Realidad Mixta (MR), AR y Calibración Espacial (`[REQ-XR]`)

* **`[REQ-XR-01]` Soporte de Passthrough (Realidad Mixta):**
  * Integración con la extensión OpenXR de Passthrough (`XR_EXT_passthrough` / `XR_FB_passthrough`).
  * En visores compatibles, el entorno 3D es transparente y la torre se proyecta sobre el mundo real.
* **`[REQ-XR-02]` Calibración de Superficie Dual:**
  * **Modo Automático:** Lectura de mallas/planos de la habitación generados por el runtime de OpenXR (`XR_EXT_plane_detection`).
  * **Modo Manual (2/3 Puntos):** El jugador sitúa el mando en las esquinas de su mesa física y pulsa el gatillo para fijar altura, posición y rotación del tablero.
* **`[REQ-XR-03]` Modo VR sin AR (Entorno Inmersivo Puro):**
  * Para usuarios con visores sin cámaras o que prefieran inmersión total, se renderiza una habitación 3D fotorrealista (Ático / Sala de Juegos) con iluminación global Lumen y una mesa central donde se ubica la torre.

---

### 3.4. Soporte PC de Escritorio / No-VR (`[REQ-CROSS]`)

* **`[REQ-CROSS-01]` Renderizado de Entorno No-VR:**
  * En modo pantalla plana (teclado/ratón), el juego siempre renderiza la habitación virtual fotorrealista.
* **`[REQ-CROSS-02]` Avatar de Mano Virtual Replicada:**
  * Mano 3D proporcional y articulada (palma de $6\text{ cm} \times 5\text{ cm} \times 2\text{ cm}$ + pulgar y 4 dedos articulados).
  * Visibilidad contextual inteligente: la mano se oculta en el aire y se posiciona automáticamente sobre el bloque o la mesa al apuntar con el ratón.
  * Cinemática de flexión en pinza/agarre (*grasping pose*) al presionar el clic de agarre.
* **`[REQ-CROSS-03]` Manipulación Física Asistida por Ratón (*Physics Handle*):**
  * **Clic Izquierdo (Mantener):** Engancha el bloque seleccionado con un `UPhysicsHandleComponent` en el punto de contacto exacto.
  * **Arrastre:** Al mover el ratón hacia afuera, el bloque se extrae suavemente respondiendo a la fricción y el peso de los pisos superiores.
  * **Clic Izquierdo (Soltar):** Libera el asa física para dejar caer o asentar el bloque.
  * **Clic Derecho (Arrastrar):** Rotación orbital libre en 360° alrededor de la torre.
  * **Rueda de Ratón:** Zoom de cámara hacia la torre.
  * **Teclas WASD:** Paneo y desplazamiento del punto focal de la cámara.
* **`[REQ-CROSS-04]` Modo Empuje con Dedo Índice (*Finger Poke Mode*):**
  * **Pose de Dedo Índice Extendido:** Al pulsar o mantener la tecla **`E`** (o alternar con **`F`**), la mano 3D adopta la pose de dedo índice totalmente extendido con el pulgar y el resto de dedos cerrados en puño.
  * **Alineación de Contacto:** La punta del dedo índice se coloca de forma precisa sobre el punto de impacto de la cara del bloque apuntado.
* **`[REQ-CROSS-05]` Empuje Longitudinal Guiado de Bloques (*Precision Longitudinal Push*):**
  * Al presionar la acción de empuje (`E` o `Clic Izquierdo` en modo dedo), se calcula el eje longitudinal del bloque.
  * Se aplica un empuje físico guiado en la dirección contraria a la cara presionada, haciendo que el bloque se deslice suavemente hacia adelante y asome por el lado opuesto de la torre ($1$ a $3\text{ cm}$) sin rozar ni desestabilizar los bloques laterales.
  * Permite al jugador rotar la cámara y extraer la pieza cómodamente por el lado opuesto.

---

### 3.5. Arte, Materiales y Apariencia Visual (`[REQ-ART]`)

* **`[REQ-ART-01]` Materiales Dinámicos de Madera Fotorrealista:**
  * Cada bloque de la torre utiliza un sombreador dinámico de madera pulida (haya/roble) con brillo satinado (`Roughness = 0.32`) y variaciones procedurales de color únicas por bloque ($Var \pm 0.07$).
  * Tablero de juego en acabado de madera oscura noble de nogal (`Roughness = 0.40`).
* **`[REQ-ART-02]` Estilización de Avatar y Guante VR:**
  * Acabado en guante de realidad virtual azul cian vibrante con propiedades reflectantes y metálicas para contraste óptimo contra la madera.

---

### 3.6. Interacción y Modo Espectador (`[REQ-CAM]`)

* **`[REQ-CAM-01]` Manipulación Bimanual de Escala en VR (*Pinch Zoom*):**
  * Los jugadores fuera de turno pueden agarrar el espacio con ambas manos y acercar/separar los mandos para escalar el tamaño de la mesa/torre de forma fluida.
* **`[REQ-CAM-02]` Paneo y Rotación del Tablero en VR:**
  * Agarre con una mano en el aire o borde de la mesa para rotar o desplazar el punto de vista sin desincronizar la posición real de la partida.
* **`[REQ-CAM-03]` Cámara Orbital para No-VR:**
  * Control de cámara orbital 360° con clic derecho, paneo con botón central/WASD y zoom con rueda de ratón.

---

### 3.7. Red, Lobbies y Comunicación (`[REQ-NET]`)

* **`[REQ-NET-01]` Integración con Steam Lobbies:**
  * Soporte nativo de `OnlineSubsystemSteam` y `SteamSockets`.
  * Invitaciones directas mediante la lista de amigos de SteamVR / Steam Overlay.
* **`[REQ-NET-02]` Códigos Alfanuméricos de Sala:**
  * Generación de códigos cortos (ej. `YK-9231`) para permitir unirse rápidamente introduciendo el código en el menú principal.
* **`[REQ-NET-03]` Chat de Voz 3D Posicional Integrado:**
  * Integración con **Steam Audio / Voice Engine** de UE5 para VoIP espacializado.
  * Atenuación de volumen por distancia y reverberación según la geometría de la habitación.

---

## 4. Requisitos No Funcionales (NFR)

* **`[NFR-PERF-01]` Tasa de Fotogramas Estable:**
  * Mínimo **90 FPS** estables en VR en configuraciones recomendadas (evitar mareo por movimiento / *motion sickness*).
  * Mínimo **60 FPS** en modo No-VR con Lumen y gráficos fotorrealistas.
* **`[NFR-LAT-02]` Latencia de Interacción Física:**
  * La manipulación local en el turno activo debe tener latencia de respuesta inferior a **15 ms** en VR y PC.
* **`[NFR-SEC-03]` Integridad de Red y P2P:**
  * Conexión a través de Steam Relay Sockets para evitar la exposición de direcciones IP públicas de los jugadores.
* **`[NFR-MOD-04]` Modularidad de Código:**
  * Arquitectura C++ limpia dividida en módulos independientes (`Core`, `Physics`, `Interaction`, `Spatial`, `Network`) para facilitar mantenimiento y extensiones futuras.

---

## 5. Matriz de Trazabilidad

| ID Requisito | Componente Principal | Archivo de Código / Módulo | Estado |
| :--- | :--- | :--- | :--- |
| `[REQ-CORE-01]` | Temporizador Turno (45s) | `Source/YenkaVR/Core/YenkaGameMode.h` | Especificado |
| `[REQ-CORE-02]` | Flujo de Jugada | `Source/YenkaVR/Core/YenkaGameState.h` | Especificado |
| `[REQ-CORE-03]` | Puntuación Acumulativa | `Source/YenkaVR/Core/YenkaPlayerState.h` | Especificado |
| `[REQ-PHYS-01]` | Chaos Physics 54 Bloques | `Source/YenkaVR/Physics/YenkaTowerManager.h` | Implementado |
| `[REQ-PHYS-02]` | Autoridad por Turno | `Source/YenkaVR/Physics/YenkaBlock.h` | Implementado |
| `[REQ-PHYS-03]` | Congelación de Reposo | `Source/YenkaVR/Physics/YenkaTowerManager.cpp` | Implementado |
| `[REQ-PHYS-04]` | Micro-Holgura & Gravity Collapse | `Source/YenkaVR/Physics/YenkaTowerManager.cpp` | Implementado |
| `[REQ-CROSS-02]`| Mano 3D Articulada & Grip | `Source/YenkaVR/Interaction/YenkaHandAvatar.cpp` | Implementado |
| `[REQ-CROSS-03]`| Extracción por PhysicsHandle | `Source/YenkaVR/Interaction/YenkaDesktopPawn.cpp` | Implementado |
| `[REQ-CROSS-04]`| Modo Dedo Índice Extendido | `Source/YenkaVR/Interaction/YenkaHandAvatar.cpp` | Implementado |
| `[REQ-CROSS-05]`| Empuje Longitudinal Guiado | `Source/YenkaVR/Interaction/YenkaDesktopPawn.cpp` | Implementado |
| `[REQ-ART-01]`  | Shaders Madera Fotorrealista | `Source/YenkaVR/Physics/YenkaBlock.cpp` | Implementado |
| `[REQ-XR-01]`   | Passthrough OpenXR | `Source/YenkaVR/Spatial/YenkaPassthroughManager.h` | Especificado |
| `[REQ-XR-02]`   | Calibración Mesa | `Source/YenkaVR/Spatial/YenkaSurfaceCalibrator.h` | Especificado |
| `[REQ-CAM-01]`  | Pinch Zoom Bimanual | `Source/YenkaVR/Interaction/YenkaVRPawn.h` | Especificado |
| `[REQ-NET-01]`  | Steam Lobbies & Codes | `Source/YenkaVR/Network/YenkaSteamLobby.h` | Especificado |
| `[REQ-NET-03]`  | 3D Spatial Audio VoIP | `Source/YenkaVR/Network/YenkaVoiceComponent.h` | Especificado |

