# CLAUDE.md — Labrynn (2026 Epic AI MegaJam)

Contexto permanente para Claude Code. Leelo completo antes de tocar el proyecto.

## Deadline (no negociable)

- **Entrega: domingo 15/11/2026, 22:59 hora de Paraguay** (5:59 PM PST). Una entrega tardía queda descalificada.
- Objetivo interno: **subir la entrega a las 21:00 del domingo** (2 h de margen).
- **Feature freeze: domingo 15:00.** Build empaquetada: domingo 17:00.
- Si algo amenaza el deadline, avisá y proponé qué recortar. Es mejor un juego chico y divertido que uno grande y roto.

## Entrega

- Build **empaquetada** para Windows en `.zip` de **menos de 1 GB**: `LabrynnTeam_Labrynn.zip`
  ("Labrynn Team" es un nombre provisional).
- Se sube a itch.io junto con un video de gameplay de 30 a 60 s y la lista de herramientas usadas (incluida la IA).
- No se usa trabajo previo al 12/11. Todo asset de terceros se declara en `CREDITS.md`, con su licencia.

## El juego

**Labrynn**: laberinto subterráneo procedural, a oscuras, cooperativo online para 2 a 4 jugadores.
**Core loop:** entrar → llegar al centro → tomar la reliquia → escapar por la salida.

- **Pilar número uno: la diversión entre amigos, al estilo de Peak.** Los gráficos son secundarios.
  Si hay que elegir, gana lo que hace reír o gritar.
- **Tono:** tensión (oscuridad, trampas) mezclada con momentos ridículos (caídas, ragdoll, arrastrar a un compañero).
- **Tema de la jam:** libre.
- **Cámara:** primera persona. Cada jugador ve sus propios brazos; a los demás los ve con el cuerpo completo
  y con ragdoll al caer o morir.

**Mecánicas cooperativas (prioridad máxima):**
1. **Luz compartida: faroles medievales** (`ALabLantern`). En la mesa del hall hay `max(1, jugadores - 1)`. Se toman, se pasan o se sueltan con E;
   F apaga o reenciende (3 s). Tienen aceite (frascos) y las corrientes de aire los apagan.
2. **Reliquia pesada:** quien la carga va lento y no puede usar la linterna. Se puede pasar a un compañero.
3. **Rescate:** quien cae en una trampa queda caído o atrapado, y los demás lo levantan.
4. **Ayuda del centro:** junto a la reliquia está el *Corazón del Laberinto*, que revive a los muertos
   y recarga todas las luces una vez. Plan B: un amuleto.

**Contenido:** laberinto con seed (backtracker, loops, dead-ends y sala central), trampas con aviso
(pinchos, foso, gas y opcionalmente dardos), items (baterías, tiza, botiquín), fase de escape,
audio guía, HUD mínimo y menú (host, unirse, brillo, volumen).

**Alcance ampliado (Enzo, 12/11):** **Minotauro** (aparece en la fase de escape, se mueve por la grilla con BFS hacia el último ruido fuerte,
más lento que el sprint, avisa con pasos y resoplidos) y **chat de voz por proximidad** (V, push-to-talk; la voz hace ruido).

**Fuera de alcance:** chunks o streaming, enemigos con NavMesh, inventario complejo,
música adaptativa y arte custom elaborado.

## Decisiones técnicas

- **Unreal Engine 5.8.4** en `C:\Program Files\Epic Games\UE_5.8`.
- **VS 2026** con **MSVC 14.50** y Windows SDK 10.0.26100. Rider es el IDE de Enzo.
- **Smart App Control tiene que estar apagado** en las PCs de desarrollo. Si no, bloquea la DLL del juego recién compilada
  (`GetLastError=4551`, *"The game module 'Labrynn' could not be loaded"*). Apagarlo **no es irreversible**: desde la
  actualización de abril de 2026 (KB5083769) se puede volver a activar en Seguridad de Windows sin reinstalar.
  Claude no cambia esta configuración sin permiso.
