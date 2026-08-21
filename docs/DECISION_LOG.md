# Registro Histórico de Decisiones de Arquitectura (ADR) - YenkaVR

Este documento registra todas las decisiones de arquitectura, diseño técnico y mecánicas acordadas para el proyecto **YenkaVR**. Sigue el formato estándar de *Architecture Decision Records* (ADR) para mantener un histórico auditable de por qué se tomaron ciertas decisiones.

---

## 📋 Índice de Decisiones

* [ADR-001: Selección de Motor (Unreal Engine 5) y Físicas (Chaos Physics)](#adr-001-selección-de-motor-unreal-engine-5-y-físicas-chaos-physics)
* [ADR-002: Modelo de Autoridad de Físicas Transferible por Turno (*Turn-Owner Physics*)](#adr-002-modelo-de-autoridad-de-físicas-transferible-por-turno-turn-owner-physics)
* [ADR-003: Compatibilidad Multi-Entorno (Realidad Mixta Passthrough, VR Puro y Escritorio No-VR)](#adr-003-compatibilidad-multi-entorno-realidad-mixta-passthrough-vr-puro-y-escritorio-no-vr)
* [ADR-004: Representación Unificada de Manos e Interacciones Asimétricas](#adr-004-representación-unificada-de-manos-e-interacciones-asimétricas)
* [ADR-005: Sistema de Conectividad mediante Steam Lobbies, Códigos Alfanuméricos y VoIP 3D](#adr-005-sistema-de-conectividad-mediante-steam-lobbies-códigos-alfanuméricos-y-voip-3d)
* [ADR-006: Estabilización de Torre Chaos Physics mediante Micro-Holgura, Sleep Inicial y Despertar Selectivo](#adr-006-estabilización-de-torre-chaos-physics-mediante-micro-holgura-sleep-inicial-y-despertar-selectivo)
* [ADR-007: Modelo 3D de Mano Articulada con Shader VR y Manipulación por Physics Handle en PC](#adr-007-modelo-3d-de-mano-articulada-con-shader-vr-y-manipulación-por-physics-handle-en-pc)

---

### ADR-001: Selección de Motor (Unreal Engine 5) y Físicas (Chaos Physics)
* **Fecha:** 2026-08-21
* **Estado:** Aceptado
* **Contexto:** Se requiere un entorno que proporcione gráficos fotorrealistas de alta gama (madera física, iluminación dinámica) junto con un motor de físicas de cuerpos rígidos robusto para simular el apilamiento de 54 bloques sin inestabilidades.
* **Decisión:** Utilizar **Unreal Engine 5.4+** con **Chaos Physics**, aprovechando **Lumen** para iluminación global en tiempo real y **Nanite** para modelos de bloques con detalle geométrico extremo.
* **Consecuencias:**
  * *(Positivo)* Fidelidad visual insuperable y simulación de colisiones continua (CCD) nativa en Chaos.
  * *(Positivo)* Ecosistema maduro de plugins para OpenXR y Steamworks.
  * *(A tener en cuenta)* Mayor consumo de memoria/GPU respecto a motores ligeros, requiriendo optimización de draw calls y shaders.

---

### ADR-002: Modelo de Autoridad de Físicas Transferible por Turno (*Turn-Owner Physics*)
* **Fecha:** 2026-08-21
* **Estado:** Aceptado
* **Contexto:** En un juego de física fina como Jenga, la latencia cliente-servidor tradicional causa sensación de desprendimiento o retraso al agarrar bloques en VR.
* **Decisión:** Implementar un esquema donde el jugador activo recibe la propiedad de red (`Network Ownership`) de los bloques de la torre durante sus 45 segundos de turno. El jugador activo simula Chaos Physics localmente y replica *Transforms* y velocidades al resto de jugadores (que actúan como receptores cinemáticos interpolados). Al finalizar el turno, el estado se congela (*Sleep*) para evitar desincronización por coma flotante.
* **Consecuencias:**
  * *(Positivo)* 0 ms de lag de entrada para el jugador en VR al empujar o tirar del bloque.
  * *(Positivo)* Tráfico de red eficiente, replicando solo durante el turno activo.
  * *(A tener en cuenta)* Requiere validación de servidor para evitar trampas en partidas competitivas públicas.

---

### ADR-003: Compatibilidad Multi-Entorno (Realidad Mixta Passthrough, VR Puro y Escritorio No-VR)
* **Fecha:** 2026-08-21
* **Estado:** Aceptado
* **Contexto:** Los usuarios disponen de diferente hardware: visores con Passthrough a color (Quest 3, Vive XR Elite), visores VR puros (Valve Index) y monitores de PC tradicionales sin visor.
* **Decisión:**
  * Para **MR (Passthrough):** Usar el plugin OpenXR Passthrough y permitir calibración automática por detección de planos o manual por toque en esquinas de la mesa real.
  * Para **VR Puro y No-VR:** Cargar un entorno 3D fotorrealista (Ático / Sala de Juegos moderna con iluminación Lumen) con una mesa central personalizable.
* **Consecuencias:**
  * *(Positivo)* Máxima accesibilidad: cualquier jugador puede unirse sin importar si tiene visor o PC estándar.
  * *(Positivo)* Inmersión contextual para cada tipo de jugador.

---

### ADR-004: Representación Unificada de Manos e Interacciones Asimétricas
* **Fecha:** 2026-08-21
* **Estado:** Aceptado
* **Contexto:** Los jugadores de VR usan mandos 6DOF o hand tracking, mientras que los de PC usan ratón y teclado. Es necesario que todos los jugadores vean visualmente la acción de los demás en la sala.
* **Decisión:**
  * Crear un actor `AYenkaHandAvatar` visible para todos los clientes en red.
  * En VR, el avatar sigue la pose de los mandos/manos del jugador.
  * En No-VR, la mano virtual se orienta y desplaza de forma cinemática guiada por el cursor del ratón hacia el bloque seleccionado.
  * Habilitar gestos de espectador: zoom bimanual (*pinch-to-zoom*) y arrastre de tablero (*world drag*) en VR, y cámara orbital para PC.
* **Consecuencias:**
  * *(Positivo)* Coherencia visual total: los jugadores de VR ven las manos de los jugadores de PC interactuar con los bloques en tiempo real.

---

### ADR-005: Sistema de Conectividad mediante Steam Lobbies, Códigos Alfanuméricos y VoIP 3D
* **Fecha:** 2026-08-21
* **Estado:** Aceptado
* **Contexto:** Se requiere un sistema de matchmaking y salas fácil de usar para grupos de amigos, sin necesidad de configurar puertos de router (NAT traversal), y con comunicación por voz integrada.
* **Decisión:**
  * Integrar `OnlineSubsystemSteam` y `SteamSockets`.
  * Generar un código alfanumérico corto (ej. `YK-4891`) vinculado al `SteamLobbyID` para permitir conexión rápida sin necesidad de estar en la lista de amigos.
  * Integrar **Steam Audio** para chat de voz 3D posicional, atenuando el audio según la distancia a la cabeza del avatar del jugador.
* **Consecuencias:**
  * *(Positivo)* Conexión directa P2P segura mediante los relays de Steam.
  * *(Positivo)* Inmersión acústica: escuchar la voz de tus amigos venir exactamente de donde se encuentra su avatar alrededor de la mesa.

---

### ADR-006: Estabilización de Torre Chaos Physics mediante Micro-Holgura, Sleep Inicial y Despertar Selectivo
* **Fecha:** 2026-08-22
* **Estado:** Aceptado
* **Contexto:** En simulaciones de apilamiento de 54 cuerpos rígidos en Chaos Physics, el contacto directo sin holgura en el frame 0 acumula micro-impulsos normales de penetración numérica que causaban la caída inmediata de la torre al nacer.
* **Decisión:**
  * Implementar una micro-holgura lateral de seguridad de $0.3\text{ mm}$ entre los 3 bloques de cada capa (`Offset = (i - 1) * 2.53f`) y $0.3\text{ mm}$ de holgura vertical por capa.
  * Mantener la torre en estado *Rigid Body Sleep* al spawnear con `SetSimulatePhysics(true)` y `SetEnableGravity(true)` habilitados.
  * Despertar selectivo: al hacer clic en un bloque, solo se despierta la pieza interactuada (`WakeRigidBody()`), propagando el movimiento a bloques vecinos por contacto y permitiendo el colapso gravitatorio cuando se retira el soporte de un piso.
  * Configurar 16 iteraciones del solucionador de posición (`PositionSolverIterationCount = 16`) y amortiguamiento de $1.0$.
* **Consecuencias:**
  * *(Positivo)* Estabilidad absoluta de la torre en el arranque sin vibraciones ni derrumbes espontáneos.
  * *(Positivo)* Colapso por gravedad 100% natural y realista al extraer piezas de soporte.

---

### ADR-007: Modelo 3D de Mano Articulada con Shader VR y Manipulación por Physics Handle en PC
* **Fecha:** 2026-08-22
* **Estado:** Aceptado
* **Contexto:** El avatar del jugador requería una representación estética clara (en lugar de figuras primitivas o esferas) y una mecánica de extracción física fluida para jugadores de ratón y teclado.
* **Decisión:**
  * Construir un avatar de mano 3D proporcional con nodo raíz neutro (`HandRoot`), palma ($6\text{ cm} \times 5\text{ cm} \times 2\text{ cm}$) y 5 dedos articulados con cinemática de flexión en pinza/agarre al accionar el clic.
  * Asignar material dinámico de guante VR en color azul cian eléctrico con brillo metálico/especular.
  * Implementar manipulación física asistida por `UPhysicsHandleComponent` para arrastrar bloques suavemente con el ratón respetando la fricción y el peso de las capas superiores.
* **Consecuencias:**
  * *(Positivo)* Identidad visual atractiva e inmersiva.
  * *(Positivo)* Extracción precisa de piezas con física continua y control intuitivo en pantalla plana.

---

## 📝 Plantilla para Nuevas Decisiones Futuras

Al acordar una nueva decisión de diseño, arquitectura o cambio de alcance, copia este bloque al final del documento:

```markdown
### ADR-XXX: [Título descriptivo de la decisión]
* **Fecha:** AAAA-MM-DD
* **Estado:** Propuesto | Aceptado | Descartado | Reemplazado por ADR-YYY
* **Contexto:** [¿Cuál es el problema o necesidad técnica/funcional?]
* **Decisión:** [¿Qué solución se ha acordado implementar?]
* **Consecuencias:**
  * *(Positivo)* [Beneficios obtenidos]
  * *(A tener en cuenta)* [Trade-offs o consideraciones de mantenimiento]
```
