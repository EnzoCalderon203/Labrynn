# Prueba de visibilidad: 3 capturas del mismo punto y dirección (sin farol tras 60 s, farol en la mano, farol en el piso a 3 m)
# y tabla de brillo medio por franjas de distancia. Uso: powershell -File Scripts\eye_test.ps1 [-ShotAt 65]
param([int]$ShotAt = 65)
$root = Split-Path $PSScriptRoot -Parent
$ue = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
$logs = Join-Path $root 'Saved\EyeTest'
$shots = Join-Path $root 'Saved\Screenshots\WindowsEditor'
New-Item -ItemType Directory -Force $logs | Out-Null
$port = 7790
foreach ($mode in 'none', 'hand', 'floor') {
    Remove-Item "$logs\eye_$mode.log", "$shots\Eye_$mode.png" -ErrorAction SilentlyContinue
    $a = "`"$root\Labrynn.uproject`" /Game/Labrynn/Maps/L_Labrynn?listen?Seed=4242 -game -windowed -ResX=1280 -ResY=720 -nosound -nosteam -unattended -nosplash " +
         "-port=$port -LabEyeTest=5 -LabEyeLantern=$mode -LabEyeLog -LabAutoShot=$ShotAt -LabShotNoAim -LabShotName=Eye_$mode -LabAutoQuit -abslog=$logs\eye_$mode.log"
    $port++
    # De a uno: 3 instancias con gráficos a la vez tiraron un crash de GPU (D3D Device Removed).
    $p = Start-Process $ue -ArgumentList $a -PassThru
    if (-not $p.WaitForExit(($ShotAt + 90) * 1000)) { $p.Kill() }
    if (Select-String "$logs\eye_$mode.log" -Pattern "GPU Crashed" -Quiet) { Write-Warning "crash de GPU en el caso $mode" }
}
& "$root\Tools\.venv\Scripts\python.exe" "$root\Scripts\eye_bands.py" $logs $shots
