# Audio de Labrynn

Todo el audio se genera, integra y prueba con scripts. Para cambiar un sonido, ver **Cómo cambiar algo** al final.

## Cómo está armado

```
Scripts/audio/build_audio.py  →  SourceAudio/**/*.wav + manifest.json + sources.json   (Python: síntesis + Kenney CC0)
Scripts/setup_audio.py        →  /Game/Labrynn/Audio/** + /Game/Labrynn/Core/DA_Audio   (editor: importa y configura)
ULabAudio::Play(...)          →  reproduce un evento (variante al azar, volumen y pitch aleatorios, atenuación, concurrencia)
ULabAudioDirector             →  música adaptativa, cama de ambiente, sonidos sueltos, stingers, ping, ducking
```

- **Formato:** WAV 16-bit, 48 kHz. Mono para todo lo 3D; estéreo solo para música, stingers y UI. Normalizado: one-shots a pico −1 a −4 dBFS, loops por RMS (−18 a −26 dBFS).
- **Mezcla** (`/Game/Labrynn/Audio/Mix`):
  - Sound Classes: `SC_Master` > `SC_SFX` > `SC_UI`; `SC_Voice`, `SC_Music` y `SC_Ambience` cuelgan de Master.
  - Submixes: `SM_SFX`, `SM_Voice`, `SM_Music` y `SM_Ambience`.
  - `MIX_Settings`: los 5 sliders del menú (General, Efectos, Voz, Música, Ambiente) se guardan en `GameUserSettings.ini`.
  - La voz por proximidad usa `SC_Voice` (el talker le pone la clase al empezar a hablar).
- **Atenuación 3D = lo que oye el Minotauro:** el alcance de cada sonido es el mismo (o un poco mayor) que el del ruido que reporta para la IA.
  Si el Minotauro lo puede oír, el jugador también. La curva es *Natural Sound* con −45 dB al final; con la distancia se pierden los agudos, y los
  sonidos fuertes se apagan detrás de las paredes (oclusión).

  | Preset | Pleno hasta | Se apaga a | Uso (ruido para la IA) |
  |---|---|---|---|
  | `ATT_Small` | 1 m | 9 m | farol, ítems, respiración, escondites |
  | `ATT_Step` | 1,5 m | 14 m | pasos y saltos (sprint = 14 m) |
  | `ATT_Medium` | 2 m | 18 m | piedra (18 m), caídas, heridas, latido |
  | `ATT_Loud` | 3 m | 32 m | botella (32 m), trampas (30 m), Corazón, reliquia |
  | `ATT_Beast` | 4 m | 45 m | Minotauro |
  | `ATT_Amb` | 2 m | 22 m | goteo, viento, crujidos, retumbes |

- **Concurrencia:** pasos 8 voces, impactos 6, Minotauro 4, ambiente 3, UI 3 y el resto 12. Se descarta primero lo más lejano o lo más viejo, así 4 jugadores no saturan la mezcla.
- **Variación:** cada evento tiene 1 a 5 variantes. Se elige una al azar sin repetir la anterior, con volumen y pitch aleatorios
  (por ejemplo, los pasos van de 0,9 a 1,08 de pitch). Esto reemplaza al nodo Random de los Sound Cues, que no se puede construir desde Python.
- **Música adaptativa:** 3 capas de 48 s (90 bpm, Re menor) que arrancan juntas y suenan siempre; solo cambia su volumen.
  - Exploración: *calma*.
  - Escape: calma + tensión.
  - Minotauro a menos de 35 m: *tensión*.
  - Minotauro persiguiendo a menos de 22 m, o a menos de 10 m: *persecución*.
  - El crossfade entra rápido y sale lento, con histéresis del 35 % para que no oscile en el borde.
  - **Ducking:** mientras alguien habla por voz, la música baja a ×0,35.
- **Red:** los sonidos son cosméticos y nunca suenan en un servidor dedicado. Hay tres caminos:
  - lo que ya se replica suena en `OnRep` (fase, farol, Corazón, reliquia, vida o muerte, estado de trampa);
  - lo que pasa solo en el servidor usa multicast (`ALabCharacter::MulticastSfx`, `ALabMinotaur::MulticastSfx` e impactos de lanzables);
  - pasos, salto y aterrizaje se calculan en cada máquina a partir del movimiento replicado.

## Inventario de eventos (`ELabSfx` → `DA_Audio`)

Origen: **K** = Kenney CC0 procesado · **S** = sintetizado (original) · **K+S** = capas de los dos.

