# DEVLOG — Labrynn (2026 Epic AI MegaJam)

Equipo: Enzo y Luis. Asistente de IA: Claude Code (modelos Claude Sonnet 5.5 / Opus 5.5).
Zona horaria de las entradas: Paraguay (UTC-3).

---

## 2026-11-12 (jueves) — Preparación de la PC de Enzo

**13:10 · Diagnóstico** (hecho por Claude)
- Windows 11 Pro build 26300, Ryzen 7 9700X, 31 GB RAM, RTX 5070.
- Discos: C: = Samsung 990 EVO Plus NVMe SSD (280 GB libres); D: = HDD Seagate (647 GB libres).
  - Decisión: Unreal y el proyecto van en **C:** (SSD). Proyecto en `C:\Dev\Labrynn`, fuera de OneDrive
    (OneDrive bloquea archivos de `Intermediate`/`DerivedDataCache` al compilar).
- Ya estaban instalados: Git 2.55.0, Git LFS 3.7.1, .NET SDK 10.0.402, Epic Games Launcher,
  Visual Studio Community 2026 (18.11) **sin** herramientas de C++.
- La carpeta original `Laberinto_Game` estaba vacía: no hay trabajo previo al 12/11 que declarar.

**13:20 · Versiones elegidas** (Claude consultó la documentación oficial de Epic)
- Unreal Engine **5.8.4**: última estable (hotfix del 20/10/2026).
- La documentación de Epic para UE 5.8 recomienda VS 2026 (18.0 o posterior), MSVC 14.50
  (mínimo 14.38), Windows SDK 10.0.26100 o posterior y .NET 10.
  - Decisión: se reutiliza VS 2026 y no se instala VS 2022.

**13:46 · Instalado**
- `git lfs install`: OK.
- Visual Studio Community 2026 18.11. Workloads agregados:
  - Desarrollo de juegos con C++.
  - Desarrollo para el escritorio con C++.
  - Desarrollo de escritorio de .NET.
  - .NET MAUI (lo pide la documentación de Epic).
- Componentes agregados: MSVC x64/x86 (**14.51.36231**, compilador 19.51), Windows 11 SDK **10.0.26100**,
  C++ profiling tools, AddressSanitizer, Visual Studio Tools for Unreal Engine, Unreal Blueprint debugger
  y Unreal Engine installer.
- JetBrains Rider **2026.2.3.1** (instalado con winget).
- Carpeta `C:\Dev\Labrynn` creada.

**Cómo ayudó la IA / tropiezos**
- Claude falló dos veces al lanzar el instalador de VS. Las dos veces terminó con código 87 (parámetro inválido):
  1. `Start-Process` partió la ruta `C:\Program Files\...` porque no tenía comillas.
  2. `setup.exe modify` no acepta `--wait`, que solo existe en el bootstrapper.

  Claude encontró las dos causas en `%TEMP%\dd_installer_*.log` y no probó cambios al azar.

**14:05 · Decisiones de Enzo**
- Repo en **GitHub** (`EnzoCalderon203/Labrynn`, privado) en lugar de Azure DevOps.
- Equipo: "Labrynn Team" (nombre provisional). Tema: libre.
- Cámara: primera persona con brazos propios; los demás jugadores se ven con el cuerpo completo y ragdoll.
- Enzo se fue unas horas y dejó a Claude trabajando en modo autónomo.

**14:00 · MSVC 14.50**
- Se agregó el componente `Microsoft.VisualStudio.Component.VC.14.50.18.0.x86.x64` (MSVC **14.50.35717**)
  al lado del 14.51.

**14:15 · Unreal Engine 5.8.4 instalado** (Enzo, desde el Launcher)
- En `C:\Program Files\Epic Games\UE_5.8` (29.9 GB). Build.version: 5.8.4, CL 58902052.
- No se instalaron los símbolos de depuración del editor (no hay `UnrealEditor.pdb`). Es opcional
  y quedó anotado en PENDIENTES_ENZO.md.

