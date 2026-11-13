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

**15:03 · Online por internet: Steam con AppID 480 ✅ (verificado hasta donde se puede con una sola PC)**
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

**15:09 · Luz compartida ✅** (adelanto del viernes)
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

**15:16 · Reliquia, fase de escape y victoria ✅ (el core loop se puede jugar de principio a fin)**
- `ILabInteractable` (tecla **E**): el personaje elige el interactuable más cercano frente a la cámara. El cliente lo usa para el
  texto del HUD; el servidor lo vuelve a calcular y valida en `ServerInteract` (el cliente no elige el objetivo).
- `ALabRelic`: cono que brilla en celeste y **sirve de fuente de luz** para el grupo. Quien la carga camina al **55 %** de velocidad y
  **no puede usar la linterna**. Con E se la **pasa al compañero de enfrente** (≤ 2,6 m) o la **suelta** en el piso.
  `bAlwaysRelevant`. El portador se replica con RepNotify y la velocidad se aplica en servidor y cliente para que la
  predicción de movimiento no "rebote".
- `ALabGameState::Phase` (Explore → Escape → Won/Lost, replicada): al tomar la reliquia por primera vez se **abre la salida
  norte** (sala nueva con **baliza roja**). Si la reliquia llega a la sala de salida, hay **victoria**, y a los 10 s
  se hace `ServerTravel ?Restart` con un laberinto nuevo.
- HUD: `[E] Tomar/Pasar/Soltar la reliquia`, "¡LA SALIDA SE ABRIÓ!", "¡ESCAPARON CON LA RELIQUIA!".
- Prueba automática `-LabAutoRelic` (solo en desarrollo): en el log aparecen `Reliquia: None -> C_0` → `Fase -> Escape` →
  `Fase -> Won` → `ProcessServerTravel: ?Restart`, y el cliente **viaja con el host** y reconstruye el laberinto. ✅
- Con esto, el Hito 2 ("jugable de principio a fin") queda cumplido en su forma mínima. Faltan el rescate y las trampas.

> Nota (15:25): las horas de las entradas de luz, reliquia y rescate se corrigieron con las horas reales de los commits; Claude había anotado estimaciones.

**15:25 · Rescate ✅**
- `ELabLifeState` (Alive / Downed / Dead) replicado, con salud y `TakeDamage`. `KnockDown(impulso)` sirve para las trampas.
- **Caído:** ragdoll en todas las máquinas, con el empujón replicado. **La cámara del caído sale del cuerpo y lo muestra
  desparramado.** Se le cae la reliquia. Se desangra en 30 s y pasa a **Muerto** (solo lo revive el Corazón, el sábado).
- **Levantar:** E sobre el compañero caído y quedarse cerca 3 s, con barra replicada. Lo deja con 50 de salud.
- HUD: "ESTÁS EN EL PISO – ¡gritale a alguien que te levante! (Ns)", y marcadores **"! AYUDA Ns"** sobre los caídos que
  se ven a través de las paredes (para coordinarse).
- **Derrota** si nadie queda en pie, y reinicio de ronda.
- Prueba `-LabAutoDown`: se encontraron y corrigieron **3 bugs**, todos vistos en capturas:
  1. El cuerpo terminaba **a 2 km**: el mesh seguía adjunto a la cápsula, el servidor movía la cápsula a la pelvis
     y el ragdoll se movía con ella (bucle). Arreglo: desadjuntar el mesh al caer.
  2. En el cliente dueño, la cápsula (quieta) **sostenía el ragdoll de pie** desde adentro. Arreglo: la cápsula ignora
     `PhysicsBody` mientras está caído.
  3. El dueño **no veía su propio cuerpo**. En UE 5.8 los primitivos `WorldSpaceRepresentation` nunca se muestran al dueño,
     aunque `OwnerNoSee=false`. Arreglo: quitar la marca durante el ragdoll.
- Resultado: `C_1 cayó` → el host lo levanta → `C_1 se levantó (salud 50)`. En la captura del caído se ve
  boca abajo con los brazos abiertos. ✅

**15:31 · Trampas 1 y 2 ✅ (pinchos y foso)**
- `ALabTrapBase`: estado replicado Armed → **Warning** → Firing → Cooldown. El aviso previo es obligatorio en todas las trampas.
  Sonidos `WarnSound`/`FireSound`/`ResetSound` asignables por Luis en un Blueprint hijo.
- **Pinchos** (6 por laberinto, en pasillos rectos lejos de la entrada): placa + destello rojo + 0,6 s → los pinchos suben y
  **lanzan por el aire en ragdoll** (750 cm/s hacia arriba más empujón lateral al azar). Se rearman a los 3 s.
- **Foso** (3 por laberinto, **solo en callejones sin salida** para que un foso abierto nunca corte el camino a la reliquia):
  - El laberinto deja el hueco (piso por celda, fondo a 2,6 m) de forma determinista con la seed.
  - El piso falso tiembla 0,5 s y se abre para siempre. Quien cae queda caído en el fondo.
  - Un compañero lo levanta **desde el borde**: el alcance para cuerpos caídos se mide en horizontal (+1 m) y al levantarlo
    sale a la celda vecina.
- Prueba `-LabAutoTrap` (log del host): `Trampas: 3 fosos, 6 pinchos` → `LabPitTrap_0 activada por C_1` → `C_1 cayó` →
  `C_1 se levantó` → `LabSpikeTrap_0 activada por C_0` → `C_0 cayó`. ✅
- **Hito 2 cumplido** (estaba planeado para el viernes a las 22:00): luz compartida, reliquia, rescate, 2 trampas, escape,
  victoria/derrota y reinicio, todo por red.

**15:33 · Corazón del Laberinto y baterías ✅** (adelanto del sábado)
- `ALabHeart` en la sala central, al lado de la reliquia. Late (doble pulso rojo). **Un solo uso**: revive a todos los caídos y muertos
  (salud 100, y los saca de los fosos) y recarga todas las linternas. Al usarlo queda como una brasa apagada.
- `ALabBatteryPickup` ×6 en callejones sin salida que no tienen foso: brillo verde tenue para encontrarlas, **+50 %** de batería.
  Solo las agarra quien tiene linterna. Sonidos asignables por Luis.
