# System Requirements Specification - YenkaVR

**Project:** YenkaVR  
**Version:** 1.1.0  
**Status:** Approved  
**Last Updated:** 2026-08-22  

---

## 1. Project Overview

**YenkaVR** is a photorealistic multiplayer video game based on the physics of balancing wooden blocks (*Yenka / Jenga*), developed in **Unreal Engine 5**. It delivers a hybrid cross-play experience uniting players in Mixed Reality (AR/MR), pure Virtual Reality (VR), and Desktop PC (No-VR), featuring real-time physical interaction, synchronized turn management, and 3D spatial audio.

---

## 2. Version History & Agreements Log

This section maintains the immutable history of all requirements defined and agreed upon between the development team and stakeholders.

| Version | Date | Author / Stakeholder | Description of Change / Milestones Agreed |
| :--- | :--- | :--- | :--- |
| **v1.0.0** | 2026-08-21 | User & Antigravity | Initial requirements baseline: UE5 + Chaos Physics, turn-owner physics authority, MR Passthrough with dual calibration (auto/manual), No-VR desktop support with virtual hand avatar, spectator bimanual zoom/drag gestures, Steam Lobbies + Room Codes, and 3D spatial VoIP. |
| **v1.1.0** | 2026-08-22 | User & Antigravity | Physics stabilization via $0.3\text{ mm}$ lateral micro-clearance, Chaos solver configuration (16 position iterations), extraction via `PhysicsHandle`, dynamic procedural wood shaders for blocks and table, articulated 3D hand avatar with grasping kinematics, index finger poke mode, and strictly horizontal hand alignment. |

> [!NOTE]
> Any new requirements or modifications agreed upon in the future must be added to this table with an incremental version tag (`v1.2.0`, etc.) and referenced in [`docs/DECISION_LOG.md`](DECISION_LOG.md).

---

## 3. Functional Requirements (FR)

### 3.1. Core Gameplay & Turn System (`[REQ-CORE]`)

* **`[REQ-CORE-01]` Turn Timer:**
  * Each active turn has a strict **45-second** countdown limit (configurable in room settings).
  * A 3D visual countdown is displayed near the tower, accompanied by a subtle audio warning during the final 10 seconds.
* **`[REQ-CORE-02]` Sequential Turn Flow:**
  1. The active player extracts a single block from any level except the top active layer.
  2. The player places the block onto the top of the tower in the orthogonal orientation corresponding to the top layer.
  3. The tower must remain balanced for at least **3 seconds** of stabilization after the block is released.
  4. The player presses "Confirm Turn" or the turn passes automatically after the stabilization window.
* **`[REQ-CORE-03]` Cumulative Scoring System:**
  * Players earn points for each successful extraction based on block height and final tower stability.
  * Penalties applied for excessive disturbance or contact with adjacent blocks.
  * Bonuses for fast and clean placements.
* **`[REQ-CORE-04]` Defeat Condition & Game Over:**
  * If the tower collapses (more than 2 blocks falling off or center of mass shifting causing structural collapse), the active player loses the round and final scores are tallied.

---

### 3.2. Physics Engine & Network Replication (`[REQ-PHYS]`)

* **`[REQ-PHYS-01]` Chaos Physics Simulation:**
  * The tower consists of 54 individual blocks simulated via Chaos Rigid Bodies with Continuous Collision Detection (CCD).
  * Standard official block dimensions: $7.5\text{ cm} \times 2.5\text{ cm} \times 1.5\text{ cm}$ with a mass of $85\text{ g}$.
  * Realistic wood physical material with static friction ($\approx 0.65$), dynamic friction ($\approx 0.48$), and physical damping (`LinearDamping 1.0`, `AngularDamping 1.0`).
* **`[REQ-PHYS-02]` Transferable Turn-Owner Physics Authority:**
  * The client of the player whose turn is active holds physical authority (`Network Ownership`) over all tower blocks to guarantee 0 ms input lag during manipulation.
  * The active player periodically replicates (30-60 Hz) positions, rotations, and linear/angular velocities to the server/host.
  * Other clients smoothly interpolate the kinematic states of the blocks.