**14:25 · Fase 0 verificada ✅**
- `git --version` → 2.55.0.windows.3 · `git lfs version` → 3.7.1 · `cl.exe` 14.51 → 19.51.
- Claude creó y compiló un proyecto C++ vacío con `Build.bat BuildTestEditor Win64 Development`: **Result: Succeeded** (76 s).
  - UBT eligió solo: *"Using Visual Studio 14.50.35739 toolchain (…MSVC\14.50.35717) and Windows 10.0.26100.0 SDK"*.
  - Coincide con lo que recomienda Epic. Después se borró el proyecto de prueba.
- Git: repo local inicializado con identidad **local**, sin tocar la configuración global
  (`Enzo Calderón` / noreply de GitHub `232564110+EnzoCalderon203@…`).
  - Para el push se configuró `gh auth git-credential` solo en este repo.

**14:30 · Proyecto Labrynn creado** (Claude, en modo autónomo)
- Plantilla First Person de UE 5.8 (variante C++), **sin Starter Content**. Claude la copió a mano, igual que lo hace el editor:
  contenido `FirstPerson/` y los paquetes compartidos `Characters`, `Input` y `LevelPrototyping`.
  Se dejaron afuera las variantes Horror y Shooter (no se usan y pesan).
- **Decisión: clases propias con prefijo `Lab`** (`ALabCharacter`, `ALabPlayerController`, `ALabCameraManager`, `ALabGameMode`,
  `ALabGameState`) en lugar de las `TP_FirstPerson*` de la plantilla.
  - Los Blueprints de la plantilla siguen funcionando gracias a **CoreRedirects** en `DefaultEngine.ini`.
  - Se mantienen los mismos nombres de propiedades y componentes, así que conservan meshes, animaciones e input.
  - Motivo: código con nombres coherentes desde el día 1 sin tener que tocar ningún `.uasset` a mano.
  - Claude lo verificó con un script de Python del editor (`Scripts/setup_project.py`):
    `BP_FirstPersonCharacter` → `ALabCharacter`, cuerpo `SKM_Manny_Simple`+`ABP_Unarmed`, brazos `ABP_FP_Copy`.
- Mapa `L_Labrynn` vacío, creado por script. El laberinto se genera en runtime.
- Primera compilación: **Result: Succeeded** (44 s).

**14:30 · Laberinto procedural replicado**
- `FLabMazeLayout::Generate(seed)`: recursive backtracker iterativo con `FRandomStream`, sala central 3x3 con 2 puertas
  en lados al azar, ~8 % de paredes extra abiertas (loops), entrada al sur y salida al norte (cerrada hasta la fase de escape).
- `ALabMaze`: paredes, piso y techo con Instanced Static Mesh (un cubo escalado); hall de entrada; luces tenues guía.
  **No se replica**: cada máquina construye el suyo.
- `ALabGameState::MazeSeed` (`ReplicatedUsing`): el host elige la seed (o `?Seed=N` en la URL) y los clientes construyen en `OnRep`.
- `ALabGameMode::RestartPlayer`: spawnea a los jugadores en el hall de entrada, mirando hacia el laberinto (máximo 4).
- Linterna replicada (tecla F): `bFlashlightOn` con RepNotify y RPC al servidor, con predicción local.

**14:31 · Prueba de red automática ✅** (Claude levantó host y cliente sin ventana: `Scripts/nettest.ps1 -NoRender`)
- Cliente: `Welcomed by server` → host: `Join succeeded`.
- **Misma seed y mismo checksum en ambos**: `seed=2043047763 … checksum=9F15578E`.
- En el cliente, su personaje es `AutonomousProxy` y el del host es `SimulatedProxy` con `Body=SKM_Manny_Simple`
  (**ve al otro con el cuerpo completo**).

**14:35 · Prueba con capturas.** Claude agregó `-LabAutoShot=N` y `-LabAutoQuit`, que solo existen en builds de desarrollo:
el jugador mira al compañero, saca una captura y cierra el juego. Así puede ver el juego sin que nadie esté en la PC.
- Bug encontrado: en el cliente la pantalla salía negra. Log: *"Calling SetStaticMesh on …Walls but Mobility is Static"*.
  El cliente construye el laberinto después de BeginPlay y Unreal no deja asignar meshes a componentes Static.
  Arreglo: movilidad Movable. Sin luz estática, no tiene costo.