- Prueba `-LabAutoHeart`: `C_1 cayó` → `Corazón usado por C_0: 1 revivido(s), linternas recargadas` → `C_1 se levantó (salud 100)`.
  En la captura se ve el Corazón latiendo en la sala central. ✅

**15:41 · Ensayo de empaquetado ✅ y trampa de gas ✅**
- **Empaquetado Shipping adelantado al jueves** para no descubrir problemas el domingo: `BUILD SUCCESSFUL` en ~2 min
  (los shaders ya estaban cacheados por las pruebas). 646 MB en total, de los cuales 214 MB son el `.pdb`.
  - El `.exe` abre el menú y muestra `servicio: STEAM` con el nombre de Steam del jugador.
  - **Aprendizaje:** la build Shipping necesita `steam_appid.txt` (con 480) al lado del exe.
- `Scripts/package.ps1`: empaqueta, agrega `steam_appid.txt`, quita los `.pdb` y arma `LabrynnTeam_Labrynn.zip`
  → **291 MB** (límite de la jam: 1 GB). ✅
- **Trampa de gas** (3 por laberinto, en pasillos sin pinchos): aviso de 1 s (siseo + luz verde que pulsa), después 6 s de nube.
  A quien está adentro se le **apaga la linterna y no puede prenderla**, y ve la pantalla verde.
  No hace daño: es una trampa "social" que deja al grupo a oscuras de golpe. La luz del gas es tenue a propósito.
  Prueba `-LabAutoGas`: `LabGasTrap_0 activada por C_0`, y en la captura la pantalla está verde y la linterna apagada. ✅

---

**16:41 · Stamina estilo Peak ✅ (pedido de Enzo)**
- **Una sola barra**, sin barra de vida aparte (`Health` se eliminó). Las **heridas bloquean la parte de arriba** de la barra: la
  stamina usable nunca supera Máx − heridas. Si las heridas llenan la barra, el personaje cae en ragdoll (el rescate sigue igual).
- **Shift = correr** (700 cm/s, gasta 9/s). Al llegar a 0 → **jadeo** 4 s: va a 230 cm/s, no puede saltar ni correr, suena la
  respiración (`BreathSound`, para Luis) y cada 0,8 s emite un **evento de ruido de AI Perception** (`UAISense_Hearing`, tag
  `Jadeo`). El **Minotauro no existe** (los enemigos quedaron fuera del alcance); el ruido queda listo por si se agrega.
- Cargar la reliquia gasta 2,5/s, y saltar gasta 8.
- Heridas: pinchos 35 (más el lanzamiento, sin caer si no llena la barra) y foso 25 (más atrapado).
  **Botiquín** (nuevo, 3 por laberinto) cura 50. **Corazón**: todos sin heridas y con la stamina llena.
  Al ser levantado por un compañero queda con 60 de heridas.
- **Sprint predicho**: `ULabCharacterMovement` + `FLabSavedMove` con `FLAG_Custom_0`. El pedido de sprint viaja en los saved moves,
  así que cliente y servidor calculan la misma velocidad y no hay rubber-banding.
- **Valores en Data Asset** `/Game/Labrynn/Core/DA_Stamina` (`ULabStaminaSettings`), creado por `Scripts/setup_project.py`.
  Se ajustan en el editor sin compilar.
- HUD provisional: barra de stamina (blanca = disponible, gris = recuperable, **roja con rayas = heridas**), batería e ícono
  de reliquia (rombo celeste).
- **Bugs encontrados en la prueba y corregidos:**
  1. 🔴 **El cliente ignoraba TODAS las correcciones de posición del servidor** (`could not resolve the new relative movement base`).
     El piso era Movable y Unreal lo trataba como base dinámica, pero `ALabMaze` no se replica (cada máquina tiene el suyo).
     Arreglo: piso y paredes **Static**, con el mesh asignado en el constructor. Después: 0 correcciones ignoradas y el cliente
     corre a ~595 cm/s visto desde el servidor (antes, ~60). Venía del Hito 1 y nunca se había visto porque ninguna prueba movía al cliente.
  2. El ícono de reliquia hacía **crashear** el juego: `FCanvasTriangleItem` con textura `nullptr` dispara un assert. Arreglo: `GWhiteTexture`.
  3. `-LabAutoRun=3,18` no andaba porque `FParse::Value` corta en la coma. Se pasó a `-LabAutoRunFrom/-LabAutoRunTo`.
- ⚠️ Hubo **2 crashes de GPU** (Virtual Shadow Maps / Device Removed, RTX 5070, driver 621.14) **antes** del cambio a Static,
  y **0 en 4 pruebas con render después**. No está confirmado que ese haya sido el motivo; si vuelve a pasar, la opción es
  desactivar las Virtual Shadow Maps (`r.Shadow.Virtual.Enable=0`).
- Prueba en LAN (`-LabAutoStamina -LabAutoRunFrom=3 -LabAutoRunTo=20`):
  - Host: stamina 100 → 11 % corriendo a ~685 → `se quedó sin aire` → 230 → se recupera.
  - Cliente: 40 de heridas (stamina topeada en 60 %) → corre, se agota y jadea → 70 más de heridas → `cayó`.
  - Botiquín: `curado: -50`. Reliquia: −2,5 %/s y velocidad 248. ✅
- **No implementado (no se pidió):** agacharse. `bCanCrouch` sigue desactivado y no tiene tecla.
**16:55 · 🐞 BUG: Shift no corría (encontrado por Enzo en PIE) → corregido ✅**
- **Causa:** era **código de prueba de Claude**. El `PlayerTick` de `-LabAutoRun` escribía `bWantsToSprint = false` **en cada frame**,
  aunque la prueba no estuviera activa, y pisaba el Shift del jugador. Las pruebas de Claude activaban el sprint por código y por eso
  **nunca pasaron por el input real**. Enzo tenía razón en sospecharlo.
- **Arreglo:** el `PlayerTick` de prueba solo toca el sprint dentro de su ventana de tiempo.
- **Input migrado a Enhanced Input:** `IA_Sprint` (Shift), `IA_Interact` (E), `IA_Lantern` (F) e `IA_Talk` (V) en
  `/Game/Labrynn/Core/Input` + `IMC_Labrynn` (prioridad 1), creados con `Scripts/create_input_assets.py`.
  `ALabPlayerController` agrega el IMC en cada máquina local (host y clientes). Se eliminaron los `BindKey` sueltos.
