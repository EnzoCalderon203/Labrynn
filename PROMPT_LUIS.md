# Prompt de instalación para Luis (pegalo en tu Claude Code)

> Luis: abrí Claude Code en PowerShell, en cualquier carpeta, y pegá todo lo que está debajo de la línea.
> Antes, pedile a Enzo que te agregue como colaborador del repo de GitHub.

---

Necesito dejar mi PC con Windows lista para desarrollar el juego **Labrynn** (Unreal Engine 5 con C++ y multijugador)
para la 2026 Epic AI MegaJam. Mi compañero Enzo ya configuró su PC y **tengo que tener exactamente las mismas versiones**.
Si no coinciden, el proyecto C++ no compila igual en las dos máquinas.

## Versiones obligatorias (las mismas que en la PC de Enzo)

| Herramienta | Versión |
|---|---|
| Unreal Engine | **5.8.4** (desde el Epic Games Launcher) |
| Visual Studio | **Community 2026** (18.x) |
| Toolchain MSVC | **14.50**: componente `Microsoft.VisualStudio.Component.VC.14.50.18.0.x86.x64` |
| Windows SDK | **10.0.26100**: componente `Microsoft.VisualStudio.Component.Windows11SDK.26100` |
| .NET SDK | **10.x** |
| Git + Git LFS | la última de Git for Windows (incluye LFS) |
| Rider | opcional, 2026.2.x (`JetBrains.Rider` en winget) |

## Reglas

- Ejecutá un comando a la vez y verificá cada uno antes de seguir.
- Si un comando falla, mostrame el error y la causa, y proponé un arreglo. No pruebes cosas al azar.
- Si aparece una ventana de permisos de administrador (UAC), avisame para que la acepte.

## Pasos

1. **Diagnóstico.** Revisá:
   - la versión de Windows, la RAM, la GPU y si `winget` funciona;
   - el espacio libre en cada disco y si es SSD o HDD (`Get-PhysicalDisk` + `Get-Partition`);
   - si ya están instalados Git, .NET, Visual Studio (con `vswhere.exe`) y el Epic Games Launcher.

   Unreal con C++ necesita unos **100 GB en un SSD**. Si no alcanza, frená y avisame.

   **Revisá también Smart App Control:**
   `(Get-ItemProperty 'HKLM:\SYSTEM\CurrentControlSet\Control\CI\Policy').VerifiedAndReputablePolicyState`
   (0 = apagado, 1 = activado, 2 = evaluación). En la PC de Enzo estaba activado y **bloqueó la DLL del juego**
   (`GetLastError=4551`, *"The game module 'Labrynn' could not be loaded"*). Si está en 1 o 2, explicame el problema
   y guiame para apagarlo en Seguridad de Windows → Control de aplicaciones y navegador. No lo cambies vos:
   es irreversible y lo decido yo.
2. **Git + LFS:** `winget install --id Git.Git -e`, después `git lfs install`. Verificá con `git --version` y `git lfs version`.
3. **.NET 10 SDK**, si no está: `winget install --id Microsoft.DotNet.SDK.10 -e`.
4. **Visual Studio Community 2026.**
   - **Si no está instalado:** instalalo con winget (`Microsoft.VisualStudio.Community`) y usá `--override` con los `--add` de abajo.
   - **Si ya está instalado:** modificalo con
     `"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\setup.exe" modify --installPath "<ruta>" --passive --norestart --add ...`

   Agregá estos workloads y componentes:
   ```
   Microsoft.VisualStudio.Workload.NativeGame
   Microsoft.VisualStudio.Workload.NativeDesktop
   Microsoft.VisualStudio.Workload.ManagedDesktop
   Microsoft.VisualStudio.Workload.NetCrossPlat
   Microsoft.VisualStudio.Component.VC.Tools.x86.x64
   Microsoft.VisualStudio.Component.VC.14.50.18.0.x86.x64
   Microsoft.VisualStudio.Component.Windows11SDK.26100
   Microsoft.VisualStudio.Component.VC.DiagnosticTools
   Microsoft.VisualStudio.Component.VC.ASAN
   Component.Unreal.Ide
   Component.Unreal.Debugger
   Component.Unreal
   ```
   Al configurar la PC de Enzo aparecieron dos problemas:
   - Si lanzás el instalador con `Start-Process`, pasá **un solo string** de argumentos con la ruta entre comillas.
     Si pasás un array, la ruta `C:\Program Files\...` se parte en dos y el instalador termina con código 87.
   - `setup.exe modify` **no acepta `--wait`**: también termina con código 87. Usá `Start-Process -Wait`.
   - Si algo falla, el log está en `%TEMP%\dd_installer_*.log`.

   Al terminar, verificá con `vswhere` que estén los workloads, y que existan
   `VC\Tools\MSVC\14.50.*` y `Windows Kits\10\Include\10.0.26100.0`.
5. **Epic Games Launcher**, si no está: `winget install --id EpicGames.EpicGamesLauncher -e`.
6. **Rider (opcional):** preguntame antes de instalarlo (`winget install --id JetBrains.Rider -e`).
7. **Pasos manuales.** Guiame paso a paso y esperá a que te confirme cada uno:
   - Iniciar sesión en el Epic Games Launcher.
   - En **Unreal Engine → Biblioteca → +**, elegir **5.8.4** e instalar en un SSD. En **Opciones** marcar:
     - Componentes principales
     - Plantillas y paquetes de funciones
     - Símbolos del editor para depuración

     Dejar desmarcado el código fuente y **todas** las plataformas de destino (Android, iOS, Linux…).
   - Esperar a que la carpeta `UE_5.8\Engine` exista y el Launcher diga que terminó.
8. **Verificación:** creá un proyecto C++ vacío de prueba fuera de OneDrive y compilalo con
   `Engine\Build\BatchFiles\Build.bat <Proyecto>Editor Win64 Development -Project="<ruta>.uproject"`.
   Confirmá que compila y después borralo.
9. **Clonar el repo** fuera de OneDrive (por ejemplo, en `C:\Dev`):
   ```
   git clone https://github.com/EnzoCalderon203/Labrynn.git C:\Dev\Labrynn
   cd C:\Dev\Labrynn
   git lfs pull
   ```
   Configurá `git config user.name` y `user.email` (los míos). Después hacé clic derecho en `Labrynn.uproject` →
   *Generate Visual Studio project files*, o abrilo directamente con Rider. Compilá y abrí el editor.
10. Leé el `CLAUDE.md` del repo: tiene las reglas del proyecto (quién es dueño de qué, commits, deadline).
    Escribí en `DEVLOG.md` qué instalaste, con qué versiones y en qué fecha y hora (hora de Paraguay, UTC-3).

**Mi rol en el equipo:** mapas, audio, Blueprints hijos (meshes, sonidos y valores) y HUD.
Enzo se encarga del código C++ y del multijugador.