- Bug encontrado: con autoexposición el laberinto "a oscuras" se veía blanco. Arreglo: post-proceso con
  **exposición manual** (`ExposureBias`, +4 por defecto) y viñeta.
- Materiales de grilla de LevelPrototyping en paredes, piso y techo.
- El host caminó solo en una prueba: probable drift de un joystick Bluetooth conectado.

**14:42 · 🔴 BLOQUEO: Smart App Control**
- Windows empezó a bloquear `UnrealEditor-Labrynn.dll` (`GetLastError=4551`, eventos CodeIntegrity 3033/3077).
  Las primeras cuatro compilaciones sí cargaron. Una recompilación con otro hash también quedó bloqueada.
- Claude no lo cambió porque es configuración del sistema. Los pasos están en `PENDIENTES_ENZO.md` (punto 1).
- **Corrección posterior (Enzo):** Claude escribió que apagar Smart App Control era irreversible, pero eso es información vieja.
  Desde la actualización de abril de 2026 (**KB5083769**) se puede volver a activar desde Seguridad de Windows
  sin reinstalar. Claude lo confirmó en la web y lo corrigió en los docs. Lección: Claude no debe afirmar como hecho
  algo que puede haber cambiado después de su fecha de corte; tiene que verificarlo.

**14:50 · Smart App Control apagado (Enzo) → Hito 1 verificado ✅**
- Estado verificado: `VerifiedAndReputablePolicyState = 0`. La DLL carga.
- **Calibración** (Claude, mirando capturas y midiendo el brillo medio):
  - Exposición +4 EV con linterna 5000: el compañero a 2 m quedaba "quemado" en blanco.
  - **Exposición +4,5 EV con linterna 3000**: se ven los detalles del traje y el fondo del pasillo, y las zonas fuera del cono quedan oscuras.
    Brillo medio ~42/255. Quedó como valor por defecto.
- Bug del script de prueba (no del juego): host y cliente sacaban la captura en el mismo segundo y el host cerraba primero.
  El cliente se desconectaba, recargaba el mapa solo y sacaba otra captura que pisaba la buena.
  Arreglo: una sola captura por proceso, y el script cierra al cliente después de que termina el host.
- Resultado: `[AutoShot] … mira a BP_FirstPersonCharacter_C_1 (dist 192)` en **ambos** lados. En las capturas,
  cada uno ve al otro con el cuerpo completo, la linterna y la sombra. **Hito 1 cumplido.**

**15:00 · Online por internet: Steam con AppID 480 ✅ (verificado hasta donde se puede con una sola PC)**
- `ULabSessionSubsystem` (GameInstanceSubsystem): Crear, Buscar, Unirse, Unirse por IP e invitaciones del overlay de Steam.
  - Como el AppID 480 lo comparten miles de juegos, se filtra por la clave `LABRYNN_BUILD=labrynn-jam26-v1`.
  - Si Steam no está o falla (`-nosteam`, Steam cerrado, segunda instancia en la misma PC), el motor cae solo a
    **OnlineSubsystem NULL (LAN)** con `IpNetDriver`. El código no cambia.
- Menú en **Slate C++** (sin assets) en el nuevo mapa `L_Menu`, que es el `GameDefaultMap`. Luis puede reemplazarlo por un UMG.
- Pruebas automáticas (Claude):
  - **LAN (`-nosteam`):** flujo completo menú → Crear partida → otra instancia Busca → encuentra 1 → se une →
    mismo checksum del laberinto. ✅
  - **Steam real (host):** `Client API initialized`, `AuthStatus OK`, lobby creado (`Type: Lobby session`, `SteamP2P`),
    relays SDR OK (Buenos Aires a 30 ms) y `SteamSocketsNetDriver … listening on 7777`. ✅
  - **Steam (cliente) no se puede probar en una sola PC.** Una segunda instancia con la misma cuenta falla
    (*"Steam server and client running on the same machine"*) y pasa a LAN. Hace falta la prueba Enzo ↔ Luis.
