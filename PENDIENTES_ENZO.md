# Pendientes para Enzo (cosas que Claude no puede o no debe hacer solo)

Ordenados por urgencia. Tachá o borrá cada uno cuando lo hagas.

---

## 1. 🔴 BLOQUEANTE: Smart App Control bloquea la DLL del juego

**Qué pasa:** Windows tiene **Smart App Control** (Control inteligente de aplicaciones) activado.
Desde las 14:42 bloquea cada `UnrealEditor-Labrynn.dll` que compilamos. Las primeras cuatro compilaciones sí cargaron.
- Error en el log de Unreal: `Failed to load '...UnrealEditor-Labrynn.dll' (GetLastError=4551)`.
  El editor muestra: *"The game module 'Labrynn' could not be loaded"*.
- Confirmado en el Visor de eventos → `Microsoft-Windows-CodeIntegrity/Operational` (eventos 3033 y 3077).
- **Mientras siga activado, no vas a poder abrir el proyecto en el editor.** Tampoco Luis, si lo tiene activado.

**Por qué no lo hice yo:** es configuración del sistema (me pediste no tocarla) y es **irreversible**.
Una vez apagado, Smart App Control no se puede volver a encender sin reinstalar o restablecer Windows.
Tu antivirus (Microsoft Defender) sigue funcionando igual. Smart App Control es una capa extra que
casi todo el que compila código termina apagando.

**Cómo apagarlo:**
1. Inicio → escribí **Seguridad de Windows** → abrila.
2. **Control de aplicaciones y navegador** → **Configuración de Smart App Control**.
3. Elegí **Desactivado** y confirmá.
4. Avisale a Claude, que va a repetir la prueba: `powershell -File Scripts\nettest.ps1 -ShotServer 45 -ShotClient 30`.

**Alternativa sin apagarlo:** no conozco ninguna confiable. Habría que firmar la DLL con un certificado de confianza
cada vez que se compila, y eso también requiere tocar el sistema (instalar un certificado raíz).

> ⚠️ **Para la entrega:** los jurados que tengan Smart App Control activado podrían ver bloqueado nuestro `.exe` sin firmar.
> Lo vamos a mencionar en la página de itch ("si Windows lo bloquea: Más información → Ejecutar de todas formas").

## 2. 🟠 Probar el Hito 1 en el editor (después del punto 1)
1. Abrí `C:\Dev\Labrynn\Labrynn.uproject` con doble clic. Si pregunta por *"Missing modules... rebuild?"*, respondé **Sí**.
   Si abrís con Rider: *Open* → `Labrynn.uproject` → aceptá instalar **RiderLink**.
2. Debería abrir el mapa `L_Labrynn`, que está **vacío a propósito**: el laberinto se genera al darle Play.
3. Al lado del botón ▶ Play, en el menú **⋮**:
   - **Number of Players: 2**
   - **Net Mode: Play As Listen Server**
4. Dale **Play**. Se abren dos ventanas.
   - Movete con **WASD**, saltá con **Espacio** y alterná la linterna con **F**.
   - Deberías ver al otro jugador con el cuerpo completo, y vos solo tus brazos.
   - En el Output Log, las dos ventanas tienen que mostrar la misma línea
     `Laberinto construido: seed=... checksum=...`.
5. **Ajustá a gusto** (decime los números y los dejo fijos):
   - **Oscuridad:** `ExposureBias` (ahora +4). También se puede probar desde la línea de comandos con `-LabExposure=5`.
   - **Linterna:** `FlashlightIntensity` (ahora 5000), o `-LabFlashlight=8000`.

## 3. 🟡 Joystick con drift
Hay un **"Wireless Controller" Bluetooth** (joystick de PlayStation) conectado. En una prueba, el personaje del host
caminó solo por el laberinto, y lo más probable es que haya sido drift del stick (el juego lee el joystick aunque la ventana
no tenga el foco). Si no lo vas a usar, **desconectalo mientras probamos**.

## 4. 🟢 Agregar a Luis al repo
GitHub → `EnzoCalderon203/Labrynn` → **Settings → Collaborators → Add people** → su usuario.
Después pasale `PROMPT_LUIS.md`. Pedile que **revise si tiene Smart App Control** (punto 1).

## 5. 🟢 Cuota de Git LFS en GitHub
Ya subimos **135 MB** a LFS (el maniquí de la plantilla pesa 125 MB). Revisá cuánto te queda en
**GitHub → Settings → Billing and plans → Git LFS**. Si el plan gratuito queda corto, avisame: puedo podar animaciones que no usamos.

## 6. ⚪ Opcional: símbolos de depuración del editor
UE 5.8.4 se instaló **sin** "Editor symbols for debugging". Si el editor crashea, los call stacks van a ser poco útiles.
Para instalarlos: Launcher → Biblioteca → flechita de 5.8.4 → **Opciones** → marcar "Símbolos del editor para depuración" → Aplicar.
