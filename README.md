# Labrynn

Laberinto subterráneo procedural, a oscuras, **cooperativo online para 2 a 4 jugadores**.
Hecho en Unreal Engine 5.8 (C++) para la **2026 Epic AI MegaJam** por *Labrynn Team* (nombre provisional).

> Entren juntos, lleguen al centro, tomen la reliquia y escapen... sin despertar al Minotauro.

## Cómo se juega

1. **Explorar** el laberinto (se genera de nuevo en cada partida) con poca luz y pocos faroles.
2. **Tomar la reliquia** del centro: pesa, te hace lento y no te deja usar el farol.
3. **Escapar** por la salida, que se abre al tomarla. En ese momento despierta el **Minotauro**, que va hacia el ruido.

Nadie lo logra solo: hay bordes que solo se suben con ayuda, compañeros caídos que hay que levantar o cargar,
y una sola fuente de luz para todo el grupo.

## Controles

| Tecla | Acción |
|---|---|
| WASD / mouse | moverse / mirar |
| Shift | correr (gasta stamina) |
| Espacio | saltar |
| C | agacharse (túneles bajos; frente a un borde = pie de banco para un compañero) |
| E | interactuar: tomar, pasar o soltar; levantar a un caído; tender o agarrar la mano |
| E mantenida sobre un caído | cargarlo al hombro (E lo suelta) |
| F | apagar o reencender el farol |
| Click izq. mantenido | apuntar una piedra o botella (soltar = lanzar) |
| V | hablar (chat de voz por proximidad) |
| Rueda arriba | ping (marca lo que mirás) |
| Esc | menú |

## Mecánicas

- **Stamina y heridas al estilo Peak:** una sola barra; las heridas bloquean la parte de arriba y, si la llenan, caés.
- **Faroles medievales** con aceite. Las corrientes de aire los apagan y el Minotauro le teme al fuego.
- **Trampas** con aviso: pinchos, foso y gas.
- **Escondites** (barril, heno, nicho), **obstáculos** (escombros, túneles bajos y bordes altos),
  **piedras y botellas** para distraer y el **Corazón del Laberinto**, que revive al grupo una vez.
- **Voz procesada según tu estado:** herido, agitado, caído, escondido o cargado.
- **Premios graciosos** al final de cada ronda.

## Compilar y correr

Requisitos: **Unreal Engine 5.8.4** y Visual Studio 2026 (MSVC 14.50, Windows SDK 10.0.26100). El repo usa **Git LFS**.

```powershell
git lfs install
git clone https://github.com/EnzoCalderon203/Labrynn.git
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" LabrynnEditor Win64 Development -Project="<ruta>\Labrynn.uproject" -WaitMutex
```

- **Editor:** abrir `Labrynn.uproject` → Play → Net Mode **Play As Listen Server**, **2 jugadores**.
- **Online:** Steam (AppID 480 de prueba) o IP directa.
- **Build empaquetada:** `powershell -File Scripts\package.ps1` → `Packaged\LabrynnTeam_Labrynn.zip`.

> En Windows 11, **Smart App Control** puede bloquear la DLL recién compilada. Ver `PENDIENTES_ENZO.md`.

## Estructura

- `Source/Labrynn/`: toda la lógica de juego en C++ (personaje, laberinto, trampas, Minotauro, red).
- `Content/Labrynn/`: mapas, Data Assets (`DA_Gameplay`, `DA_Visuals`, `DA_Stamina`, `DA_Minotaur`) e input.
- `Scripts/`: creación de assets, pruebas de red automáticas (`nettest.ps1`) y empaquetado.
- `DEVLOG.md`: diario de desarrollo con hora y decisiones (incluye cómo ayudó la IA).

## Herramientas

Unreal Engine 5.8, Visual Studio / Rider, Git + GitHub (LFS) y **Claude Code** (asistente de IA para programar, probar y documentar; ver `DEVLOG.md`).