- Para la build empaquetada: se agregó `bRelaunchInSteam=false`. Sin eso, la build Shipping se relanzaría a través de Steam
  como "Spacewar" (AppID 480). Al empaquetar hay que poner `steam_appid.txt` con `480` junto al exe (queda en PLAN.md, domingo).
- Riesgo: la entrega con AppID 480 obliga a los jurados a tener Steam abierto para jugar online. Hay fallback por LAN/IP.

**15:25 · Luz compartida ✅** (adelanto del viernes)
- **Decisión de diseño:** solo **1 de cada 2 jugadores** empieza con linterna (`FlashlightEveryN=2`). Con 2 jugadores
  hay una sola linterna, así que el grupo tiene que moverse junto. Todos tienen un **brillo personal** muy tenue (radio 2,2 m)
  para ver la silueta de los compañeros cercanos, pero no alcanza para explorar.
- Batería de **240 s** con la linterna prendida, controlada por el servidor y replicada como byte a todos. Por debajo del 25 %
  pierde intensidad y **parpadea** (cosmético y local); al llegar a 0 se apaga y no se puede prender. `AddBattery()` queda listo
  para los pickups del sábado.
- HUD mínimo (Canvas, provisional para Luis): `LINTERNA [F] NN%` con barra, o "SIN LINTERNA – quedate cerca del grupo".
- **Esc** sale de la partida y vuelve al menú (destruye la sesión).
- Verificado con 2 instancias y batería acelerada (`-LabBatterySeconds=70`): el host tiene linterna y el cliente no;
  el cliente ve la batería del host bajar (20 %) y el HUD del host se pone rojo al 14 %. ✅
- Aprendizaje: con Steam activo, el host escucha por **SteamSockets** y no por IP. Para "Unirse por IP", el host tiene que abrir
  el juego con `-nosteam`. Las pruebas locales de `nettest.ps1` ahora usan siempre `-nosteam`.

---

## 📋 RESUMEN PARA ENZO (14:45)

**Funciona y está verificado:**
- Toolchain: VS 2026, MSVC 14.50, SDK 26100 y UE 5.8.4. Un proyecto vacío compiló y se borró.
- Proyecto `C:\Dev\Labrynn` en C++. Compila. Subido a GitHub (`main`, 3 commits, contenido en LFS).
- **Hito 1 en red**: dos jugadores (listen server + cliente) en un laberinto procedural con seed replicada.
  Mismo layout verificado por checksum, y cada uno ve al otro con el cuerpo completo.
- `CLAUDE.md`, `PLAN.md` (aprobado por Claude según tu indicación), `PROMPT_LUIS.md`, `PENDIENTES_ENZO.md`.

**Falta:**
- Ver el Hito 1 en PIE y calibrar oscuridad y linterna. **Bloqueado por Smart App Control.**
- Online por internet (OSS Steam AppID 480). Está planeado para hoy de 17:00 a 20:00.
  - **Decisión:** Steam con AppID 480.
    - **Pro:** es gratis, no hay que configurar nada en un portal, y tiene NAT traversal, relay y amigos/invitaciones.
    - **Contra:** Steam tiene que estar abierto en las dos PCs, 480 es un AppID compartido (hay que filtrar sesiones por una clave propia)
      y no sirve para un producto final.
    - EOS se descartó porque requiere configurar un producto en el Developer Portal y un flujo de login: demasiado para una jam.
    - Fallback: IP directa (o una VPN tipo Tailscale) con `open <ip>`.

**Lo que tenés que hacer al volver** (detalle en `PENDIENTES_ENZO.md`):
1. Decidir si apagás **Smart App Control**. Sin eso no se puede abrir el proyecto.
2. Probar en PIE con 2 jugadores como Listen Server y decirme qué te parece la oscuridad.
3. Agregar a Luis al repo y pasarle `PROMPT_LUIS.md`.
4. Revisar la cuota de LFS en GitHub (135 MB usados).