* **`[REQ-PHYS-03]` Sleep State Stabilization:**
  * Upon tower spawn and at the end of each turn after stability is confirmed, blocks enter a rigid body *Sleep* state with physics simulation and gravity permanently enabled to prevent floating-point drift.
* **`[REQ-PHYS-04]` Micro-Clearance & Contact Solver:**
  * Lateral safety gap of $0.3\text{ mm}$ between the 3 adjacent blocks of each floor (`Offset = (i - 1) * 2.53f`) to prevent repulsive normal impulses on frame 0.
  * Vertical layer spacing of $0.3\text{ mm}$ (`CurrentZ = BaseZ + Layer * 1.503f + 0.75f`).
  * Chaos solver calibrated to 16 position iterations and 8 velocity iterations (`PositionSolverIterationCount = 16`, `VelocitySolverIterationCount = 8`).
  * Dynamic wakeup and gravity collapse: when supporting blocks are extracted or a floor is cleared, upper blocks fall and collapse naturally under physical weight and gravity.

---

### 3.3. Mixed Reality (MR), AR & Spatial Calibration (`[REQ-XR]`)

* **`[REQ-XR-01]` Passthrough Support (Mixed Reality):**
  * Integration with OpenXR Passthrough extensions (`XR_EXT_passthrough` / `XR_FB_passthrough`).
  * On supported headsets, the virtual environment is transparent and the tower projects onto the real physical room.
* **`[REQ-XR-02]` Dual Surface Calibration:**
  * **Automatic Mode:** Scans room meshes/planes provided by OpenXR (`XR_EXT_plane_detection`).
  * **Manual Mode (2/3 Points):** The player touches two opposing corners of their real table with the motion controller to lock height, position, and yaw rotation.
* **`[REQ-XR-03]` Pure VR Mode (Immersive Environment):**
  * For headsets without passthrough cameras or players seeking full immersion, a photorealistic 3D loft/lounge is rendered with dynamic Lumen global illumination and a central game table.

---

### 3.4. Desktop PC / No-VR Support (`[REQ-CROSS]`)

* **`[REQ-CROSS-01]` Desktop Environment Rendering:**
  * In desktop mode (mouse & keyboard), the photorealistic 3D room is always rendered.
* **`[REQ-CROSS-02]` Replicated 3D Virtual Hand Avatar:**
  * Articulated proportional 3D hand (palm $6\text{ cm} \times 5\text{ cm} \times 2\text{ cm}$ + thumb and 4 articulated fingers).
  * Contextual smart visibility: hand hides in open air and automatically snaps horizontally onto the hovered block or table.
  * Grasping kinematics (*pinch pose*) during drag interaction.
  * **Strictly Horizontal Alignment:** The hand is locked horizontal (`Pitch = 0°`, `Roll = 0°`) facing towards the tower center, eliminating vertical tilts.
* **`[REQ-CROSS-03]` Mouse-Assisted Physics Handle Extraction:**
  * **Left Click (Hold):** Grips the selected block with a `UPhysicsHandleComponent` at the exact raycast hit point.
  * **Drag:** Moving the mouse outward pulls the block smoothly against friction and the weight of upper floors.
  * **Left Click (Release):** Releases the physics handle to drop or settle the block.
  * **Right Click (Drag):** 360° orbital camera rotation around the tower.
  * **Mouse Wheel:** Smooth camera zoom towards the tower.
  * **WASD Keys:** Panning and focal point translation.
* **`[REQ-CROSS-04]` Index Finger Poke Mode:**
  * **Extended Index Finger Pose:** Holding **`E`** (or pressing **`F`** to toggle) switches the hand into poke mode (index finger fully extended, others closed in a fist).
  * **Contact Alignment:** The index fingertip places precisely against the target block face.
* **`[REQ-CROSS-05]` Precision Longitudinal Block Push:**
  * In poke mode, pressing the push action calculates the longitudinal axis of the block.