- **Nueva herramienta de prueba con input REAL:** `-LabKeys=t:Tecla:D|U|P` inyecta las teclas en `UGameViewportClient::InputKey`,
  el mismo camino que el teclado (viewport → PlayerController → Enhanced Input → acciones).
  - Shift+W: host y cliente → `sprintPedido=1` mientras se mantiene, `vel=700` y la stamina baja. ✅
  - F: host → luz 1→0→1. El cliente no tiene linterna y no cambia, como corresponde. ✅
  - E: host → toma la reliquia. **Cliente → E viaja por RPC** y el servidor registra `Reliquia: None -> C_1`. ✅
- **Regla nueva (en CLAUDE.md):** toda mecánica se prueba también por el camino del input real.
**18:09 · Punto 1: Farol medieval ✅ (cambio de diseño aprobado por Enzo)**
- La **linterna se eliminó**. Ahora la luz es **`ALabLantern`**: llama cálida omnidireccional con parpadeo (ruido Perlin; más inestable con
  poco aceite), radio de 6,5 m a propósito para que el grupo vaya junto, y sombras.
  - Aceite: 300 s encendido. Los **frascos de aceite** (`ALabOilFlask`, ×6) reemplazan a las baterías y suman +50 %.
- **Mesa en el hall** con `max(1, jugadores - 1)` faroles. `EnsureLanterns()` agrega los que falten cuando entra alguien.
- **E**: tomar, pasar al compañero de enfrente o dejar en el piso; en el piso **sigue iluminando**. No se puede llevar junto con la reliquia:
  al tomarla, el farol cae. Al caer en ragdoll también se suelta.
- **F**: apaga al instante. Reencender **tarda 3 s** (barra en el HUD).
- La trampa de gas pasó a ser una **corriente de aire**: apaga el farol que lleva quien está adentro y los apoyados en el piso de esa
  zona, y no deja reencender mientras dure. Velo frío en el HUD.
- **Corazón**: además, recarga y enciende todos los faroles.
- Calibración con captura: intensidad 2600 → **900**. A 2600 la imagen quedaba lavada (brillo medio 165/255); ahora es cálida, con la sombra
  de la mesa marcada. El farol se sostiene más abajo y a la derecha.
- **Prueba con input real (`-LabKeys`)**:
  - Host: E toma → F apaga → F reencendiendo 3 s → encendido → E lo deja en el piso.
  - **Cliente: E levanta el farol del piso por RPC** (`None -> C_1`).
  - Corriente de aire: `la corriente de aire apagó la llama`; F adentro no hace nada; F después reenciende en 3 s. ✅
  - 0 crashes de GPU.
- Pendiente opcional: antorchas tenues en las paredes.
**18:12 · Punto 2: Chat de voz por proximidad ✅ (lógica verificada; falta oír el audio real)**
- **V = push-to-talk** (`IA_Talk`): llama al `StartTalking`/`StopTalking` nativo de Unreal (OnlineVoice: Steam o NULL) y replica
  `bTalking`. Config: `bHasVoiceEnabled`, `[Voice] bEnabled` y `bRequiresPushToTalk`.
- **Voz 3D:** en cada máquina se crea un `UVOIPTalker` por compañero, pegado a su cápsula, con atenuación esférica
  (pleno hasta 1,5 m, se apaga a ~20 m) y **oclusión por paredes** (pasa-bajos a 900 Hz, volumen ×0,35).
- **La voz hace ruido:** mientras alguien habla, el servidor emite cada 0,4 s `ReportNoiseEvent` (tag `Voz`) con volumen = nivel medido
  de la voz (`GetVoiceLevel`; mínimo 0,3) y alcance proporcional. Para la voz del propio host no hay medición local y se usa 0,6.
- HUD: "HABLANDO [V]" propio y "( habla )" sobre compañeros cercanos.
- **Prueba con input real (`-LabKeys` V)** en host y cliente: `empieza a hablar` / `deja de hablar`, ruido de voz en el servidor para
  ambos (la V del cliente llega al servidor), y talkers 3D creados en servidor y cliente. ✅
- ⚠️ **Sin verificar:** que el audio del micrófono se transmita y se escuche bien. Las pruebas automáticas no tienen micrófono.
  Lo tienen que probar Enzo y Luis hablando, idealmente por Steam (está en PENDIENTES).
**18:19 · Punto 3: Minotauro ✅**
- `ALabMinotaur` (ACharacter, maniquí ×1,35 con ojos rojos) + `ALabMinotaurController` con **AI Perception: Hearing** (6000 cm,
  oye a todos). Aparece **al empezar la fase de escape**, en el callejón sin salida (sin foso) **más lejano** de los jugadores.
- **Sin NavMesh:** BFS sobre la grilla del laberinto, con paredes y fosos bloqueados, hacia la celda del **último ruido fuerte** (≥ 0,25; memoria de 8 s;
  prefiere el más fuerte y reciente). Si no oye nada, **deambula** a 300. Persiguiendo va a **600** (sprint 700, caminar 450). Al final de la
  persecución va directo al jugador más cercano dentro de 4,5 m.
- **Ruidos que oye:** voz (V), jadeo, **sprint** (nuevo: 0,5 cada 0,5 s, alcance 14 m) y **trampas al dispararse** (nuevo: 1,0, alcance 30 m).
- **Golpe:** 50 de heridas + ragdoll (se caen la reliquia y el farol) y **queda aturdido 2,5 s** (ventana para escapar o rescatar).
- **Telegrafía:** ojos rojos (más brillantes al perseguir), pasos y resoplidos 3D (sonidos para Luis: `FootstepSound`, `SnortSound`,
  `RoarSound`), viñeta roja que late en el HUD (más rápido si persigue) y textos "Algo enorme respira cerca... silencio" / "¡TE OYÓ! ¡CORRÉ!".
  Al abrirse la salida, el HUD avisa "Algo despertó en el laberinto... no hagan ruido".
