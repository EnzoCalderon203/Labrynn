# Créditos — Labrynn

Juego hecho para la **2026 Epic AI MegaJam** (12–15/11/2026) por *Labrynn Team* (Enzo Calderón y Luis).
Todo el código y el diseño se hicieron durante la jam.

## Audio

### Sonidos de terceros (CC0)

| Pack | Autor | Origen y licencia (verificada) | Licencia |
|---|---|---|---|
| **Impact Sounds** | Kenney (kenney.nl) | https://kenney.nl/assets/impact-sounds | [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) |
| **RPG Audio** | Kenney (kenney.nl) | https://kenney.nl/assets/rpg-audio | [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) |

La licencia se verificó en la página de origen (kenney.nl) el 13/11/2026. kenney.nl bloquea las descargas automáticas,
así que los mismos archivos se tomaron de copias públicas en GitHub que redistribuyen los packs completos
(Impact Sounds: `iree-gd/iree.gd`, carpeta `sample/assets/audio/impact`, 130 archivos; RPG Audio: `pigdevstudio/assets`,
carpeta `sandbox/audio/rpg_sfx`). Al ser CC0, se pueden redistribuir y modificar sin restricciones.

**Archivos de Kenney usados.** Todos se procesaron: cambio de tono, capas, reverb, filtros y normalización.
- *Impact Sounds:* `footstep_concrete_000–004`, `impactGlass_heavy_000–002`, `impactGlass_medium_000`, `impactMetal_heavy_000–001`,
  `impactMetal_light_000–002`, `impactMining_000–004`, `impactPunch_heavy_000–004`, `impactSoft_heavy_000–004`,
  `impactWood_heavy_000–003`, `impactWood_medium_000–002`.
- *RPG Audio:* `beltHandle1`, `cloth1–4`, `creak1–3`, `doorOpen_1`, `drawKnife1`, `dropLeather`, `handleSmallLeather`,
  `handleSmallLeather2`, `metalClick`, `metalLatch`.

En `SourceAudio/sources.json` figura qué originales entran en cada WAV del juego.

### Sonidos y música originales (sintetizados para el juego)

Se generaron desde cero con `Scripts/audio/build_audio.py` (Python + numpy/scipy, sin samples). Son **100 % originales**
y propiedad del equipo:

- **Música adaptativa** (3 capas de 48 s, 90 bpm, Re menor, que se mezclan entre sí): *Calma* (colchón y campanas),
  *Tensión* (pulso grave y trémolo disonante) y *Persecución* (tambores tipo taiko, bajo ostinato y golpes de metal).
- **Ambiente:** cama de drones de mazmorra (loop de 40 s), viento por grietas, goteo en caverna y retumbes del laberinto.
- **Minotauro:** rugido (sierra con formantes, gruñido y distorsión), rugido de miedo al fuego y resoplidos.
  Los pasos y el ataque son mezclas de impactos de Kenney bajados de tono con sub-graves sintetizados.
- **Faroles:** encendido, apagado y crepitar de la llama (loop).
- **Corazón del Laberinto:** latido (loop) y activación. **Reliquia:** campana inarmónica.
- **Trampas:** corriente de aire del gas, derrumbe del foso (capas sintetizadas más una madera de Kenney) y chirrido de los pinchos.
- **Jugador:** respiración agitada (loop), lanzamiento (whoosh) y revivir (arpegio).
- **Interfaz y stingers:** ping, clic, salida abierta, victoria y derrota.

## Arte y otros assets

- **Maniquí, animaciones y brazos en primera persona:** plantilla *First Person* de Unreal Engine 5.8 (Epic Games),
  usada bajo el EULA de Unreal Engine.
- **Formas básicas** (cubos, cilindros y esferas provisorias): contenido del motor (`/Engine/BasicShapes`), bajo el EULA de Unreal Engine.
- Los materiales del laberinto se generan por script (`Scripts/create_materials.py`).

## Herramientas

- **Unreal Engine 5.8.4** (C++, Enhanced Input, AI Perception, Online Subsystem Steam, VOIP).
- **Visual Studio 2026** (MSVC 14.50) y **JetBrains Rider**.
- **Git + GitHub** (con Git LFS).
- **Python 3.12** con numpy, scipy, soundfile y matplotlib (síntesis y control de audio).
- **Claude Code** (Anthropic, modelo Claude Opus): asistente de IA usado para escribir y revisar el código C++,
  los scripts del editor y de audio, sintetizar el sonido, correr las pruebas automáticas de red y documentar.
  El detalle de qué hizo la IA y qué decidió el equipo está en `DEVLOG.md`.