| Evento | Cuándo suena | Variantes | Origen | Atenuación |
|---|---|---|---|---|
| Footstep | cada paso (agachado ×0,3, caminando ×0,6, corriendo ×1) | 5 | K | Step |
| Jump / Land | salto e impulso / aterrizaje (más fuerte si caía rápido) | 2 / 3 | K+S / K | Step |
| BodyFall | alguien cae (ragdoll) | 3 | K | Medium |
| Hurt | herida sin caer (pinchos, golpe) | 3 | K+S | Medium |
| Revive | un caído se levanta | 1 | K+S | Medium |
| Effort | pie de banco / tirar de la mano | 2 | K+S | Medium |
| Breath | loop mientras está sin aire | loop | S | Small |
| Pickup / Drop | juntar piedra o botella, agarrar la reliquia de nuevo / soltar | 2 / 1 | K | Small |
| Throw | lanzar | 3 | S | Small |
| CarryLift | cargar a un caído al hombro | 1 | K+S | Medium |
| HideEnter / HideExit | entrar / salir de un escondite | 2 / 1 | K | Small |
| LanternLight / LanternOut | encender / apagar el farol | 1 / 1 | S | Small |
| LanternFire | crepitar en loop mientras está encendido | loop | S | Small |
| LanternTake | agarrar o recibir el farol | 1 | K | Small |
| OilPickup / MedkitPickup | aceite / botiquín | 1 / 1 | K+S / K | Small |
| StoneImpact | cada rebote fuerte de una piedra | 5 | K | Medium |
| BottleShatter | botella rota | 3 | K+S | Loud |
| TrapWarn / TrapReset | aviso / rearme de cualquier trampa | 2 / 1 | K | Medium |
| SpikeFire / PitFire / GasFire | pinchos / foso que se hunde / corriente de aire | 2 / 1 / 1 | K+S / K+S / S | Loud |
| HeartUse | usar el Corazón | 1 | S | Loud |
| Heartbeat | latido en loop del Corazón (hasta usarlo) | loop | S | Medium |
| RelicTake | primera vez que se toma la reliquia | 1 | S | Loud |
| StingerExit / StingerVictory / StingerDefeat | salida abierta / victoria / derrota (2D) | 1 c/u | S (+K) | 2D |
| Ping / UIClick | ping con la rueda / botones del menú (2D) | 1 / 1 | S | 2D |
| MinoStep / MinoSnort | pasos (más fuertes al perseguir) / resoplidos cada 4–8 s | 4 / 3 | K+S / S | Beast |
| MinoRoar / MinoAttack / MinoFireFear | empieza a perseguir / atrapa a alguien / el fuego lo frena | 2 / 2 / 1 | S / K+S / S | Beast |
| AmbDrip / AmbWind / AmbCreak / AmbRumble | ambiente al azar cada 5–12 s, a 5–16 m del jugador | 5 / 3 / 3 / 2 | S / S / K / S | Amb |
| *Cama de ambiente* | drones de mazmorra en loop (40 s, 2D) | loop | S | 2D |
| *Música* | Calma / Tensión / Persecución (48 s, estéreo) | 3 capas | S | 2D |

En total son **88 WAV (48,7 MB de fuente)**. Licencias y autores: `CREDITS.md`.

## Cómo cambiar algo

- **Volumen, pitch o atenuación de un evento:** se edita `DA_Audio` en el editor, sin compilar.
- **Reemplazar un sonido por uno mejor:** se pone el WAV en `SourceAudio/<carpeta>/` con el mismo nombre y se corre `setup_audio.py`.
  Si es de terceros, se agrega a `CREDITS.md` con su licencia (nada NC, ND ni dudoso).
- **Regenerar los sonidos sintetizados:** `Tools\.venv\Scripts\python.exe Scripts\audio\build_audio.py` (es determinista).
  Después `Scripts\audio\check_audio.py` revisa clipping, DC, silencio y empalmes de los loops.
- **Volver a integrar:** con el editor cerrado,
  `UnrealEditor-Cmd.exe Labrynn.uproject -run=pythonscript -script=Scripts/setup_audio.py`.
  Los assets ya commiteados están en solo lectura (LFS lockable), así que antes hay que hacer `git lfs lock` de los que cambian.
- **Probar sin escuchar:** con `-LabAudioLog`, cada sonido queda en el log (`Sfx[servidor|cliente]: Evento -> archivo posición`),
  también con `-nosound`. La música escribe `Música: calma -> tensión (...)` y `Música: ducking ON`.
- **Entorno para regenerar:** el venv en `Tools\.venv` (fuera de git) se crea con `python -m venv Tools\.venv` y
  `Tools\.venv\Scripts\pip install numpy scipy soundfile`. Los ogg originales de Kenney van en `Tools\downloads\kenney\{impact,rpg}`.