- **Prueba con input real:** el host toma la reliquia con **E** (empieza el escape) y mantiene **V** (habla).
  - Spawn normal: a 38 m deambula siguiendo el BFS y **no** lo oye (la voz llega a 15 m). Correcto.
  - Con `-LabMinotaurSpawnDist=1200` (solo para probar): `oyó 'Voz' (0.60)` → persigue a 600 por un camino de 32 celdas → `¡atrapó a C_0!` → `cayó`. ✅
  - Con render: 0 crashes de GPU.
- ⚠️ **Balance a mirar en el playtest:** quien carga la reliquia va a 248 cm/s, así que si el Minotauro lo oye, lo alcanza. Es la tensión buscada
  (protegerlo, distraer al Minotauro o soltar la reliquia y correr), pero puede resultar demasiado. Todo se ajusta en el BP o el DA sin compilar.
**18:46 · Diseño v2, punto 1: Visibilidad mínima sin farol ✅**
- **`DA_Visuals`** (`ULabVisualSettings`, creado con `setup_project.py`): todos los valores se ajustan sin compilar.
- **Rayos de luna:** 9 grietas en el techo (emisivo celeste) con un spot frío y tenue hacia abajo, elegidas con la seed (iguales en todas las máquinas),
  más **niebla volumétrica** leve para que el haz se vea.
- **Musgo:** tiras emisivas verdes al pie de las paredes en 110 celdas (`M_Glow`, material creado con `Scripts/create_materials.py`;
  provisorio para reemplazar por el kit medieval).
- **Adaptación ocular** (solo cámara local): lejos de un farol encendido, la exposición sube **despacio** (de 4,0 a 6,8 EV en 10 s); cerca de la luz se pierde rápido.
  **Encandilamiento**: al mirar un farol de frente y a menos de 2,3 m, salto de exposición y bloom durante 1,2 s, más fuerte cuanto más adaptados estaban los ojos.
- **Silueta de compañeros:** brillo personal tenue (14, radio 3,2 m) desde el DA.
- Prueba en LAN (logs, `-LabEyeLog`, **W real**): al alejarse del farol la adaptación sube 0 → 1 en 10 s (exposición 4,0 → 5,4 con el valor inicial).
  Frente al farol: `encandilado (fuerza 1.00)` → la exposición salta a 6,8 y vuelve a 4,0. El cliente construye el mismo ambiente
  (`musgo en 40 celdas, 9 haces`). ✅
- Calibración (dos capturas, imprescindibles para juzgar la oscuridad): con +1,4 EV de adaptación el pasillo quedaba **negro**. Con **+2,8 EV**,
  110 celdas de musgo y brillo personal de 3,2 m se distinguen la pared cercana y la forma del pasillo, pero no los detalles. Ese quedó como valor por defecto.
**18:53 · Diseño v2, punto 2: El Minotauro le teme al fuego ✅**
- **`DA_Minotaur`** (`ULabMinotaurSettings`): velocidades, ataque, oído y fuego. Ya no hay valores sueltos en la clase.
- Si un **farol encendido** está a < 4 m y **por delante** (entre él y su objetivo: coseno > 0,2), el Minotauro **frena, retrocede 3 s** y después
  hace **BFS evitando las celdas con faroles encendidos**. Si no hay otro camino, igual va y el fuego lo vuelve a frenar.
- Mientras lo repele, ese farol gasta **aceite ×4**. HUD del portador: **"¡Lo estás conteniendo!"**. Sonido `FireFearSound` para Luis.
- Herramienta de prueba: `-LabKeysHost=` / `-LabKeysClient=` (guiones de teclas por rol). La prueba anterior se arruinó porque los dos procesos
  compartían el guion y la segunda E soltaba el farol y la reliquia.
- **Prueba con input real:** el host toma el farol con **E**, el cliente toma la reliquia con **E** (empieza el escape) y el host habla con **V**.
  Resultado en ~70 s: `oyó 'Voz'` → `¡el fuego de C_0 lo frena! retrocede 3 s` → `busca otro camino` → … **6 frenadas y 0 atrapados**.
  Aceite del farol al **59 %** (sin el ×4 habría quedado en ~77 %). ✅
**18:58 · Diseño v2, punto 3: Escondites ✅**
- `ALabHidingSpot` replicado: **barril, paja o nicho** (formas provisorias; un BP hijo puede poner las del kit medieval). Se colocan en
  **callejones sin salida**, contra la pared del fondo y mirando a la abertura (`DA_Gameplay.HidingSpotCount = 8`).
- **E** para entrar o salir; una persona por escondite y no se puede entrar con la reliquia. Escondido no te movés, tu cuerpo no se ve y no tenés colisión.
  Al caer en ragdoll, salís.
- **El Minotauro no detecta ni ataca** a quien está escondido, **salvo que tenga un farol encendido** (`IsDetectableByMinotaur`).
- **La voz escondida hace menos ruido** (×0,35).
- Nuevo **`DA_Gameplay`** (`ULabGameplaySettings`): escondites, lanzables, obstáculos y bordes.
- **Prueba con input real:** el cliente se esconde con **E** y habla con **V** → ruido de voz de **0,11 (2,6 m)**, por debajo del umbral del Minotauro.
  El host toma y suelta la reliquia con **E**, se para al lado del escondite y habla con **V**. El Minotauro llega y `¡atrapó a C_0!` (el host);
  **al cliente escondido nunca lo atacó**. ✅
- Nota de diseño: un jugador caído que sigue apretando V sigue haciendo ruido. Queda así a propósito (pedir ayuda tiene riesgo).
**19:06 · Diseño v2, punto 4: Obstáculos de movimiento + agacharse (C) ✅**
- **Escombros y vigas** (`DA_Gameplay.RubbleCount = 8`, altura 70 cm) cruzando pasillos rectos: **solo se pasan saltando** (el salto llega a ~90 cm y cuesta 8 de stamina).
  El Minotauro los pasa por paso alto (`MaxStepHeight 85`).
- **Túneles bajos** (`LowTunnelCount = 5`, techo a 125 cm): **solo se pasan agachado**. **Al Minotauro le bloquean el paso** (su BFS los evita),
  así que funcionan como refugio.
- **Agacharse = C mantenida** (`IA_Crouch` + `IMC_Labrynn`), con el crouch replicado del CharacterMovement. Cápsula agachada de 110 cm a 220 cm/s
  (más lento con la reliquia). Todo en `DA_Gameplay`.
