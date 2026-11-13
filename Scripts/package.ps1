# Empaqueta Labrynn (Win64) y arma el zip.
# Uso:  powershell -File Scripts\package.ps1 [-Team "LabrynnTeam"] [-Config Shipping|Development]
#   Shipping    → Packaged\<Team>_Labrynn.zip              (entrega; Shipping no escribe log)
#   Development → Packaged\<Team>_Labrynn_PRUEBA_DEV.zip   (prueba online: log completo en Labrynn\Saved\Logs\Labrynn.log)
# El zip no lleva .pdb, sí steam_appid.txt (480) y dos lanzadores: "Labrynn (Steam).bat" y "Labrynn (IP directa).bat" (-nosteam).
param([string]$Team = 'LabrynnTeam', [ValidateSet('Shipping', 'Development')][string]$Config = 'Shipping')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$uat = 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat'
$out = if ($Config -eq 'Shipping') { Join-Path $root 'Packaged' } else { Join-Path $root 'Packaged\Dev' }
$log = Join-Path $root "Saved\Package_$Config.log"

& $uat BuildCookRun -project="$root\Labrynn.uproject" -platform=Win64 "-clientconfig=$Config" `
    -build -cook -stage -pak -archive -archivedirectory="$out" -noP4 -utf8output -unattended -nocompileeditor *> $log
if ($LASTEXITCODE -ne 0) { Select-String $log -Pattern 'Error' | Select-Object -Last 20; throw "Empaquetado falló (ver $log)" }

$game = Join-Path $out 'Windows'
# Sin esto la build Shipping no inicia Steam (AppID 480 de desarrollo).
Set-Content -Encoding ascii (Join-Path $game 'Labrynn\Binaries\Win64\steam_appid.txt') '480'

# Lanzadores (.bat en vez de accesos directos .lnk: un .lnk guarda la ruta absoluta y se rompe al descomprimir en otra PC).
Set-Content -Encoding ascii (Join-Path $game 'Labrynn (Steam).bat') "@echo off`r`nstart `"`" `"%~dp0Labrynn.exe`"`r`n"
Set-Content -Encoding ascii (Join-Path $game 'Labrynn (IP directa).bat') "@echo off`r`nrem Plan B sin Steam (Tailscale / LAN): Crear partida o Unirse por IP.`r`nstart `"`" `"%~dp0Labrynn.exe`" -nosteam`r`n"
if ($Config -eq 'Development') { Copy-Item (Join-Path $root 'PRUEBA_STEAM.md') $game -Force }

# Zip sin símbolos de depuración.
$staging = Join-Path $out 'zipstage'
Remove-Item $staging -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item $game $staging -Recurse
Get-ChildItem $staging -Recurse -Filter *.pdb | Remove-Item -Force
$name = if ($Config -eq 'Shipping') { "${Team}_Labrynn.zip" } else { "${Team}_Labrynn_PRUEBA_DEV.zip" }
$zip = Join-Path (Join-Path $root 'Packaged') $name
Remove-Item $zip -ErrorAction SilentlyContinue
Compress-Archive -Path "$staging\*" -DestinationPath $zip -CompressionLevel Optimal
Remove-Item $staging -Recurse -Force
"{0}: {1:N0} MB (límite de la jam: 1024 MB)" -f $zip, ((Get-Item $zip).Length / 1MB)