- **La lógica de juego va en C++** (módulo `Labrynn`, en `Source/Labrynn`). Claude no edita `.uasset`.
  Los Blueprints son solo **clases hijas** para asignar meshes, sonidos y valores.
  Cuando haga falta algo en el editor, se dan pasos exactos.
  - Excepción: los assets simples se pueden crear con scripts de Python del editor, en `Scripts/`.
- **Multijugador desde el día 1** con listen server:
  - Toda la lógica corre con autoridad del servidor (`HasAuthority()`).
  - El estado se replica con `Replicated` o `ReplicatedUsing=OnRep_X`.
  - Las acciones del cliente van como `Server` RPC, y los efectos puntuales como `NetMulticast` (unreliable si son cosméticos).
  - Nunca se hace una versión single-player "para adaptar después".
- **La seed del laberinto** la elige el GameMode del host y la guarda `ALabGameState::MazeSeed` (RepNotify).
  Cada máquina construye el mismo laberinto en local con `FRandomStream`. Las paredes no se replican.
- **Conexión online:** Online Subsystem Steam con AppID 480 (ver DEVLOG). Fallback: IP directa.

## Controles (Enhanced Input: IMC_Default de la plantilla + IMC_Labrynn propio)
WASD moverse · mouse mirar · Espacio saltar (gasta stamina) · **Shift correr** · **C agacharse** (túneles bajos; agachado frente a un borde = pie de banco)
· F apagar/reencender el farol · **E interactuar** (tomar/pasar/soltar, levantar a un caído; **mantener E sobre un caído = cargarlo al hombro**, E otra vez lo suelta;
agarrar la mano de quien la extiende) · **click izq. mantenido = apuntar** piedra/botella (arco), soltar = lanzar · **V hablar** (push-to-talk, proximidad) · Esc menú.
Assets de input: `Scripts/create_input_assets.py` (IA_Sprint, IA_Interact, IA_Lantern, IA_Talk, IA_Crouch, IA_Throw en IMC_Labrynn).

## Mecánicas del diseño v2 (todas en C++, valores en `DA_Gameplay` / `DA_Visuals` / `DA_Minotaur`)
- **Visibilidad mínima sin farol:** adaptación a la oscuridad (la exposición sube en 10 s), musgo y grietas emisivas, haces de luna y un brillo personal tenue.
- **El Minotauro le teme al fuego:** un farol encendido cerca lo hace retroceder 3 s y gasta aceite ×4 mientras lo contiene.
- **Escondites** (`ALabHidingSpot`: barril, heno, nicho): no te detecta y la voz hace menos ruido.
- **Obstáculos:** escombros/vigas (saltar), túneles bajos (agacharse) y bordes altos en el camino principal (solo con ayuda → requiere ≥ 2 jugadores).
  Se validan al generar (alturas contra salto y cápsula).
- **Pie de banco / dar la mano:** agachado frente a un borde, la E del compañero lo impulsa. Desde arriba se extiende la mano (E mantenida) y el de abajo salta + E.
- **Cargar a un caído:** lento (×0,5), gasta stamina, sin farol; si el que carga cae, caen los dos.
- **Lanzables** (`ALabThrowable`): máx. 3; la botella hace ruido fuerte y se rompe, la piedra rebota. El Minotauro va al ruido y los objetos activan trampas a distancia.
- **Voz procesada por estado** (`ULabVoipTalker` + `TickVoice`): herido entrecortada, corriendo agitada, caído ahogada, escondido baja, cargado rebota.

## Stamina / heridas (estilo Peak)
Una sola barra; las heridas bloquean la parte de arriba; si la llenan → caído. Valores en `/Game/Labrynn/Core/DA_Stamina`.
El sprint va en los saved moves (`ULabCharacterMovement`), nunca por RPC.
**El piso del laberinto tiene que ser Static**: si es Movable, el cliente no puede resolver la base de movimiento (ALabMaze no se replica)
e ignora las correcciones del servidor.

