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
1. **Luz compartida:** pocas fuentes de luz, con batería limitada. El grupo tiene que moverse junto.
2. **Reliquia pesada:** quien la carga va lento y no puede usar la linterna. Se puede pasar a un compañero.
3. **Rescate:** quien cae en una trampa queda caído o atrapado, y los demás lo levantan.
4. **Ayuda del centro:** junto a la reliquia está el *Corazón del Laberinto*, que revive a los muertos
   y recarga todas las luces una vez. Plan B: un amuleto.

**Contenido:** laberinto con seed (backtracker, loops, dead-ends y sala central), trampas con aviso
(pinchos, foso, gas y opcionalmente dardos), items (baterías, tiza, botiquín), fase de escape,
audio guía, HUD mínimo y menú (host, unirse, brillo, volumen).

**Fuera de alcance:** chunks o streaming, enemigos con NavMesh, inventario complejo,
música adaptativa y arte custom elaborado.

## Decisiones técnicas

- **Unreal Engine 5.8.4** en `C:\Program Files\Epic Games\UE_5.8`.
- **VS 2026** con **MSVC 14.50** y Windows SDK 10.0.26100. Rider es el IDE de Enzo.
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
- **Commits chicos y frecuentes**: siempre `git pull --rebase` antes de `git push`. Nunca hacer force-push a `main`.
- Nunca se commitean `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/` ni builds empaquetadas.

## Registro

- `DEVLOG.md`: fecha y hora (Paraguay), qué se hizo, qué se decidió y cómo ayudó la IA. Sirve para la categoría Glass Box.
- `PENDIENTES_ENZO.md`: lo que solo puede hacer un humano (UAC, editor, logins).
- Cómo probar en PIE: Play → Net Mode **Play As Listen Server**, Number of Players **2**.
