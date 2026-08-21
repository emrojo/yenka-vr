# System Architecture - YenkaVR

This document describes in depth the technical architecture, Unreal Engine 5 subsystems, networking model, and physics lifecycle implemented in **YenkaVR**.

---

## 1. High-Level Architecture Overview

```mermaid
graph TD
    subgraph ClientLayer ["Client & Input Layer"]
        VRPawn["AYenkaVRPawn (OpenXR 6DOF)"]
        DesktopPawn["AYenkaDesktopPawn (Mouse / Orbit)"]
        HandAvatar["AYenkaHandAvatar (Visual Replicated Hand)"]
    end

    subgraph SpatialLayer ["Spatial & Calibration Layer"]
        PassthroughMgr["AYenkaPassthroughManager (OpenXR MR)"]
        SurfaceCalib["AYenkaSurfaceCalibrator (Plane & Manual)"]
    end

    subgraph CoreLayer ["Game Rules & Turn Management Layer"]
        GameMode["AYenkaGameMode (Server Authority)"]
        GameState["AYenkaGameState (Synced State)"]
        PlayerState["AYenkaPlayerState (Scores, VR Mode)"]
    end

    subgraph PhysicsLayer ["Chaos Physics Layer"]
        TowerMgr["AYenkaTowerManager (54 Blocks Manager)"]
        BlockActor["AYenkaBlock (Chaos RigidBody & Net Owner)"]
    end

    subgraph NetworkLayer ["Networking & Communications Layer"]
        SteamLobby["UYenkaSteamLobbyManager (SteamSockets)"]
        VoiceComponent["UYenkaVoiceComponent (Steam Audio 3D VoIP)"]
    end

    VRPawn --> HandAvatar
    DesktopPawn --> HandAvatar
    SurfaceCalib --> TowerMgr
    GameMode --> GameState
    GameMode --> TowerMgr
    TowerMgr --> BlockActor
    SteamLobby --> GameState
```

---

## 2. Chaos Physics Lifecycle & Turn Authority

In a precision block-physics game, jitter and latency are critical. The physics architecture follows a strict state machine:

```mermaid
stateDiagram-v2
    [*] --> IdleLocked : Tower Spawn & Initialization (54 Blocks)
    IdleLocked --> TurnActive : Turn Start (Assigned Player)
    
    state TurnActive {
        [*] --> AuthorityTransferred : Server assigns SetOwner(PlayerController)
        AuthorityTransferred --> ChaosSimulating : Blocks activate Chaos Simulation + CCD
        ChaosSimulating --> ReplicatingToSpectators : Transform/Velocity Replication (45Hz)
        ReplicatingToSpectators --> BlockPlacedOnTop : Block placed on top layer
    }

    TurnActive --> StabilizationCheck : Block Released
    
    state StabilizationCheck {
        [*] --> MeasuringVelocity : Check angular/linear velocity < epsilon
        MeasuringVelocity --> Stable : 3 seconds without collapse
        MeasuringVelocity --> Collapsed : Block fallen / Tower unstable
    }

    Stable --> IdleLocked : Sleep State Freeze -> Next Turn
    Collapsed --> GameOver : Round Over / Penalty -> Final Score
```

### Physical Material Properties (`UPhysicalMaterial`)
* **Mass per Block:** $0.085 \text{ kg}$ (Balsa/birch wood).
* **Static Friction ($\mu_s$):** $0.65$
* **Dynamic Friction ($\mu_d$):** $0.48$
* **Restitution ($e$):** $0.02$ (Near-zero bounce to prevent artificial jitter).
* **Continuous Collision Detection (CCD):** Enabled on all blocks.
* **Solver Iterations:** `PositionSolverIterationCount = 16`, `VelocitySolverIterationCount = 8`.

---

## 3. Spatial Calibration Pipeline & Mixed Reality (OpenXR)

```mermaid
sequenceDiagram
    autonumber
    actor Player as VR Player
    participant Calib as AYenkaSurfaceCalibrator
    participant OpenXR as OpenXR Runtime (Passthrough)
    participant Tower as AYenkaTowerManager

    Player->>Calib: Initiate table calibration
    alt Automatic Mode (AR)
        Calib->>OpenXR: Request detected planes (XR_EXT_plane_detection)
        OpenXR-->>Calib: Return dominant horizontal plane
        Calib->>Tower: Align Tower Transform to plane normal
    else Manual Mode (Corner Touch)
        Player->>Calib: Touch Corner 1 (Set Origin X, Y, Z)
        Player->>Calib: Touch Corner 2 (Set Yaw Rotation Vector)
        Calib->>Tower: Set Table Transform
    else Pure VR / Desktop Mode
        Calib->>Tower: Use default Transform in 3D Virtual Loft
    end
    Tower->>Tower: Spawn 54 blocks over calibrated table
```

---

## 4. Pawn Architecture & Hand Representation (VR vs No-VR)

* **`AYenkaVRPawn`:**
  * `UCameraComponent` bound to the HMD.
  * `UMotionControllerComponent` (Left and Right) with OpenXR Hand Tracking support.
  * Bimanual *Pinch-to-Zoom* and *World Drag* logic for spectators.
* **`AYenkaDesktopPawn`:**
  * `USpringArmComponent` + orbital `UCameraComponent`.
  * Mouse input for raycasting, horizontal hand lock, and `PhysicsHandle` extraction.
  * Dedicated **Finger Poke Action (`E` / `F`)** for guided longitudinal pushing of lower-level blocks.
* **`AYenkaHandAvatar`:**
  * 3D articulated hand mesh with procedural finger poses (`OpenHand`, `GrabPinch`, `FingerPoke`).
  * Network-replicated: smoothly interpolates position, rotation, and pose mode across all clients.

---

## 5. Network Architecture & Spatial VoIP

```mermaid
sequenceDiagram
    autonumber
    actor Host as Host (Player 1)
    actor Client as Client (Player 2)
    participant Steam as Steamworks / Steam Lobbies
    participant Audio as Steam Audio 3D VoIP

    Host->>Steam: CreateLobby(LobbyType::Public, MaxPlayers: 6)
    Steam-->>Host: Return SteamLobbyID
    Host->>Host: Generate Short Alphanumeric Code (e.g., "YK-8492")
    Client->>Steam: JoinLobbyByCode("YK-4892")
    Steam-->>Client: Resolve SteamLobbyID & connect via SteamSockets
    Client->>Host: RPC Connection successful to GameState
    Host->>Audio: Initialize 3D voice channel attached to Pawn
    Client->>Audio: Initialize 3D voice channel attached to Pawn
    Audio-->>Audio: Distance-attenuated VoIP transmission in 3D space
```