- Obstáculos deterministas en el laberinto (igual en todas las máquinas). Las trampas ya no caen en celdas con obstáculos.
- **Validación al generar:** escombro 70 < salto 90 y túnel 125 entre agachado 110 y parado 192 → `OK`. Los obstáculos nunca cierran celdas de la grilla,
  así que el camino entrada → centro → salida siempre se puede recorrer con salto o agachado. Si alguien cambia el DA a medidas imposibles, sale un error en el log.
- **Prueba con input real:**
  - Host frente a un escombro: con W queda bloqueado (`vel=0` durante 3 s) → **Espacio** → stamina 100 → 92 y pasa a la celda siguiente.
  - Host frente a un túnel: parado queda bloqueado → **C** → `z=57 agachado=1` → cruza (8,9) → (8,10) → (8,11).
  - **Cliente: lo mismo con C real**, con **0 correcciones de red ignoradas**. ✅
- 🐞 **Aprendizaje:** el script de input no guardaba (`Cannot remove ... as it is read only`). Los `.uasset` son `lockable` en `.gitattributes`,
  así que Git LFS los deja en **solo lectura** hasta bloquearlos. Solución: `git lfs lock` de los assets de input, que son de Enzo. Quedó en CLAUDE.md.
**19:24 · Diseño v2, punto 5: Pie de banco + dar la mano ✅**
- **Bordes de 150 cm** (más que salto + paso, ~135): la generación busca en los caminos entrada → centro y centro → salida una **celda puente**
  (una que, si se bloquea, deja sin camino; o sea, sin rodeo posible) y pone el borde ahí. **El último siempre necesita una mano.**
  El Minotauro no trepa bordes. ⚠️ **Consecuencia: el juego necesita 2 o más jugadores** (solo no se pueden pasar los bordes).
- **Pie de banco:** E sobre un compañero **agachado** → impulso de 620 hacia arriba y 300 hacia adelante (atraviesa al compañero y sin frenado en el aire
  durante 0,8 s). El dueño lo predice con `ClientLaunch`.
- **Dar la mano:** el de arriba, en el filo y mirando hacia abajo, **mantiene E** → brazo extendido (cilindro provisorio, visible para todos; **no lo
  verifiqué con captura**). Mientras sostiene, no se mueve ni usa el farol. El de abajo **salta + E** (alcance de 60 cm: **sin saltar no llega**) y
  **sube en 1,5 s**, con un tirón interpolado igual en servidor y cliente a partir de datos replicados.
- **Tirar gasta stamina** al que ayuda (30/s, ×1,6 si el de abajo lleva la reliquia). Si se queda sin stamina o suelta E, **el de abajo cae**.
  Sonido `EffortSound` para Luis. Valores en `DA_Gameplay`.
- **Techo del laberinto subido de 350 a 400 cm**: con 350, al impulsarse se chocaba la cabeza y no llegaba arriba.
- **Iteración guiada por logs** (velocidad a +0,1 s y +0,3 s después del impulso):
  1. Con 350 de techo, la cabeza chocaba.
  2. Sin avance horizontal: el frenado en el aire de la plantilla (1500) lo frenaba.
  3. Chocaba contra el compañero agachado.
  4. Se chocaba con la **cara del borde** (subía demasiado lento para lo que avanzaba).

  Solución: impulso más vertical, sin frenado y atravesando al compañero.
- **Prueba con input real:**
  - Cliente **C** + host **E** → `pie de banco` → host arriba (z=248).
  - Host **mantiene E** → `estira la mano`. Cliente **Espacio + E** → `se agarró` → `subió` (el cliente queda arriba, z=248).
  - **Solo E sin saltar → no se agarra.**
  - Con el host herido (15 de stamina): `no aguanta más y suelta` → `se soltó y cae`. ✅
**19:28 · Diseño v2, punto 6: Cargar a un compañero caído ✅**
- Sobre un caído: **tocar E = levantarlo** (como antes); **mantener E 0,6 s = cargarlo al hombro**. Para eso se mide en el cliente cuánto se mantiene E.
  **E otra vez = soltarlo** (vuelve al ragdoll, al lado).
- El cargado: su cápsula sin colisión queda pegada al que lo carga y el cuerpo va cruzado sobre el hombro (**pose provisoria, no verificada con captura**).
  **Sigue viendo desde su cuerpo**, **sigue hablando** (antes un caído no podía hablar; corregido) y se sigue desangrando.
  Si un compañero lo levanta o se usa el Corazón, primero lo bajan.
- El que carga: va a ×0,5 de velocidad, gasta 6 de stamina por segundo y **suelta el farol** al empezar. No puede tomar la reliquia, el farol ni esconderse.
  Si se queda sin aire, lo suelta; **si cae él, caen los dos**. Valores en `DA_Gameplay`.
- Mientras carga a alguien, no cuenta como "levantándolo" (antes, mantener E también habría arrancado el levantar).
- **Prueba con input real:** el cliente cae; el host **mantiene E** → `carga al hombro` → **W** → velocidad 225 y stamina 92 → 56 %
  → **E** → `suelta`. El cliente caído **habla con V** y su voz hace ruido (0,3). ✅
- No probado aparte (lógica directa): "si cae él, caen los dos" y "llevarlo al Corazón".
**19:35 · Diseño v2, punto 7: Piedras y botellas para distraer ✅**
- `ALabThrowable` replicado, **12 por laberinto** (mitad y mitad). **E** para juntar, **máximo 3** en el inventario (replicado solo al dueño).
- **Click izquierdo mantenido** (`IA_Throw`) = apuntar: **arco de trayectoria** con `PredictProjectilePath`, dibujado en el HUD (anda en Shipping).
  **Soltar = lanzar**: el servidor valida el inventario y acota el origen y la velocidad.
- **Botella:** se rompe al primer golpe (ruido 1,0, alcance 32 m). **Piedra:** rebota con un ruido proporcional a la velocidad del golpe (0,6 → 0,24)
  y, cuando para, **se puede volver a juntar**.
- **El Minotauro oye los golpes** (tags `Botella` / `Piedra`) y **los objetos lanzados activan trampas** al pasar por su zona.
  Sonidos `ImpactSound` y `ShatterSound` para Luis. Valores en `DA_Gameplay`.
