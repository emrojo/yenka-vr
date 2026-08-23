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
* [ADR-011: 7-Color Balanced Wood-Stain Palette Distribution](#adr-011-7-color-balanced-wood-stain-palette-distribution)
* [ADR-012: Organic Placement Deviation ($\pm 0.2\text{ cm}$) and Safety Boundary Clamp](#adr-012-organic-placement-deviation-pm-02text-cm-and-safety-boundary-clamp)
* [ADR-013: Sequential Piece-by-Piece FIFO Construction with 1.0cm Top-Down Drop](#adr-013-sequential-piece-by-piece-fifo-construction-with-10cm-top-down-drop)
* [ADR-014: Protrusion-Only Selective Grabbing & Tower Structural Integrity Protection](#adr-014-protrusion-only-selective-grabbing--tower-structural-integrity-protection)
* [ADR-015: High-Power Active Push Velocity ($18.0\text{ cm/s}$ - $24.0\text{ cm/s}$) for Lower-Layer Extraction](#adr-015-high-power-active-push-velocity-180text-cms---240text-cms-for-lower-layer-extraction)
* [ADR-016: Double-Click Tap Strike Mechanics (+50% Force / 27.0cm/s + Impulse) in Push Mode](#adr-016-double-click-tap-strike-mechanics-50-force--270cms--impulse-in-push-mode)
* [ADR-017: Standardized Table Surface Elevation ($90.0\text{ cm}$) Above Ground](#adr-017-standardized-table-surface-elevation-900text-cm-above-ground)
* [ADR-018: Anatomical Index-Thumb Caliper Pinch Pose for Block Extraction](#adr-018-anatomical-index-thumb-caliper-pinch-pose-for-block-extraction)
* [ADR-019: Photorealistic Human Skin Subsurface Scattering (SSS) & Skeletal Hand Architecture](#adr-019-photorealistic-human-skin-subsurface-scattering-sss--skeletal-hand-architecture)
* [ADR-020: PC VR 90 FPS Optimization and Meta Quest 2 Lens Clarity Calibration](#adr-020-pc-vr-90-fps-optimization-and-meta-quest-2-lens-clarity-calibration)
* [ADR-021: Parabolic Arc Teleportation ("Caña de Teletransporte") & Snap Turning in VR](#adr-021-parabolic-arc-teleportation-caña-de-teletransporte--snap-turning-in-vr)
* [ADR-022: 15-Phalanx Forward Kinematics & Stationary Viewport Lock in Phalanx Edit Mode](#adr-022-15-phalanx-forward-kinematics--stationary-viewport-lock-in-phalanx-edit-mode)
* [ADR-023: Dynamic JSON-Backed Gesture Library & Default Posture Ingestion](#adr-023-dynamic-json-backed-gesture-library--default-posture-ingestion)
* [ADR-024: Real-Time Live File Watcher & Instant JSON Transform Synchronization](#adr-024-real-time-live-file-watcher--instant-json-transform-synchronization)
* [ADR-025: Strictly Horizontal Perpendicular Block Pulling & Invariant Extraction Orientation](#adr-025-strictly-horizontal-perpendicular-block-pulling--invariant-extraction-orientation)
* [ADR-026: Context-Aware Intelligent Gesture Selection Matrix](#adr-026-context-aware-intelligent-gesture-selection-matrix)
* [ADR-027: 3D Crane Block Manipulation & Top-of-Tower Placement Assist](#adr-027-3d-crane-block-manipulation--top-of-tower-placement-assist)

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
  * Implement an automatic stand-off proximity snapping system (`GetBlockStandOffLocation`) with a calibrated safety clearance of $D_{clearance} = 1.0\text{ cm}$.
  * In **Push Mode**, the hand automatically snaps in front of the nearest piece's accessible face in `FingerPoke` posture without touching the piece. Force is only applied when the player deliberately moves the mouse forward past the stand-off clearance.
  * In **Grab Mode**, the open hand hovers at the stand-off distance facing the block end. Grasping and extraction occur exclusively upon pressing and dragging Left Mouse Button.
* **Consequences:**
  * *(Positive)* Completely prevents accidental block collisions and premature forces during aiming or mode switching.
  * *(Positive)* High player control and tactile feedback when intentionally applying force or pulling pieces.

---

### ADR-011: 7-Color Balanced Wood-Stain Palette Distribution
* **Date:** 2026-08-22
* **Status:** Accepted
* **Context:** The standard uncolored wood blocks lacked visual contrast and party-game vibrancy, making multi-block structure discernment difficult.
* **Decision:**
  * Define a 7-color palette of saturated wood-stain tones: Red, Orange, Yellow, Green, Cyan, Blue, and Purple.
  * Distribute the 54 blocks evenly using modulo indexing ($54 \pmod 7$), yielding 5 groups of 8 blocks and 2 groups of 7 blocks.
  * Apply tinting via dynamic material instances multiplying base albedo while keeping specular roughness ($0.32$) and micro-grain details intact.
  * Replicate `ColorIndex` and `BlockColor` via `OnRep_BlockColor`.
* **Consequences:**
  * *(Positive)* Distinct, beautiful visual identification of pieces while maintaining photorealistic physical wood material properties.
  * *(Positive)* Fully deterministic and network-replicated color assignments.

---

### ADR-012: Organic Placement Deviation ($\pm 0.2\text{ cm}$) and Safety Boundary Clamp
* **Date:** 2026-08-22
* **Status:** Accepted
* **Context:** Perfectly mathematical $0.000\text{ mm}$ grid alignment produced an artificial, robotic tower appearance unlike real human-built Jenga towers, which naturally contain minor offsets and rotational misalignments.
* **Decision:**
  * Maintain Layer 0 as the strict reference base square ($7.5\text{ cm} \times 7.5\text{ cm}$).
  * For Layers $1 \sim 17$, apply organic random displacement:
    * Longitudinal shift between $-0.2\text{ cm}$ and $+0.2\text{ cm}$ ($\pm 2\text{ mm}$).
    * Yaw rotation jitter between $-1.0^\circ$ and $+1.0^\circ$.
    * Adjacent block gap between $1.0\text{ mm}$ and $2.5\text{ mm}$.
  * Implement an automatic clamp rule: no block offset may ever exceed the base boundary by more than $+1.0\text{ cm}$.
* **Consequences:**
  * *(Positive)* Highly authentic, organic look and feel that realistically challenges players' balance strategies.
  * *(Positive)* 100% stable initialization guaranteed by the $+1.0\text{ cm}$ safety perimeter clamp.

---

### ADR-013: Sequential Piece-by-Piece FIFO Construction with 1.0cm Top-Down Drop
* **Date:** 2026-08-22
* **Status:** Accepted
* **Context:** Spawning 54 physics-simulating rigid bodies simultaneously on frame 0 caused collision solver penetration impulses and body-sleep cascades that led to sudden pops when players first touched the tower.
* **Decision:**
  * Queue all 54 blocks into a FIFO construction queue (`PendingSpawnQueue`) and spawn them sequentially from Layer 0 to 17 using a $0.05\text{ s}$ timer (~$2.7\text{ s}$ total).
  * Spawn each block with a $1.0\text{ cm}$ clearance above the top of the underlying layer ($Z_{\text{spawn}} = \text{BaseZ} + (Layer \times 1.50\text{ cm}) + 1.75\text{ cm}$).
  * Allow each piece to fall freely under gravity onto the stack, dampening impact energy and settling naturally into true physical contact equilibrium.
* **Consequences:**
  * *(Positive)* Zero penetration pops; natural physical resting contacts from creation.
  * *(Positive)* Engaging, dynamic top-down tower building animation on game start.

---

### ADR-014: Protrusion-Only Selective Grabbing & Tower Structural Integrity Protection
* **Date:** 2026-08-22
* **Status:** Accepted
* **Context:** In Grab Mode, accidentally clicking flush or deeply recessed blocks in load-bearing layers applied sudden lateral impulses that could collapse the tower unintentionally.
* **Decision:**
  * Evaluate block protrusion relative to the tower perimeter using `IsBlockProtruding()`.
  * Only allow `UPhysicsHandleComponent` grasping if a block's longitudinal end extends at least $4\text{ mm}$ (`PROTRUSION_THRESHOLD = 4.15cm` from center) beyond the flush outer perimeter.
  * If a block is flush or recessed, mouse clicking is ignored. Players must first push the block out in Push Mode before grabbing.
* **Consequences:**
  * *(Positive)* Eliminates accidental grabs that ruin games.
  * *(Positive)* Rewards strategic two-step extraction (Push to reveal edge $\rightarrow$ Grab to extract).

---

### ADR-015: High-Power Active Push Velocity ($18.0\text{ cm/s}$ - $24.0\text{ cm/s}$) for Lower-Layer Extraction
* **Date:** 2026-08-22
* **Status:** Accepted
* **Context:** In lower tower floors, the normal force $N$ exerted by 15+ overhead layers creates high frictional resistance ($\mu N \approx 7.2\text{ N}$), causing gentle pushes to stall or feel unresponsive.
* **Decision:**
  * Increase active longitudinal push velocity to **$18.0\text{ cm/s}$** when holding Left Click in Push Mode.
  * Enable forward mouse motion (`OnMouseY`) to dynamically accelerate the push speed up to **$24.0\text{ cm/s}$**.
  * Harden Chaos solver iterations (`PositionSolverIterationCount = 24`, `VelocitySolverIterationCount = 12`) and physical damping (`LinearDamping 1.2`, `AngularDamping 1.8`).
* **Consequences:**
  * *(Positive)* Authoritative, satisfying piece sliding through tight, heavily weighted layers without toppling adjacent columns.
  * *(Positive)* Precise real-time control via mouse drag and click holding.

---

### ADR-016: Double-Click Tap Strike Mechanics (+50% Force / 27.0cm/s + Impulse) in Push Mode
* **Date:** 2026-08-22
* **Status:** Accepted
* **Context:** In real Jenga matches, players frequently use rapid tap strikes / finger flicks to dislodge sticky or friction-locked pieces quickly before the surrounding structure shifts. A steady continuous push alone does not replicate this tactical tapping technique.
* **Decision:**
  * Track click timestamps with a $350\text{ ms}$ interval window to detect rapid double-clicks on target blocks in Push Mode (`E` / `F`).
  * On double-click, deliver a sharp physical strike ("golpe"):
    * Velocity boosted by $+50\%$ over standard push ($1.5 \times 18.0\text{ cm/s} = \mathbf{27.0\text{ cm/s}}$).
    * Instantaneous longitudinal physical impulse (`AddImpulse`) applied along the piece's locked axis.
* **Consequences:**
  * *(Positive)* Enables authentic high-speed tap-strike strategies for dislodging sticky pieces.
  * *(Positive)* Intuitive desktop interaction pattern (Single Click = Smooth Push, Double Click = Strike / Golpe).

---

### ADR-017: Standardized Table Surface Elevation ($90.0\text{ cm}$) Above Ground
* **Date:** 2026-08-22
* **Status:** Accepted
* **Context:** A standard ergonomic playing height was required for both VR standing/seated comfort and desktop camera focal framing.
* **Decision:**
  * Set `AYenkaTowerManager` world elevation to $Z = 90.0\text{ cm}$.
  * Update fallbacks and camera target anchors across `AYenkaGameMode` and `AYenkaDesktopPawn` to $90.0\text{ cm}$.
* **Consequences:**
  * *(Positive)* Ergonomically ideal height for both VR reach and desktop camera framing.
  * *(Positive)* Uniform table height coordinate referenced cleanly across all modules.

---

### ADR-018: Anatomical Index-Thumb Caliper Pinch Pose for Block Extraction
* **Date:** 2026-08-22
* **Status:** Accepted
* **Context:** In Grab Mode, the previous generic open fist/grip pose did not visually show how the fingers held the piece, creating an unnatural look when extracting blocks.
* **Decision:**
  * Redesign `EHandPoseMode::GrabPinch` in `AYenkaHandAvatar` as a precision caliper pinch:
    * Index finger curves on the right lateral side ($Y = +1.25\text{ cm}$).
    * Thumb extends on the left lateral side ($Y = -1.25\text{ cm}$).
    * The two fingertips span exactly $2.5\text{ cm}$, clamping directly onto the outer corners of the protruding Jenga piece.
    * Middle, ring, and pinky fingers curl into the palm out of the way.
  * Align the hand root offset in `AYenkaDesktopPawn` so the fingertips clamp precisely at the block face during both hover and drag.
* **Consequences:**
  * *(Positive)* Highly photorealistic and anatomically natural pinch pose.
  * *(Positive)* Visual feedback clearly demonstrates firm contact on the lateral sides of the block.

---

### ADR-019: Photorealistic Human Skin Subsurface Scattering (SSS) & Skeletal Hand Architecture
* **Date:** 2026-08-22
* **Status:** Accepted
* **Context:** To achieve an ultra-realistic human appearance, the hand avatar required physical skin material properties (Subsurface Scattering light transmission, lipid layer specular sheen, keratin fingernails) and a MetaHuman-compatible 24-bone skeletal component.
* **Decision:**
  * Integrate `USkeletalMeshComponent` into `AYenkaHandAvatar` to support MetaHuman and standard Unreal mannequin skeletal rigs.
  * Implement Subsurface Scattering (SSS) skin shader properties:
    * Melanin skin tone (`#DCA98C`) with vascular capillary bleed (`#D91508`).
    * Natural lipid layer specular sheen (`SkinRoughness = 0.36`, `Specular = 0.50`).
    * 5 distinct translucent keratin fingernail subcomponents with polished specular reflection (`Roughness = 0.12`).
  * Seamlessly bind procedural kinematics across `OpenHand`, `GrabPinch`, and `FingerPoke`.
* **Consequences:**
### ADR-020: PC VR 90 FPS Optimization and Meta Quest 2 Lens Clarity Calibration
* **Date:** 2026-08-22
* **Status:** Accepted
* **Context:** When running YenkaVR in desktop PC VR mode streamed to a Meta Quest 2 headset (via Quest Link, AirLink, or Virtual Desktop), the default desktop rendering settings (designed for 60 FPS flat screens) overloaded the 90 Hz ($11.11\text{ ms}$) stereo frame budget, triggering ASW reprojection drops to 45 FPS and showing shimmering artifacts on Quest 2 Fresnel lenses.
* **Decision:**
  * Configure `vr.InstancedStereo=1` and `r.SkinCache.CompileShaders=True` for single-pass DirectX 12 hardware stereo drawing.
  * Optimize Lumen for 90 FPS PC VR: `r.Lumen.ScreenProbeGather.DownsampleFactor=16`, `r.LumenScene.RadianceCache.ClipmapResolution=64`, `r.Lumen.Reflections.DownsampleFactor=2`, `r.RayTracing=False`.
  * Optimize Virtual Shadow Maps with directional/local resolution LOD bias (`1`).
  * Eliminate lens blur and shimmering on Quest 2: `r.TSR.Quality=2`, `r.TSR.ShadingRejection.Flickering=1`, `r.Tonemapper.Sharpen=0.8`.
  * Enable Dynamic Resolution Scaling (`r.DynamicRes.OperationMode=2`, `FrameTimeBudget=11.11`) to prevent frame drops during complex tower collapses.
### ADR-021: Parabolic Arc Teleportation ("Caña de Teletransporte") & Snap Turning in VR
* **Date:** 2026-08-22
* **Status:** Accepted
* **Context:** In VR mode, players had no comfortable locomotion method to navigate around the physical table, inspect the tower from different angles, and reposition themselves naturally during extraction turns without physical roomscale walking limitations.
* **Decision:**
  * Implement an intuitive parabolic arc teleportation system ("caña de teletransporte") in `AYenkaVRPawn` activated by forward thumbstick tilt (`Thumbstick Y > 0.55f`).
  * Calculate physics projectile trajectories using `UGameplayStatics::PredictProjectilePath` ($900\text{ cm/s}$ launch speed, gravity $980\text{ cm/s}^2$).
  * Render a dynamic curved spline beam with cyan/green emission for valid horizontal floor surfaces ($\text{ImpactNormal.Z} \ge 0.65$, $Z \le 40\text{ cm}$) and bright neon red for invalid surfaces (table top, tower blocks, walls).
  * Project an aligned holographic ground target ring (`TeleportTargetRing`) at the exact impact point.
  * Execute teleport on thumbstick release with HMD positional offset subtraction so player eye position matches the target point precisely.
  * Implement $45^\circ$ Snap Turning with horizontal thumbstick flicking (`Thumbstick X > 0.6f` / `X < -0.6f`) with $250\text{ ms}$ debouncing.
* **Consequences:**
  * *(Positive)* Completely eliminates VR motion sickness while enabling total, effortless 360-degree navigation around the Jenga table.
  * *(Positive)* Clear visual feedback with the parabolic arc and ground ring.

---

### ADR-022: 15-Phalanx Forward Kinematics & Stationary Viewport Lock in Phalanx Edit Mode
* **Date:** 2026-08-23
* **Status:** Accepted
* **Context:** Fine-tuning hand postures required full articulation across 15 anatomical phalanges. However, when navigating the camera in edit mode (`F4`/`K`), camera movement previously displaced the hand away from view, preventing close-up inspection of modified joints.
* **Decision:**
  * Implement component-space Forward Kinematics evaluating local Pitch, Yaw, and Roll transforms across all 15 bones in `AYenkaHandAvatar::ApplyPhalanxTransforms`.
  * Lock the 3D hand transform strictly stationary in world coordinates upon entering `bIsPhalanxEditMode` (`FixedPhalanxEditTransform`), decoupling orbital camera rotation, WASD panning, and mouse-wheel zoom so the player can zoom in and inspect finger modifications from any perspective.
* **Consequences:**
  * *(Positive)* Intuitive, comfortable 3D pose authoring with complete viewport navigation freedom.

---

### ADR-023: Dynamic JSON-Backed Gesture Library & Default Posture Ingestion
* **Date:** 2026-08-23
* **Status:** Accepted
* **Context:** Hand poses were hardcoded in C++ source files, requiring code compilation to test new gestures or tweak default postures (`FingerPoke`, `GrabPinch`, `OpenHand`).
* **Decision:**
  * Persist hand postures to `Saved/HandGestures/CustomGestures.json` using `FJsonObjectConverter`.
  * Update `AYenkaHandAvatar::LoadPresetPose` and `BeginPlay` to dynamically ingest joint angles from disk.
* **Consequences:**
  * *(Positive)* Posture presets are fully customizable and moddable without touching C++ code.

---

### ADR-024: Real-Time Live File Watcher & Instant JSON Transform Synchronization
* **Date:** 2026-08-23
* **Status:** Accepted
* **Context:** Editing `CustomTransforms.json` or `CustomGestures.json` while the game was running did not update the active offsets because the in-memory array was cached and `BeginPlay` did not copy disk transforms directly into active pawn offsets.
* **Decision:**
  * Implement `CheckForLiveJsonModifications` checking `IFileManager::Get().GetTimeStamp` every frame in `AYenkaDesktopPawn::Tick`.
  * External file saves (`Ctrl+S`) in any text editor trigger instantaneous hot-reloading and viewport update in the very same frame.
  * Replace Euler addition with quaternion composition (`BaseRot.Quaternion() * GrabHandRotationOffset.Quaternion()`) in grab physics manipulation.
  * Bind rapid 90-degree wrist rotation hotkeys (`C` for Yaw, `X` for Pitch, `Z` for Roll) with on-screen HUD feedback.
* **Consequences:**
  * *(Positive)* Zero-friction calibration workflow: change angles in JSON or via hotkeys and see immediate real-time visual results.

---

### ADR-025: Strictly Horizontal Perpendicular Block Pulling & Invariant Extraction Orientation
* **Date:** 2026-08-23
* **Status:** Accepted
* **Context:** During mouse block pulling, `HandleMouseTrace` dynamically recalculated the hand rotation from camera deprojection rays (`GetHorizontalFacingRotation`), causing the hand to spin/snap while dragging. Additionally, 3D mouse deprojection pulled the physics handle downwards into the table surface instead of horizontally outward.
* **Decision:**
  * Lock the extraction hand orientation (`LockedPullHandQuat`) upon grab start using `(-LockedPullDirection).Rotation().Quaternion() * GrabHandRotationOffset.Quaternion()` and keep it invariant throughout the pull.
  * Raycast the mouse cursor against the block's horizontal layer plane ($Z = \text{LockedPullPlaneZ}$) and project displacement strictly onto $\text{LockedPullDirection}$, guaranteeing extraction force is 100% perpendicular to the tower face with constant elevation.
  * Harden `PhysicsHandle` stiffness (`LinearStiffness = 2500`, `InterpolationSpeed = 50`) for immediate, responsive block extraction.
* **Consequences:**
  * *(Positive)* Completely stable hand posture during extraction without angle drift or snapping.
  * *(Positive)* Effortless, clean block extraction along its slot without downward table drag friction.

---

### ADR-026: Face-Specific Context-Aware Intelligent Gesture Selection Matrix
* **Date:** 2026-08-23
* **Status:** Accepted
* **Context:** Switching between pushing, pulling, and grabbing previously required manual key toggles or caused gesture flickering when hovering across different edges of the block.
* **Decision:**
  * Implement an automatic face-specific 4-way gesture matrix evaluated in `HandleMouseTrace`:
    1. **`FingerPoke` (Push):** Activated when cursor is on the small end faces ($2.5\text{ cm} \times 1.5\text{ cm}$) aligned with the longitudinal axis ($\pm \text{ForwardVector}$). Locked in `FingerPoke` during active push until mouse is released.
    2. **`GrabPinch` (Pull):** Activated when cursor is on the protruding section of the large lateral faces ($7.5\text{ cm} \times 1.5\text{ cm}$) extending beyond the tower boundary.
    3. **`VerticalGrab` (Top-Down Claw):** Activated when cursor is on the top face of a block with clearance ($\ge 0.5\text{ cm}$) on both sides of the layer above.
    4. **`OpenHand` (Neutral):** Displayed in all other cases (table, ambient air, blocked top faces, or unprotruding large sides).
* **Consequences:**
  * *(Positive)* Natural, anatomically grounded interaction where small faces push inward and protruding long sides are pinched outward.

---

### ADR-027: 3D Crane Block Manipulation with Smooth S-Curve Elevation & Safe Tower Clearance
* **Date:** 2026-08-23
* **Status:** Accepted
* **Context:** Moving an extracted piece toward the top of the tower could cause collisions with the tower body or protruding blocks if the elevation curve did not clear the tower perimeter at a safe distance. Furthermore, players need to rotate the piece in the horizontal plane to place it neatly on the topmost layer.
* **Decision:**
  * Engage Crane Mode (`bIsCraneGrabbing`) on vertical grab.
  * 2D mouse drag translates the block horizontally in $XY$.
  * Automated Hermite SmoothStep cubic curve ($S(t) = 3t^2 - 2t^3$) with expanded safe clearance:
    * $R_{\text{inner}} = 20.0\text{ cm}$ ($16.25\text{ cm}$ buffer from tower perimeter): achieves $100\%$ apex elevation ($6.0\text{ cm}$ above tower top) well before reaching the tower zone.
    * $R_{\text{outer}} = 30.0\text{ cm}$: settles at $5.0\text{ cm}$ above table surface.
  * Enable horizontal rotation of the suspended piece via mouse wheel (`OnMouseWheel`, $\pm 15^\circ$ per step) in Crane Mode.
  * Live hot-reloading from `Saved/Config/YenkaInteractionConfig.json`.
### ADR-028: Strict Hand & Finger Physical Non-Penetration & Continuous Collision Solver
* **Date:** 2026-08-24
* **Status:** Accepted
* **Context:** In flat desktop and VR interaction, pointing near the tabletop or rotating the camera could cause virtual hand fingers or palm mesh to clip through the table surface or penetrate inside solid tower blocks. The hand must strictly remain in valid physical space above the table and collide realistically with the tower.
* **Decision:**
  * Implement `GetHandAnatomicalSamplePoints` in `AYenkaHandAvatar` sampling 14 anatomical landmarks (wrist, palm, knuckles, and all 5 fingertips).
  * Implement `ValidateAndResolveCollisions`:
    1. Table plane non-penetration constraint ($Z \ge \text{TableSurfaceZ} + 0.25\text{ cm}$): lifts hand if any finger/palm descends below tabletop.
    2. Kinematic OBB depenetration solver: pushes hand outwards along contact normals if any finger penetrates solid non-target tower blocks.
  * Apply `SetTargetHandTransformWithCollision` across all pawn interaction modes (Hover, Push, Pull, Crane, OpenHand, and Phalanx Edit).
* **Consequences:**
  * *(Positive)* Absolute physical non-penetration of table and blocks with zero clipping and natural, believable physical interactions.

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
