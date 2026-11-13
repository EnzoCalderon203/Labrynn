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

## 2b. 🟠 Prueba online REAL por Steam (Enzo ↔ Luis). Necesita dos PCs y dos cuentas de Steam
Claude solo pudo probar el lado del host: con una sola cuenta, la segunda instancia no puede usar Steam.
1. Los dos con **Steam abierto y con sesión iniciada**. Si son amigos en Steam, mejor.
2. Los dos abren el juego **en modo standalone**: clic derecho en `Labrynn.uproject` → *Launch game*.
   También sirve desde el editor: Play → **Standalone Game**. En PIE común Steam no se activa.
3. El menú tiene que decir `servicio: STEAM`. Si dice `NULL`, Steam no inició; revisen que esté abierto.
4. Enzo → **Crear partida**. Luis → **Buscar partidas** → aparece "Unirse a Enzo (1/4)" → clic.
   - Otra opción: **Shift+Tab** → Amigos → clic derecho en Enzo → **Unirse a la partida**.
5. Si no aparece o no conecta, pásenle a Claude el log `Saved\Logs\Labrynn.log` de **las dos** PCs.

**Fallback si Steam falla:** IP directa. Los dos instalan **Tailscale** (gratis) y lo abren con `-nosteam`. Enzo crea la partida;
Luis escribe la IP 100.x.x.x de Enzo y toca **Unirse por IP**.

## 2c. 🟡 Aviso del Firewall de Windows sobre "UnrealPak"
Al empaquetar apareció un aviso: *"¿Quieres permitir que las redes públicas y privadas accedan a esta aplicación? UnrealPak"*.
Claude no lo tocó. UnrealPak es la herramienta de empaquetado y no necesita red, así que podés poner **Cancelar**.
**Ojo:** cuando abras el **juego** o el **editor** en modo standalone y el firewall pregunte por `Labrynn-Win64-Shipping`
o por `UnrealEditor`, ahí sí hay que poner **Permitir**. Si no, nadie se va a poder conectar a tu partida.

## 2d. 🟢 Probar el zip en la PC de Luis
`Packaged\LabrynnTeam_Labrynn.zip` (291 MB) se genera con `powershell -File Scripts\package.ps1`.
Antes del domingo conviene descomprimirlo en otra PC (la de Luis) para confirmar que arranca sin tener Unreal instalado.

## 2e. 🟠 Probar el chat de voz con Luis
En la prueba por Steam (2b): mantener **V** y hablar. Verificar que (1) se escucha, (2) baja con la distancia,
(3) suena apagado detrás de una pared. Si no se escucha nada, pasarle a Claude `Saved\Logs\Labrynn.log` de las dos PCs.
Windows tiene que tener el micrófono permitido para apps de escritorio.

## 2f. 🟠 Luis: pruebas con micrófono (diseño v2, punto 8)
La voz se procesa según el estado de **quien habla**, y eso lo escucha **el otro**. Las pruebas automáticas no tienen micrófono:
solo verifican por logs (`Voz[...]: ahogada (caído)`, etc.). Hay que escucharlo de verdad.
En LAN o Steam, Luis mantiene **V** y habla mientras:
1. **Normal**, parado: referencia.
2. **Corriendo** (Shift) o sin stamina → tiene que sonar "agitada" (oscila un poco).
3. **Herido** (≥ 50 % de la barra bloqueada; pisar pinchos dos veces) → entrecortada, con cortes breves.
4. **Caído** (foso) → ahogada, como con la boca tapada.
5. **Escondido** (E en barril o heno) → más baja y apagada.
6. **Cargado** (Enzo mantiene E sobre su cuerpo y camina) → el volumen "rebota".
7. Detrás de una pared → apagada (oclusión), y baja con la distancia.

Si algún efecto es demasiado fuerte o flojo, ajustar la categoría **Voz** de `DA_Gameplay`
(`VoiceDownedLowPass`, `VoiceHiddenVolume`…) o avisarle a Claude.

## 2g. 🟠 Enzo: escuchar el audio con auriculares y decirle a Claude qué cambiar
Todo el audio ya está integrado: 44 eventos, música adaptativa, ambiente y volúmenes en el menú.
Claude lo verificó por logs (cada evento dispara su sonido en host y cliente) y por análisis numérico, pero **no lo pudo escuchar**.
Abrí el juego (PIE, 2 jugadores) con auriculares y anotá lo que suene mal, fuerte, flojo o raro. Las referencias son los nombres de `AUDIO.md`.