- **Prueba con input real:**
  - Host **E** junta una piedra → **click izquierdo** → `Trampa LabSpikeTrap_0 activada por LabThrowable_12` (a distancia) más los rebotes.
  - Con el escape activo: `Minotauro: oyó 'Piedra' (0.60)`.
  - **Cliente: E + click** → `C_1 juntó` / `lanzó` (por RPC).
  - Botella: un solo golpe (ruido 1,0) y se rompe. ✅
- Nota de prueba: si cargás la reliquia, la E apunta primero a la reliquia (la tenés en los brazos) y no al objeto del piso.
**19:37 · Diseño v2, punto 8: Voz según tu estado ✅ (lógica; falta escucharlo con micrófono)**
- `ULabVoipTalker` (subclase de `UVOIPTalker`) guarda el `UAudioComponent` de la voz al empezar a hablar. En cada máquina, el personaje
  procesa la voz de los demás según su estado replicado: **volumen, pasa-bajos y pitch**, sin plugins.
  - **Herido** (heridas ≥ 50 %): temblor de volumen y cortes breves (entrecortada).
  - **Corriendo o sin aire:** modulación de respiración y pitch levemente más alto (el loop de respiración del jadeo ya se mezcla).
  - **Caído:** ahogada (pasa-bajos a 700 Hz, ×0,7).
  - **Escondido:** baja (pasa-bajos a 2500 Hz, ×0,45).
  - **Cargado al hombro:** el volumen "rebota" con los pasos.
  - Valores en `DA_Gameplay` (categoría Voz).
- **Prueba con input real** (W+Shift del cliente, E mantenida del host, caída y heridas preparadas):
  - Servidor sobre la voz del cliente: `normal → agitada → normal → ahogada (700 Hz) → rebota (cargado) → ahogada`.
  - Cliente sobre la voz del host herido: `entrecortada`. ✅
- ⚠️ **No se puede escuchar en pruebas automáticas** (no hay micrófono). Queda en PENDIENTES para probar con Luis.
**19:46 · Diseño v2, punto 9: Premios graciosos + ping ✅**
- `ALabPlayerState` replicado con estadísticas de la ronda (solo las escribe el servidor): caídas, heridas, trampas pisadas, compañeros levantados,
  metros cargando a un caído, voz que hizo ruido, cosas lanzadas, segundos escondido y pings.
- Al ganar o perder, `ALabGameState::ComputeAwards` le da cada premio al que más tiene: *Alfombra del laberinto*, *Imán de trampas*, *Colador humano*,
  *Ángel de la guarda*, *Mula de carga*, *Megáfono*, *Lanzador olímpico*, *Rey del barril* y *Señalador compulsivo*.
  El que no ganó nada se lleva *Turista*. Se dibujan en el HUD debajo del mensaje final.
- **Ping:** **rueda hacia arriba** (`IA_Ping`) marca lo que mirás, hasta 30 m, para todos y durante 5 s, con un rombo, el nombre y la distancia.
  Un ping por jugador, cooldown de 1 s y validación en el servidor. Los caídos pueden marcar ("¡estoy acá!"). Valores en `DA_Gameplay` (categoría Ping).
- **Prueba con input real** (rueda en host y cliente; caída del cliente y heridas del host preparadas para forzar la derrota):
  - `Ping: ... marcó` en los dos. El segundo ping del cliente, a 0,4 s del primero, se descartó por el cooldown.
  - `Fase -> Lost` → 4 premios en el servidor y `Premios recibidos: 4` en el cliente. ✅
- Corregido en la prueba: las heridas contaban el golpe bruto (500) y no lo que realmente se sumó (100).
- Build **Shipping** de verificación OK: zip de 291 MB.
**20:08 · Pedido de Enzo jugando: "sin farol no se ve nada" y "no se agacha"**
- **Visión mínima:** luz suave **solo en la máquina de cada jugador** (point light sin unidades, caída suave, sin sombras, pegada a la cámara).
  Da un piso de visibilidad, ~11 m que se apagan a negro, sin iluminar la pantalla de los demás ni delatar a nadie. `DA_Visuals` → *Visión mínima*.
  Primera calibración: con 1,5 la pantalla quedó blanca (la exposición alta la multiplica). **0,04** = pasillo gris tenue, fondo negro (verificado con captura).
- **Agacharse "no hacía nada":** la cápsula sí se achicaba (los túneles se pasaban), pero el motor sube el modelo para compensar,
  y la cámara está pegada a la cabeza del modelo, así que la vista no bajaba.
  Ahora, con `TickCrouchVisual`, tus brazos y tu cámara bajan ~70 cm en forma suave y los demás ven tu cuerpo achicado hacia los pies
  (provisorio hasta tener animación). Verificado con C real y captura: cámara a la altura del túnel.
**13/11 13:43 · Repo público** (pedido de Enzo). Antes se revisó que no hubiera claves ni datos personales: solo aparece el email noreply de GitHub y el token del servidor de archivos de Android de la plantilla, que no se usa. Se agregó `README.md`.

**13/11 14:37 · Audio completo (pedido de Enzo: "hoy no hay ningún sonido y el audio es criterio de los jueces", sin Luis)**
- **Inventario:** había 15 propiedades de sonido vacías (trampas ×3, farol ×2, aceite, botiquín, Corazón, lanzables ×2, Minotauro ×4, `EffortSound`)
  más `BreathSound` en `DA_Stamina`. Además faltaban pasos, salto, aterrizaje, caída, revivir, escondites, cargar, ping, UI, ambiente, música y stingers.
  En total son **44 eventos** (tabla en `AUDIO.md`).
- **Fuentes:**
  - Packs **CC0 de Kenney** (*Impact Sounds*, *RPG Audio*), con licencia verificada en kenney.nl. kenney.nl y OpenGameArt **bloquean la descarga automática (403)**;
    no se intentó saltear el bloqueo y se usaron copias completas publicadas en GitHub (CC0 permite redistribuir).
  - **Síntesis propia** con numpy/scipy (`Scripts/audio/build_audio.py`, determinista): ambiente, fuego, viento, goteo, latido, Minotauro
    (sierra con formantes, gruñido y distorsión; pasos = impactos de Kenney bajados de tono con sub-grave), stingers y UI.
  - **Música:** no se buscó un tema CC0 suelto. Se **sintetizaron 3 capas** (calma, tensión, persecución) con el mismo largo (48 s), tempo (90 bpm) y tonalidad (Re menor),
    porque el crossfade adaptativo necesita capas sincronizadas y los temas CC0 no vienen separados en capas.