* **`[REQ-CROSS-06]` Proximity Horizontal Enforcement ($2\times \text{HandLength}$):**
  * Within a proximity radius of $2\times \text{HandLength}$ ($22.0\text{ cm}$ from the tower boundary), the hand orientation is strictly constrained to be horizontal (`Pitch = 0°`, `Roll = 0°`) facing the tower center.
  * Outside the proximity boundary, the hand moves freely in 3D space following the cursor ray.
* **`[REQ-CROSS-07]` Perpendicular Push Lock & Inspection Perimeter Lock:**
  * **Push Mode Lock:** In Poke/Push mode (`E` key or toggle `F`), mouse movement is locked strictly perpendicular (radial) to the tower face, allowing only inward push or outward retraction along the piece's longitudinal axis to prevent lateral disturbance of neighboring blocks.
  * **Inspection Mode Perimeter Lock:** In normal inspection/hover mode, inward radial motion is clamped to a safe perimeter ($R \ge \text{INSPECTION_SAFE_RADIUS} \approx 16.0\text{ cm}$), allowing only orbital/circumferential movement around the tower and vertical floor transitions, preventing accidental collision with or toppling of tower blocks.
* **`[REQ-CROSS-08]` Assisted Stand-Off Proximity Snapping:**
  * In both **Push Mode** and **Grab Mode**, the hand automatically approaches and aligns with the nearest Jenga block's accessible face at a calibrated safety stand-off clearance ($8\text{ mm}$).
  * The hand stops before contact, ensuring that aiming or activating modes never applies premature pressure or knocks over pieces.
  * Physical pushing or extraction is only initiated upon deliberate forward mouse movement or left-click drag.

---

### 3.5. Art, Materials & Visual Appearance (`[REQ-ART]`)

* **`[REQ-ART-01]` Dynamic Photorealistic Wood Shaders:**
  * Each tower block uses a dynamic polished wood shader (beech/oak) with satin sheen (`Roughness = 0.32`) and unique organic color variations per block ($Var \pm 0.07$).
  * Game tabletop finished in noble dark walnut wood (`Roughness = 0.40`).
* **`[REQ-ART-02]` VR Glove Avatar Styling:**
  * Articulated hand rendered with vibrant cyan-blue VR glove styling featuring metallic/specular highlights.

---

### 3.6. Interaction & Spectator Modes (`[REQ-CAM]`)

* **`[REQ-CAM-01]` Bimanual VR Scale Manipulation (*Pinch Zoom*):**
  * Spectating players can grab 3D space with both hands and pull apart or push together to smoothly scale the table/tower.
* **`[REQ-CAM-02]` VR Tabletop Panning & Rotation:**
  * Single-hand grab in air or table edge to rotate or translate the viewpoint without desynchronizing the match state.
* **`[REQ-CAM-03]` Orbital Camera for Desktop:**
  * Free 360° orbital camera with right click drag, middle click / WASD pan, and scroll wheel zoom.

---

### 3.7. Networking, Lobbies & Communication (`[REQ-NET]`)

* **`[REQ-NET-01]` Steam Lobbies Integration:**
  * Native integration with `OnlineSubsystemSteam` and `SteamSockets`.
  * Direct invites via SteamVR / Steam Overlay friends list.
* **`[REQ-NET-02]` Alphanumeric Room Codes:**
  * Short alphanumeric join codes (e.g., `YK-9231`) for joining without requiring prior Steam friendship.
* **`[REQ-NET-03]` Integrated 3D Positional Voice Chat:**
  * Integration with **Steam Audio / Voice Engine** for spatialized VoIP.
  * Distance attenuation and room reverberation based on the 3D environment.

---

## 4. Non-Functional Requirements (NFR)

* **`[NFR-PERF-01]` Stable Frame Rates:**
  * Minimum **90 FPS** steady in VR on recommended hardware (preventing motion sickness).
  * Minimum **60 FPS** in desktop mode with Lumen and photorealistic settings.
