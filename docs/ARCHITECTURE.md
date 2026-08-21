# Arquitectura del Sistema - YenkaVR

Este documento describe en profundidad la arquitectura técnica, los subsistemas de Unreal Engine 5, el modelo de red y el ciclo de vida de físicas implementados en **YenkaVR**.

---

## 1. Visión Global de la Arquitectura

```mermaid
graph TD
    subgraph ClientLayer ["Capa de Cliente y Entrada"]
        VRPawn["AYenkaVRPawn (OpenXR 6DOF)"]
        DesktopPawn["AYenkaDesktopPawn (Mouse / Orbit)"]
        HandAvatar["AYenkaHandAvatar (Visual Replicated Hand)"]
    end

    subgraph SpatialLayer ["Capa Espacial y Calibración"]
        PassthroughMgr["AYenkaPassthroughManager (OpenXR MR)"]
        SurfaceCalib["AYenkaSurfaceCalibrator (Plane & Manual)"]
    end

    subgraph CoreLayer ["Capa de Reglas de Juego y Turnos"]
        GameMode["AYenkaGameMode (Server Authority)"]
        GameState["AYenkaGameState (Synced State)"]
        PlayerState["AYenkaPlayerState (Scores, VR Mode)"]
    end

    subgraph PhysicsLayer ["Capa de Físicas Chaos"]
        TowerMgr["AYenkaTowerManager (54 Blocks Manager)"]
        BlockActor["AYenkaBlock (Chaos RigidBody & Net Owner)"]
    end

    subgraph NetworkLayer ["Capa de Red y Comunicaciones"]
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

## 2. Ciclo de Vida de Físicas Chaos y Autoridad de Turno

En un juego de física fina de bloques, el jitter y la latencia son críticos. La arquitectura de físicas sigue una máquina de estados estricta:

```mermaid
stateDiagram-v2
    [*] --> IdleLocked : Inicialización de la Torre (54 Bloques)
    IdleLocked --> TurnActive : Inicio de Turno (Jugador Asignado)
    
    state TurnActive {
        [*] --> AuthorityTransferred : Servidor asigna SetOwner(PlayerController)
        AuthorityTransferred --> ChaosSimulating : Bloques activan Simulación Chaos + CCD
        ChaosSimulating --> ReplicatingToSpectators : Replicación Transform/Velocity (45Hz)
        ReplicatingToSpectators --> BlockPlacedOnTop : Bloque colocado en la cima
    }

    TurnActive --> StabilizationCheck : Suelta de Bloque
    
    state StabilizationCheck {
        [*] --> MeasuringVelocity : Comprobación de velocidad angular/lineal < epsilon
        MeasuringVelocity --> Stable : 3 segundos sin colapso
        MeasuringVelocity --> Collapsed : Caída de bloques / Torre inestable
    }

    Stable --> IdleLocked : Congelación de Reposo (Sleep State) -> Siguiente Turno
    Collapsed --> GameOver : Fin de Ronda / Penalización -> Puntuación Final
```

### Parámetros Físicos de los Bloques (`UPhysicalMaterial`)
* **Masa por Bloque:** $0.085 \text{ kg}$ (Madera de balsa/abedul).
* **Fricción Estática ($\mu_s$):** $0.65$
* **Fricción Dinámica ($\mu_d$):** $0.48$
* **Restitución ($e$):** $0.02$ (Cero rebote para evitar saltos artificiales).
* **Continuous Collision Detection (CCD):** Habilitado en todos los bloques.

---

## 3. Pipeline de Calibración Espacial y Realidad Mixta (OpenXR)

```mermaid
sequenceDiagram
    autonumber
    actor Player as Jugador VR
    participant Calib as AYenkaSurfaceCalibrator
    participant OpenXR as OpenXR Runtime (Passthrough)
    participant Tower as AYenkaTowerManager

    Player->>Calib: Iniciar calibración de mesa
    alt Modo Automático (AR)
        Calib->>OpenXR: Solicitar planos detectados (XR_EXT_plane_detection)
        OpenXR-->>Calib: Devolver plano horizontal dominante
        Calib->>Tower: Alinear Transform de la Torre a la normal del plano
    else Modo Manual (Toque de Esquinas)
        Player->>Calib: Tocar Esquina 1 (Fijar Origen X, Y, Z)
        Player->>Calib: Tocar Esquina 2 (Fijar Vector de Rotación Yaw)
        Calib->>Tower: Establecer Transform de Mesa
    else Modo VR Puro / No-VR
        Calib->>Tower: Usar Transform por defecto en Sala Virtual 3D
    end
    Tower->>Tower: Generar 54 bloques sobre la mesa calibrada
```

---

## 4. Arquitectura de Jugador y Representación de Manos (VR vs No-VR)

* **`AYenkaVRPawn`:**
  * Componente `UCameraComponent` ligado al HMD.
  * Componentes `UMotionControllerComponent` (Izquierda y Derecha) con soporte para OpenXR Hand Tracking.
  * Lógica bimanual de *Pinch-to-Zoom* y *World Drag* para espectadores.
* **`AYenkaDesktopPawn`:**
  * Componente `USpringArmComponent` + `UCameraComponent` orbital.
  * Entrada de ratón para cálculo de raycast de bloque y profundidad de empuje.
* **`AYenkaHandAvatar`:**
  * Malla 3D de mano con animación procedimental de dedos/pinza.
  * Replicado en red: interpola suavemente la posición enviada por el jugador (sea VR o No-VR).

---

## 5. Arquitectura de Red y VoIP Espacial

```mermaid
sequenceDiagram
    autonumber
    actor Host as Host (Jugador 1)
    actor Client as Cliente (Jugador 2)
    participant Steam as Steamworks / Steam Lobbies
    participant Audio as Steam Audio 3D VoIP

    Host->>Steam: CreateLobby(LobbyType::Public, MaxPlayers: 6)
    Steam-->>Host: Retorna SteamLobbyID
    Host->>Host: Genera Código Corto Alfanumérico (ej: "YK-8492")
    Client->>Steam: JoinLobbyByCode("YK-4892")
    Steam-->>Client: Resuelve a SteamLobbyID y conecta mediante SteamSockets
    Client->>Host: RPC Conexión exitosa al GameState
    Host->>Audio: Inicializar canal de voz 3D asociado al Pawn
    Client->>Audio: Inicializar canal de voz 3D asociado al Pawn
    Audio-->>Audio: Transmisión VoIP atenuada por distancia en el espacio 3D
```
