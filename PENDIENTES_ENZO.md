# Pendientes para Enzo (cosas que Claude no puede o no debe hacer solo)

Ordenados por urgencia. Tachá o borrá cada uno cuando lo hagas.

---

## 1. ✅ RESUELTO (12/11): Smart App Control bloqueaba la DLL del juego

Enzo lo desactivó y la DLL carga (estado verificado: `VerifiedAndReputablePolicyState = 0`).
- **Corrección:** antes escribí que era irreversible, y está mal. Desde la actualización de abril de 2026
  (**KB5083769**), Smart App Control se puede volver a activar desde Seguridad de Windows → Control de aplicaciones
  y navegador → Configuración de Smart App Control, **sin reinstalar Windows**.
- Si algún día lo reactivás, el editor va a volver a fallar con `GetLastError=4551` al cargar la DLL del juego.

> ⚠️ **Para la entrega:** los jurados que tengan Smart App Control activado podrían ver bloqueado nuestro `.exe` sin firmar.
> Lo vamos a mencionar en la página de itch ("si Windows lo bloquea: Más información → Ejecutar de todas formas").

## 2. 🟠 Probar el Hito 1 en el editor
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
5. **Ajustá a gusto** (Claude ya los calibró con capturas; decime si los querés más oscuros o más claros):
   - **Oscuridad:** `ExposureBias` (ahora +4,5). También se puede probar desde la línea de comandos con `-LabExposure=5`.
   - **Linterna:** `FlashlightIntensity` (ahora 3000), o `-LabFlashlight=4000`.

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