## Minotauro (ALabMinotaur)
Aparece en la fase de escape. Oye por AI Perception (Hearing) todo `UAISense_Hearing::ReportNoiseEvent` (tags: Voz, Jadeo, Sprint, Trampa, Piedra, Botella).
Evita fosos, túneles, bordes y celdas con farol encendido. No ataca a jugadores escondidos.
Se mueve con BFS sobre `FLabMazeLayout` (sin NavMesh). Toda acción nueva que deba "hacer ruido" tiene que llamar a `ReportNoiseEvent`.

## Pruebas (regla obligatoria)
- Toda mecánica nueva se prueba **también por el camino del input real**: `Scripts/nettest.ps1 -Extra "-LabKeys=t:Tecla:D|U|P ..."`
  (inyecta en `UGameViewportClient::InputKey`). Los flags que activan cosas por código (`-LabAuto*`) sirven para preparar escenarios,
  **nunca** para dar por probado un control. El código de prueba no puede tocar estado de juego fuera de su ventana activa.
- Se prueba siempre en LAN con host + cliente; el cliente es el caso importante (RPC + predicción).
  Guiones por rol: `-LabKeysHost=...` / `-LabKeysClient=...`. Preparadores de escenario (solo dev): `-LabTeleportTo{Relic,Table,Hide,Client,Throwable,Spike}=T`,
  `-LabTeleportToObstacle=N -LabObstacleType=rubble|tunnel`, `-LabLedgeSetup`, `-LabDownClient=T`, `-LabWoundHost=T`, `-LabThrowClient`, `-LabThrowKind=bottle`,
  `-LabMinotaurSpawnDist=N`. Se verifica primero por logs (`LogLabrynn`); capturas solo si es imprescindible.
- Si vuelven los crash de GPU: probar `r.Shadow.Virtual.Enable=0` y anotarlo en el DEVLOG.

## Convenciones de código

- Las clases C++ usan el prefijo **`Lab`** (`ALabCharacter`, `ALabMaze`…). Los Blueprints hijos usan `BP_` y viven en `Content/Labrynn/`.
- Una clase por archivo: `Public/` para los headers y `Private/` para los cpp, dentro de `Source/Labrynn/`.
- Los valores ajustables son `UPROPERTY(EditDefaultsOnly, Category="Lab|...")`, para que Luis los ajuste desde el Blueprint hijo.
- Los comentarios del código van en español y son breves.
- Después de cada cambio, compilar con
  `& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" LabrynnEditor Win64 Development -Project="C:\Dev\Labrynn\Labrynn.uproject" -WaitMutex`.

## Git y trabajo en equipo (Enzo y Luis)

- Repo: `https://github.com/EnzoCalderon203/Labrynn.git`, rama `main`. Los binarios van en LFS (ver `.gitattributes`).
- **Dueños de carpetas.** Solo el dueño edita los `.uasset` de su carpeta:
  - **Enzo:** `Source/`, `Config/`, `Content/Labrynn/Core/` (Blueprints hijos de gameplay, input, GameMode).
  - **Luis:** `Content/Labrynn/Maps/`, `Content/Labrynn/Audio/`, `Content/Labrynn/UI/`, `Content/Labrynn/Art/`.
  - Para tocar un asset ajeno: `git lfs lock <archivo>`, avisarle al otro y hacer `git lfs unlock` al terminar.
- Los `.uasset` quedan **en solo lectura** (`lockable`) hasta hacer `git lfs lock <archivo>`. Los scripts de Python que modifican assets fallan con`n  `Cannot remove ... read only` si no se bloquean antes. Al terminar: `git lfs unlock`.
- **Commits chicos y frecuentes**: siempre `git pull --rebase` antes de `git push`. Nunca hacer force-push a `main`.
- Nunca se commitean `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/` ni builds empaquetadas.

## Registro

- `DEVLOG.md`: fecha y hora (Paraguay), qué se hizo, qué se decidió y cómo ayudó la IA. Sirve para la categoría Glass Box.
- `PENDIENTES_ENZO.md`: lo que solo puede hacer un humano (UAC, editor, logins).
- Cómo probar en PIE: Play → Net Mode **Play As Listen Server**, Number of Players **2**.
