# Architectural Decision Records (ADR) - YenkaVR

This document records all architectural, technical design, and mechanics decisions agreed upon for the **YenkaVR** project. It follows the standard Architecture Decision Records (ADR) format to maintain an auditable history of why key technical choices were made.

---

## 📋 Decision Index

* [ADR-001: Engine Selection (Unreal Engine 5) & Physics (Chaos Physics)](#adr-001-engine-selection-unreal-engine-5--physics-chaos-physics)
* [ADR-002: Transferable Turn-Owner Physics Authority Model](#adr-002-transferable-turn-owner-physics-authority-model)
* [ADR-003: Multi-Environment Compatibility (Mixed Reality Passthrough, Pure VR & Desktop No-VR)](#adr-003-multi-environment-compatibility-mixed-reality-passthrough-pure-vr--desktop-no-vr)
* [ADR-004: Unified Hand Representation and Asymmetrical Interactions](#adr-004-unified-hand-representation-and-asymmetrical-interactions)
* [ADR-005: Networking via Steam Lobbies, Alphanumeric Room Codes & 3D VoIP](#adr-005-networking-via-steam-lobbies-alphanumeric-room-codes--3d-voip)
* [ADR-006: Chaos Physics Tower Stabilization via Micro-Clearance, Initial Sleep State & Selective Wakeup](#adr-006-chaos-physics-tower-stabilization-via-micro-clearance-initial-sleep-state--selective-wakeup)
* [ADR-007: 3D Articulated Hand Model with VR Shader & Physics Handle Manipulation on PC](#adr-007-3d-articulated-hand-model-with-vr-shader--physics-handle-manipulation-on-pc)
* [ADR-008: Index Finger Poke Pose & Guided Longitudinal Block Pushing](#adr-008-index-finger-poke-pose--guided-longitudinal-block-pushing)
* [ADR-009: Proximity Boundary Constraint, Perpendicular Push Lock & Inspection Perimeter Lock](#adr-009-proximity-boundary-constraint-perpendicular-push-lock--inspection-perimeter-lock)
* [ADR-010: Assisted Stand-Off Proximity Snapping in Push and Grab Modes](#adr-010-assisted-stand-off-proximity-snapping-in-push-and-grab-modes)

---

### ADR-001: Engine Selection (Unreal Engine 5) & Physics (Chaos Physics)
* **Date:** 2026-08-21
* **Status:** Accepted
* **Context:** A photorealistic graphical rendering pipeline (PBR wood, dynamic lighting) was required alongside a robust rigid-body physics engine to simulate the stacking and interaction of 54 blocks without instability.
* **Decision:** Use **Unreal Engine 5.4+** with **Chaos Physics**, leveraging **Lumen** for real-time global illumination and **Nanite** for high-fidelity block geometry.
* **Consequences:**
  * *(Positive)* High visual fidelity and native Continuous Collision Detection (CCD) in Chaos.
  * *(Positive)* Mature plugin ecosystem for OpenXR and Steamworks.
  * *(Trade-off)* Higher GPU/memory footprint compared to lightweight engines, requiring optimization of draw calls and shaders.

---

### ADR-002: Transferable Turn-Owner Physics Authority Model
* **Date:** 2026-08-21
* **Status:** Accepted
* **Context:** In fine block physics games like Jenga, traditional client-server roundtrip latency causes noticeable lag and detachment when grabbing pieces in VR.
* **Decision:** Implement a scheme where the active player receives physical network ownership (`Network Ownership`) of the tower blocks during their 45-second turn. The active player simulates Chaos Physics locally and replicates *Transforms* and velocities to spectators (who interpolate the states kinematically). At turn completion, the state freezes (*Sleep State*) to prevent floating-point drift.
* **Consequences:**
  * *(Positive)* 0 ms input lag for the active VR/PC player when pushing or extracting blocks.
  * *(Positive)* Efficient bandwidth usage, replicating active simulation only during turn interaction.
  * *(Trade-off)* Requires server-side bounds validation in public matchmaking.

---

### ADR-003: Multi-Environment Compatibility (Mixed Reality Passthrough, Pure VR & Desktop No-VR)
* **Date:** 2026-08-21
* **Status:** Accepted
* **Context:** Users possess diverse hardware: color passthrough headsets (Quest 3, Vive XR Elite), pure VR headsets (Valve Index), and traditional desktop PCs without VR headsets.
* **Decision:**
  * For **MR (Passthrough):** Integrate OpenXR Passthrough with automatic plane detection or manual 2-point tactile tabletop calibration.
  * For **Pure VR & No-VR:** Render a photorealistic 3D virtual environment (Lounge/Loft with Lumen dynamic lighting) centered around the game table.
* **Consequences:**
  * *(Positive)* Maximum accessibility: anyone can join regardless of hardware setup.
  * *(Positive)* Contextual immersion tailored to each player's platform.

---

### ADR-004: Unified Hand Representation and Asymmetrical Interactions
* **Date:** 2026-08-21
* **Status:** Accepted
* **Context:** VR players use 6DOF motion controllers or hand tracking, while PC players use keyboard and mouse. All players must visually perceive each other's actions around the table.
* **Decision:**
  * Create a replicated `AYenkaHandAvatar` visible to all clients.
  * In VR, the avatar tracks controller/hand 6DOF poses.
  * In No-VR, the virtual hand aligns kinematically guided by the mouse cursor on the target block.
  * Support spectator interactions: bimanual pinch-zoom and world-drag in VR, plus 360° orbital camera on PC.
* **Consequences:**
  * *(Positive)* Complete visual consistency: VR players see desktop players' hands moving and interacting with blocks in real time.

---

### ADR-005: Networking via Steam Lobbies, Alphanumeric Room Codes & 3D VoIP
* **Date:** 2026-08-21
* **Status:** Accepted
* **Context:** A friendly matchmaking system was required without port forwarding / NAT traversal issues, complemented by integrated spatial voice chat.
* **Decision:**
  * Integrate `OnlineSubsystemSteam` and `SteamSockets`.
  * Generate short alphanumeric join codes (e.g., `YK-4891`) bound to the `SteamLobbyID` for rapid connection.
  * Integrate **Steam Audio** for distance-attenuated 3D positional voice chat.
* **Consequences:**
  * *(Positive)* Secure P2P connection via Steam relay servers.
  * *(Positive)* Acoustic realism: player voices originate from their exact 3D avatar position around the table.

---

### ADR-006: Chaos Physics Tower Stabilization via Micro-Clearance, Initial Sleep State & Selective Wakeup
* **Date:** 2026-08-22
* **Status:** Accepted
* **Context:** In 54-body rigid stacking simulations in Chaos Physics, frame-0 penetration impulses between touching bodies caused immediate collapse upon spawn.
* **Decision:**
  * Add a $0.3\text{ mm}$ lateral safety clearance between adjacent blocks (`Offset = (i - 1) * 2.53f`) and $0.3\text{ mm}$ vertical clearance per layer.
  * Initialize the tower in *Rigid Body Sleep State* upon spawn with `SetSimulatePhysics(true)` and `SetEnableGravity(true)` enabled.
  * Implement selective wakeup: clicking on a block wakes only the target piece (`WakeRigidBody()`), allowing contact forces to propagate naturally and enabling gravity collapse when floor supports are removed.
  * Set 16 position solver iterations (`PositionSolverIterationCount = 16`) and damping to $1.0$.
* **Consequences:**
  * *(Positive)* Rock-solid tower stability on startup with zero jitter or spontaneous collapse.
  * *(Positive)* 100% natural gravity collapse when supporting blocks are extracted.

---

### ADR-007: 3D Articulated Hand Model with VR Shader & Physics Handle Manipulation on PC
* **Date:** 2026-08-22
* **Status:** Accepted
* **Context:** The player's avatar required a clean visual aesthetic (replacing primitive sphere/box placeholders) and smooth physics extraction for desktop mouse input.
* **Decision:**
  * Build a proportional 3D hand avatar with a neutral root component (`HandRoot`), palm ($6\text{ cm} \times 5\text{ cm} \times 2\text{ cm}$), and 5 articulated fingers with grasping kinematics.
  * Assign dynamic cyan-blue VR glove material with metallic/specular highlights.
  * Implement `UPhysicsHandleComponent` interaction on desktop to pull blocks smoothly under friction and load.
* **Consequences:**
  * *(Positive)* Immersive and visually clear hand avatar.
  * *(Positive)* Precise block extraction with continuous physics on desktop.

---

### ADR-008: Index Finger Poke Pose & Guided Longitudinal Block Pushing
* **Date:** 2026-08-22
* **Status:** Accepted
* **Context:** In lower levels where blocks are tight under tower weight, players need to nudge a piece out from one side using a single finger to grab and extract it cleanly from the opposite side without toppling adjacent blocks.
* **Decision:**
  * Create `EHandPoseMode::FingerPoke` in `AYenkaHandAvatar`, extending only the index finger while folding the thumb and other fingers into a fist.
  * Implement poke mode (`bIsPokeModeActive` / `bIsPushingBlock`) bound to `E` (hold) or `F` (toggle) in `AYenkaDesktopPawn`.
  * Lock the hand orientation strictly horizontal (`Pitch = 0°`, `Roll = 0°`) facing the tower center, and apply guided longitudinal force (`AddImpulse`) along the block's main axis.
* **Consequences:**
  * *(Positive)* Faithful reproduction of real-world Jenga poking techniques.
  * *(Positive)* Maximum extraction precision on tightly packed lower floors.

### ADR-009: Proximity Boundary Constraint, Perpendicular Push Lock & Inspection Perimeter Lock
* **Date:** 2026-08-22
* **Status:** Accepted
* **Context:** In desktop mode, unconstrained 3D mouse motion allowed players to accidentally bump into the tower during inspection or slip laterally while attempting to push a block with the index finger.
* **Decision:**
  * Define a proximity threshold of $2\times \text{HandLength} \approx 22.0\text{ cm}$ from the tower boundary ($R_{prox} \approx 25.75\text{ cm}$ from center).
  * Within proximity, strictly enforce horizontal orientation (`Pitch = 0°`, `Roll = 0°`) facing the tower center. Outside proximity, allow free 3D cursor movement.
  * In **Push Mode** (`E` key or Poke toggle `F`), lock the mouse movement to a single radial perpendicular axis (`LockedRadialDirection`), enabling only inward push and outward retraction along the piece's longitudinal axis.
  * In **Inspection Mode** (normal hover), enforce a safe radial perimeter ($R \ge \text{INSPECTION_SAFE_RADIUS} \approx 16.0\text{ cm}$), constraining motion to orbital movement around the tower and vertical floor transitions without allowing inward penetration.
* **Consequences:**
  * *(Positive)* High-precision block pushing with zero risk of lateral slipping into adjacent pieces.
  * *(Positive)* Fail-safe inspection mode preventing accidental collisions with tower blocks.

### ADR-010: Assisted Stand-Off Proximity Snapping in Push and Grab Modes
* **Date:** 2026-08-22
* **Status:** Accepted
* **Context:** Requiring pixel-perfect manual positioning of the virtual hand against target blocks caused accidental contact and premature force exertion, potentially toppling tower blocks before the player intended to push or grab.
* **Decision:**
  * Implement an automatic stand-off proximity snapping system (`GetBlockStandOffLocation`) with a calibrated safety clearance of $D_{clearance} = 8\text{ mm}$.
  * In **Push Mode**, the hand automatically snaps in front of the nearest piece's accessible face in `FingerPoke` posture without touching the piece. Force is only applied when the player deliberately moves the mouse forward past the stand-off clearance.
  * In **Grab Mode**, the open hand hovers at the stand-off distance facing the block end. Grasping and extraction occur exclusively upon pressing and dragging Left Mouse Button.
* **Consequences:**
  * *(Positive)* Completely prevents accidental block collisions and premature forces during aiming or mode switching.
  * *(Positive)* High player control and tactile feedback when intentionally applying force or pulling pieces.

---

## 📝 Template for Future Decisions

When agreeing upon a new design, architecture, or scope decision, copy this template at the bottom of the document:

```markdown
### ADR-XXX: [Descriptive Decision Title]
* **Date:** YYYY-MM-DD
* **Status:** Proposed | Accepted | Deprecated | Superseded by ADR-YYY
* **Context:** [What is the technical/functional problem?]
* **Decision:** [What solution has been agreed upon?]
* **Consequences:**
  * *(Positive)* [Benefits obtained]
  * *(Trade-off)* [Trade-offs or maintenance considerations]
```
