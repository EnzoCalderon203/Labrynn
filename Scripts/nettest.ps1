# Prueba de red automática: listen server + 1 cliente en 127.0.0.1, con capturas de pantalla.
# Uso:  powershell -File Scripts\nettest.ps1 [-Seed 4242] [-NoRender] [-ShotServer 60] [-ShotClient 40]
# Resultado: logs en Saved\NetTest\, capturas en Saved\Screenshots\WindowsEditor\
#   (HighresScreenshot00000 = la primera que se sacó; el script dice cuál es cuál).
param(
    [int]$Seed = 4242,
    [switch]$NoRender,
    # El cliente saca su captura y cierra ANTES que el host (si el host cierra primero, el cliente se desconecta).
    [int]$ShotServer = 60,
    [int]$ShotClient = 25,
    [int]$ClientDelay = 15,
    [string]$Extra = '',  # parámetros extra para ambos, ej. "-LabExposure=6 -LabFlashlight=8000"
    [switch]$Menu         # flujo real: menú → Crear partida / Buscar y unirse (LAN con -nosteam)
)
$ErrorActionPreference = 'Stop'
$ue   = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$root = Split-Path $PSScriptRoot -Parent
$proj = Join-Path $root 'Labrynn.uproject'
$logs = Join-Path $root 'Saved\NetTest'
$shots = Join-Path $root 'Saved\Screenshots'
New-Item -ItemType Directory -Force $logs | Out-Null
Remove-Item "$logs\*.log" -ErrorAction SilentlyContinue
Remove-Item $shots -Recurse -ErrorAction SilentlyContinue

$render = if ($NoRender) { '-nullrhi' } else { '-windowed -ResX=1280 -ResY=720' }
$common = "-game $render -nosound -unattended -nosplash -log $Extra"

# Solo el host se cierra solo (-LabAutoQuit); al cliente lo cierra el script cuando termina el host.
if ($Menu) {
    $srvTarget = '-nosteam -LabMenuHost'   # sin mapa: arranca en L_Menu (GameDefaultMap)
    $cliTarget = '-nosteam -LabMenuJoin'
} else {
    $srvTarget = "/Game/Labrynn/Maps/L_Labrynn?listen?Seed=$Seed"
    $cliTarget = '127.0.0.1'
}
$srv = Start-Process $ue -PassThru -ArgumentList "`"$proj`" $srvTarget $common -LabAutoQuit -WinX=0 -WinY=0 -abslog=`"$logs\server.log`" -LabAutoShot=$ShotServer"
Start-Sleep -Seconds $ClientDelay
$cli = Start-Process $ue -PassThru -ArgumentList "`"$proj`" $cliTarget $common -WinX=640 -WinY=360 -abslog=`"$logs\client.log`" -LabAutoShot=$ShotClient"

$srv.WaitForExit(($ShotServer + 120) * 1000) | Out-Null
Stop-Process -Id $cli.Id, $srv.Id -Force -ErrorAction SilentlyContinue

Add-Type -AssemblyName System.Drawing
Get-ChildItem $shots -Recurse -Filter *.png -ErrorAction SilentlyContinue | ForEach-Object {
    $bmp = [System.Drawing.Bitmap]::FromFile($_.FullName); $sum = 0; $n = 0
    for ($x = 0; $x -lt $bmp.Width; $x += 16) { for ($y = 0; $y -lt $bmp.Height; $y += 16) { $c = $bmp.GetPixel($x, $y); $sum += ($c.R + $c.G + $c.B) / 3; $n++ } }
    $bmp.Dispose()
    "{0}: brillo medio {1:N1}/255" -f $_.Name, ($sum / $n)
}

foreach ($n in 'server', 'client') {
    "===== $n"
    Select-String -Path "$logs\$n.log" -Pattern 'LogLabrynn|Join succeeded|Welcomed|Warning: .*Lab|Error: .*Lab' |
        ForEach-Object { $_.Line -replace '^\[[^\]]+\]\[[^\]]+\]', '' }
}