**Menú**
1. ¿Suena la música (capa *calma*) y el **clic** de los botones? ¿Los 5 sliders de **Volumen** cambian lo que tienen que cambiar?

**Explorando**
2. **Cama de ambiente:** ¿el drone grave de mazmorra se siente como cueva o molesta? ¿Se nota el corte del loop (cada 40 s)?
3. **Sonidos sueltos de ambiente** (goteo, viento, crujidos, retumbes) cada 5–12 s: ¿asustan bien o son demasiado seguidos o fuertes?
4. **Pasos:** caminar, correr (Shift) y agachado (C). ¿Suenan a piedra? ¿Se repiten de forma notoria? ¿Los pasos del compañero se ubican bien en 3D?
5. **Salto y aterrizaje**, escombros, pie de banco (**esfuerzo**).
6. **Farol:** encender y apagar (F), **crepitar** de la llama (¿suena a fuego o a estática?) y agarrarlo (E).
7. **Trampas:** clic de aviso, pinchos, foso que se hunde, corriente de aire (gas) y rearme.
8. **Lanzables:** juntar, el *whoosh* al lanzar, rebotes de la piedra, botella rota.
9. **Escondites** (entrar y salir), **heridas**, **caída** (ragdoll), **cargar** a un caído, **revivir**, **respiración** sin aire.
10. **Ping** (rueda): ¿se escucha bien sin molestar?

**Centro y escape**
11. **Latido del Corazón** al acercarte y el sonido al **usarlo**. **Campana de la reliquia**.
12. **Stinger de salida abierta** (estruendo + reja). Después debería entrar la capa de **tensión**.
13. **Minotauro:**
    - pasos pesados y resoplidos (¿suena a bestia grande?);
    - **rugido** cuando oye algo (¿asusta o da risa?);
    - **ataque**;
    - quejido ante el **fuego**.
14. **Música adaptativa:** lejos del Minotauro debería sonar calma/tensión; cerca, *tensión*; si te persigue, *persecución* (tambores).
    ¿Los cambios son suaves? ¿La persecución es demasiado fuerte?
15. **Ducking:** mientras alguien mantiene **V**, ¿baja la música?
16. **Victoria / derrota:** ¿se oyen los stingers sobre la pantalla de premios?

**Cómo ajustar:** casi todo se arregla sin compilar en `DA_Audio` (volumen y pitch por evento, distancias de la música, ducking) o con los sliders.
Si un sonido no sirve, decile a Claude cuál y cómo debería sonar ("más grave", "menos agudo", "más corto"): se regenera con `build_audio.py`.
## 2h. 🟡 Mirar en pantalla (la lógica está probada por logs, lo visual no)
En PIE (2 jugadores, Listen Server):
1. **Cargar a un caído:** ¿el cuerpo se ve cruzado sobre el hombro, o queda raro? Si queda raro, avisar (se ajusta un offset).
2. **Click izquierdo mantenido** con una piedra: ¿se ve bien el arco de puntitos?
3. **Ping** (rueda arriba): ¿se ve el rombo amarillo con el nombre y los metros?
4. **Premios** al ganar o perder: ¿se leen bien debajo del mensaje final?

## 3. 🟡 Joystick con drift
Hay un **"Wireless Controller" Bluetooth** (joystick de PlayStation) conectado. En una prueba, el personaje del host
caminó solo por el laberinto, y lo más probable es que haya sido drift del stick (el juego lee el joystick aunque la ventana
no tenga el foco). Si no lo vas a usar, **desconectalo mientras probamos**.

## 4. 🟢 Agregar a Luis al repo
GitHub → `EnzoCalderon203/Labrynn` → **Settings → Collaborators → Add people** → su usuario.
Después pasale `PROMPT_LUIS.md`. Pedile que **revise si tiene Smart App Control** (punto 1).

## 5. 🟢 Cuota de Git LFS en GitHub
Ya subimos **~210 MB** a LFS (el maniquí de la plantilla pesa 125 MB; el audio, ~70 MB entre WAV fuente y assets). Revisá cuánto te queda en
**GitHub → Settings → Billing and plans → Git LFS**. Si el plan gratuito queda corto, avisame: puedo podar animaciones que no usamos.

## 6. ⚪ Opcional: símbolos de depuración del editor
UE 5.8.4 se instaló **sin** "Editor symbols for debugging". Si el editor crashea, los call stacks van a ser poco útiles.
Para instalarlos: Launcher → Biblioteca → flechita de 5.8.4 → **Opciones** → marcar "Símbolos del editor para depuración" → Aplicar.
