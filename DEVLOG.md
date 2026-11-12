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