- **Control sin oído:** `check_audio.py` revisa formato, pico, RMS, clipping, DC y empalme de loops; además se miraron espectrogramas.
  Así se encontraron y arreglaron clics al final de golpes (envolvente cortada al 4 %), un crepitar del farol que sonaría a estática (ruido de fondo demasiado alto),
  un rugido con demasiado soplido, un loop de respiración corto (`wrap` sin relleno), DC en un rugido y clipping después del pasa-altos.
- **Integración** (`Scripts/setup_audio.py`):
  - **Sound Classes** Master > SFX > UI, Voice, Music y Ambience; **Submixes**; `MIX_Settings` con **5 sliders de volumen** en el menú (se guardan en GameUserSettings.ini).
  - **Atenuación igual al ruido que oye la IA** (pasos = sprint 14 m, piedra 18 m, botella y trampas 32 m, Minotauro 45 m), con caída de agudos y oclusión.
  - **Concurrency** por grupo (pasos 8, impactos 6, Minotauro 4).
  - **Variación** al azar de variante, pitch y volumen en C++. Reemplaza al nodo Random de los Sound Cues, que no se puede construir desde Python.
- **Música adaptativa** (`ULabAudioDirector`): calma en exploración → tensión con el Minotauro a menos de 35 m → persecución si persigue a menos de 22 m.
  El crossfade entra rápido y sale lento. **Ducking** a ×0,35 cuando alguien habla.
  En la prueba la música oscilaba tensión ↔ persecución en el borde de 10 m; se agregó **histéresis del 35 %**.
- **Prueba en LAN con `-LabAudioLog`** (input real + preparadores). Los eventos sonaron **en host y cliente** en estos casos:
  - pasos, salto y aterrizaje (también del personaje remoto);
  - farol;
  - piedra → aviso, pinchos, rebotes y rearme;
  - reliquia → stinger de salida → música a tensión;
  - **botella → el Minotauro la oye → rugido → persecución → ataque → caída**;
  - escondite, cargar, soltar, herida, revivir, respiración, gas, foso, botiquín, Corazón, ping, ducking y stingers de victoria y derrota.
  
  Con audio real (sin `-nosound`) se crean las 3 capas de música en el laberinto y en el menú, sin advertencias.
- **No verificado:** cómo suena (Claude no puede escuchar). Lista de "qué escuchar" en **PENDIENTES 2g**.
  Tampoco el clic de los botones del menú: las pruebas no pueden hacer clic en Slate.
- Tamaño: 88 WAV fuente (48,7 MB) más 112 assets (22 MB) en LFS.
- **Build Shipping OK:** zip de **295 MB** (antes 291). Se verificó en el índice del paquete (`.utoc`) que están `DA_Audio`, las 3 capas de música, la mezcla, las atenuaciones y la concurrencia: el cook ahora incluye siempre `/Game/Labrynn/Core` y `/Game/Labrynn/Audio` (`DirectoriesToAlwaysCook`), porque se cargan por ruta.
**13/11 15:25 · Bug de visibilidad: se veía mejor SIN farol que con farol (prioridad alta)**
- **Causas:**
  1. La adaptación ocular subía la exposición **+2,8 EV (×7)** a oscuras y se perdía cerca de un farol. Con farol, todo lo que quedaba fuera de su radio se veía hasta 7 veces más oscuro.
  2. La luz del farol tenía radio de atenuación de **6,5 m**: más allá, negro absoluto (corte brusco).
- **Arreglo** (todo en `DA_Visuals`):
  - **Exposición casi fija:** base 6,3 EV, y la adaptación suma como máximo **+0,2 EV (≈ +15 %)** (antes era +2,8).
  - **Encandilamiento breve y suave:** +0,3 EV, bloom 1,5 y 0,5 s (antes +1,6 EV, bloom 4 y 1,2 s).
  - **Farol** = luz principal cálida con **caída física 1/d², radio 18 m**, más una luz de **relleno** tenue (sin unidades, exponente 1,6, radio 22 m, con sombras para que no atraviese paredes).
    La intensidad, los radios y el exponente están en `DA_Visuals` (categoría *Farol*).
  - La penumbra base (luna, musgo y visión mínima local) queda igual con o sin farol.
- **Crash de GPU (regla de Enzo):** con la luz de relleno, el caso "farol en el piso" daba **"GPU Crashed or D3D Device Removed"** a los ~30 s, reproducible solo.
  Con `r.Shadow.Virtual.Enable 0` no crashea → **virtual shadow maps desactivadas** en `DefaultEngine.ini` (vuelve a sombras clásicas).
- **Verificación** (`Scripts/eye_test.ps1` + `Scripts/eye_bands.py`):
  - Mismo punto y misma dirección: inicio del pasillo recto más largo (31 m), mirando a lo largo, cámara a 1,62 m, inclinada −20°, FOV 90, HUD oculto y farol sin parpadeo.
  - Captura a los 65 s en los 3 casos.
  - Brillo medio (luma sRGB 0–255) del piso en una franja central, con la distancia de cada fila calculada por geometría:

| Franja | Sin farol (60 s a oscuras) | Farol en la mano | Farol en el piso a 3 m | Con farol ≥ sin farol |
|---|---|---|---|---|
| 0–3 m | 50.2 | 131.5 | 144.2 | ✅ |
| 3–8 m | 22.8 | 64.6 | 117.4 | ✅ |
| 8–15 m | 0.3 | 15.2 | 14.3 | ✅ |

  Exposición en la captura: sin farol 6,50 EV (adaptado); con farol 6,30 EV.
  Visualmente: sin farol se ve una penumbra fría con el piso y algo de pared cerca; con farol, luz cálida fuerte que se desvanece de forma gradual por el pasillo, sin corte.