* **`[NFR-LAT-02]` Physics Interaction Latency:**
  * Local turn-owner physical manipulation must have input response under **15 ms** in VR and desktop.
* **`[NFR-SEC-03]` Network Integrity & P2P:**
  * Connection routing through Steam Relay Sockets to prevent exposing public IP addresses.
* **`[NFR-MOD-04]` Code Modularity:**
  * Clean C++ architecture partitioned into independent modules (`Core`, `Physics`, `Interaction`, `Spatial`, `Network`) for long-term maintainability.

---

## 5. Traceability Matrix

| Requirement ID | Primary Component | Source File / Module | Status |
| :--- | :--- | :--- | :--- |
| `[REQ-CORE-01]` | Turn Timer (45s) | `Source/YenkaVR/Core/YenkaGameMode.h` | Specified |
| `[REQ-CORE-02]` | Turn Sequence Flow | `Source/YenkaVR/Core/YenkaGameState.h` | Specified |
| `[REQ-CORE-03]` | Cumulative Score | `Source/YenkaVR/Core/YenkaPlayerState.h` | Specified |
| `[REQ-PHYS-01]` | Chaos Physics 54 Blocks | `Source/YenkaVR/Physics/YenkaTowerManager.h` | Implemented |
| `[REQ-PHYS-02]` | Turn-Owner Authority | `Source/YenkaVR/Physics/YenkaBlock.h` | Implemented |
| `[REQ-PHYS-03]` | Sleep State Freeze | `Source/YenkaVR/Physics/YenkaTowerManager.cpp` | Implemented |
| `[REQ-PHYS-04]` | Micro-Clearance & Gravity Collapse | `Source/YenkaVR/Physics/YenkaTowerManager.cpp` | Implemented |
| `[REQ-CROSS-02]`| Articulated 3D Hand & Horizontal Lock | `Source/YenkaVR/Interaction/YenkaHandAvatar.cpp` | Implemented |
| `[REQ-CROSS-03]`| PhysicsHandle Mouse Extraction | `Source/YenkaVR/Interaction/YenkaDesktopPawn.cpp` | Implemented |
| `[REQ-CROSS-04]`| Extended Index Finger Mode | `Source/YenkaVR/Interaction/YenkaHandAvatar.cpp` | Implemented |
| `[REQ-CROSS-05]`| Guided Longitudinal Push | `Source/YenkaVR/Interaction/YenkaDesktopPawn.cpp` | Implemented |
| `[REQ-CROSS-06]`| Proximity Horizontal Lock (2x Hand) | `Source/YenkaVR/Interaction/YenkaDesktopPawn.cpp` | Implemented |
| `[REQ-CROSS-07]`| Perpendicular & Perimeter Locks | `Source/YenkaVR/Interaction/YenkaDesktopPawn.cpp` | Implemented |
| `[REQ-CROSS-08]`| Assisted Stand-Off Snapping | `Source/YenkaVR/Interaction/YenkaDesktopPawn.cpp` | Implemented |
| `[REQ-ART-01]`  | Photorealistic Wood Shaders | `Source/YenkaVR/Physics/YenkaBlock.cpp` | Implemented |
| `[REQ-XR-01]`   | OpenXR Passthrough | `Source/YenkaVR/Spatial/YenkaPassthroughManager.h` | Specified |
| `[REQ-XR-02]`   | Table Calibration | `Source/YenkaVR/Spatial/YenkaSurfaceCalibrator.h` | Specified |
| `[REQ-CAM-01]`  | Bimanual Pinch Zoom | `Source/YenkaVR/Interaction/YenkaVRPawn.h` | Specified |
| `[REQ-NET-01]`  | Steam Lobbies & Codes | `Source/YenkaVR/Network/YenkaSteamLobby.h` | Specified |
| `[REQ-NET-03]`  | 3D Spatial Audio VoIP | `Source/YenkaVR/Network/YenkaVoiceComponent.h` | Specified |
