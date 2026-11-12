# Empaqueta Labrynn (Shipping, Win64) y arma el zip de entrega para itch.io.
# Uso:  powershell -File Scripts\package.ps1 [-Team "LabrynnTeam"]
# Resultado: Packaged\<Team>_Labrynn.zip (sin .pdb, con steam_appid.txt).
param([string]$Team = 'LabrynnTeam')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$uat = 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat'
$out = Join-Path $root 'Packaged'
$log = Join-Path $root 'Saved\Package.log'

& $uat BuildCookRun -project="$root\Labrynn.uproject" -platform=Win64 -clientconfig=Shipping `
    -build -cook -stage -pak -archive -archivedirectory="$out" -noP4 -utf8output -unattended -nocompileeditor *> $log
if ($LASTEXITCODE -ne 0) { Select-String $log -Pattern 'Error' | Select-Object -Last 20; throw "Empaquetado falló (ver $log)" }

$game = Join-Path $out 'Windows'
# Sin esto la build Shipping no inicia Steam (AppID 480 de desarrollo).
Set-Content -Encoding ascii (Join-Path $game 'Labrynn\Binaries\Win64\steam_appid.txt') '480'

# Zip sin símbolos de depuración.
$staging = Join-Path $out 'zipstage'
Remove-Item $staging -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item $game $staging -Recurse
Get-ChildItem $staging -Recurse -Filter *.pdb | Remove-Item -Force
$zip = Join-Path $out "${Team}_Labrynn.zip"
Remove-Item $zip -ErrorAction SilentlyContinue
Compress-Archive -Path "$staging\*" -DestinationPath $zip -CompressionLevel Optimal
Remove-Item $staging -Recurse -Force
"{0}: {1:N0} MB (límite de la jam: 1024 MB)" -f $zip, ((Get-Item $zip).Length / 1MB)
