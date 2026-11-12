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