**13/11 15:46 · Preparación de la prueba por Steam (Enzo ↔ Luis)**
- **Zips:**
  - **Entrega (Shipping):** `Packaged\LabrynnTeam_Labrynn.zip`, **295 MB**, con `steam_appid.txt` = 480 y los lanzadores.
  - **Prueba (Development):** `Packaged\LabrynnTeam_Labrynn_PRUEBA_DEV.zip`, **360 MB**, además con `PRUEBA_STEAM.md`.
- **Por qué dos builds:** Shipping no escribe log. Activar `bUseLoggingInShipping` exige un build environment único, que el motor instalado del Launcher no permite.
  Development escribe `Labrynn\Saved\Logs\Labrynn.log`.
- **Logs nuevos:**
  - `Red: entró/salió` (nombre, ID de Steam o IP, cantidad de jugadores);
  - cada 10 s, `Red[host→cliente|cliente→host]`: ping, pérdida in/out, KB/s y driver (SteamSockets o IpNetDriver);
  - búsqueda: sesiones devueltas por Steam contra las de Labrynn, y la URL de unión;
  - `Error de red` y `Error de viaje`, también en el menú;
  - verbosidad de `LogOnlineSession`, `LogOnlineVoice`, Steam y sockets en `[Core.Log]`.

  Probado en LAN: `ping=4 ms pérdida 0 %`.
- **Filtro de búsqueda:** ya existía la clave propia `LABRYNN_BUILD=labrynn-jam26-v1`, en la consulta a Steam y como segundo filtro del lado del cliente.
  Ahora se loguea cuántas sesiones de otros juegos del AppID 480 se descartan.
- **Lanzadores:** `Labrynn (Steam).bat` y `Labrynn (IP directa).bat` (`-nosteam`, plan B con Tailscale). Son `.bat` y no `.lnk`: un acceso directo guarda la ruta absoluta y se rompe al descomprimir en otra PC.
- Errores corregidos al empaquetar:
  - PowerShell no expandía `-clientconfig=$Config` sin comillas;
  - en Shipping, `LogNetStats` quedaba fuera del `#if !UE_BUILD_SHIPPING`.
**13/11 17:32 · Primera conexión por Steam Enzo ↔ Luis ✅ (después de diagnosticar con los logs)**
- **Intento 1 ("0 encontradas"):** Luis tenía **dos ventanas** abiertas. La segunda no pudo iniciar Steam (`InitGameServer failed`, la primera ocupaba el puerto)
  y cayó a `NULL`: todas sus búsquedas fueron en red local. Además, los dos crearon partida, en momentos distintos.
- **Intento 2:** el host salía con **Esc** a los pocos segundos (partidas de 1 s, 5 s y 25 s; `Browse L_Menu` sin error). Probablemente cerraba el overlay de Steam con Esc.
  Al salir se borra el lobby: por eso no aparecía en la búsqueda, y el overlay solo ofrecía "ver información" y no "unirse".
- Los lobbies estaban bien publicados: públicos, BUILDID 55116800, `LABRYNN_BUILD_s=labrynn-jam26-v1`, P2P, regiones cercanas (eze / gru).
- **Intento 3:** host sin tocar Esc → **funciona**.
- Pendiente propuesto: Esc doble para salir y aviso en rojo en el menú si no hay Steam.
## 📋 RESUMEN PARA ENZO (15:41) (reemplaza al de las 14:45)

> Decisión de comunicación del online (14:45, sigue vigente): **Steam con AppID 480**.
> Pro: gratis, sin configurar ningún portal, con NAT traversal, relay e invitaciones de amigos.
> Contra: Steam tiene que estar abierto, el AppID es compartido (filtramos con una clave propia) y no sirve para un producto final.
> EOS se descartó porque requiere un producto en el Developer Portal y un flujo de login. Fallback: IP directa con `-nosteam`.

**Funciona y está verificado (pruebas automáticas con 2 instancias y capturas):**
- **Hitos 1 y 2 completos.** El jueves a las 15:41 ya se juega de principio a fin. El Hito 2 estaba planeado para el viernes a las 22:00.
- Menú (Slate) → Crear / Buscar / Unirse / IP. Steam real en el host (lobby + relays) y LAN entre 2 instancias.
- Laberinto con seed replicada, oscuridad calibrada, 1 linterna cada 2 jugadores con batería, brillo personal.
- Reliquia (lenta, sin linterna, se pasa o se suelta) → se abre la salida → victoria → nueva ronda.
- Caída con ragdoll (el caído ve su cuerpo desparramado), rescate con E, desangrado, muerte, derrota.
- Trampas: **pinchos** (lanzan por el aire), **fosos** (rescate desde el borde) y **gas** (apaga linternas).
- **Corazón del Laberinto** (revive a todos y recarga las luces, 1 uso) y **baterías**.
- **Build Shipping empaquetada**: zip de 291 MB con `Scripts/package.ps1`. Arranca con Steam.

**Falta (según el plan):**
- 🔴 **Prueba online real Enzo ↔ Luis por Steam.** Es lo más importante que queda sin verificar.
- Tiza para marcar paredes y botiquín (sábado). Opcional: dardos.
- **Audio** (Luis): todas las trampas, el Corazón y las baterías tienen propiedades de sonido listas para asignar en un BP hijo.
  Faltan pasos, goteo, gritos y la baliza de la salida.
- HUD y menú definitivos (Luis); ajustes de brillo y volumen.
- Playtest con 3–4 personas y balance: tamaño del laberinto, batería de 240 s, cantidad de trampas.
- Video, página de itch y `CREDITS.md` (contenido de la plantilla de Epic: maniquí, LevelPrototyping, Input).

**Lo que tenés que hacer al volver** (detalle en `PENDIENTES_ENZO.md`):
1. Jugarlo vos: abrí `Labrynn.uproject`, Play con **2 jugadores** como **Listen Server**. Teclas: WASD, mouse, F linterna, E interactuar, Esc menú.
2. Agregar a Luis al repo y hacer la **prueba por Steam** entre las dos PCs (punto 2b de pendientes).
3. En pantalla quedó abierto un aviso del **Firewall de Windows sobre UnrealPak**. No lo toqué. Para el juego no hace falta;
   podés cancelarlo.
4. Revisar la cuota de LFS en GitHub.