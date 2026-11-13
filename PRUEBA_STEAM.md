# Prueba online por Steam: Enzo ↔ Luis

**Qué zip usar:** `LabrynnTeam_Labrynn_PRUEBA_DEV.zip`. Es el build *Development*: igual al juego final, pero **escribe un log completo**
de red, Steam y voz. El Shipping, que es el de la entrega, no escribe log. Pesa unos 300 MB.

## 1. Abrir el juego (los dos)

1. **Steam abierto y con sesión iniciada** (cada uno con su cuenta). Si son amigos en Steam, mejor.
2. **Descomprimí el zip** en una carpeta, por ejemplo `C:\Juegos\Labrynn`. No lo abras desde adentro del zip.
3. Doble clic en **`Labrynn (Steam).bat`**.
   - Si aparece **"Windows protegió tu PC"** (SmartScreen): *Más información* → **Ejecutar de todas formas**.
   - Si aparece el **Firewall de Windows**: marcá las dos casillas (redes privadas y públicas) → **Permitir**.
     Si no lo permitís, nadie se va a poder conectar a tu partida.
4. En el menú tiene que decir **`servicio: STEAM`** debajo del título. Si aparece el **aviso en rojo** ("Steam no disponible"),
   el juego no vio a Steam: Steam cerrado, o **otra ventana de Labrynn abierta** (fijate en el Administrador de tareas). Cerrá todo y abrí una sola vez.
5. **Micrófono:** Windows → Configuración → Privacidad → Micrófono → "Permitir que las aplicaciones de escritorio accedan" **activado**.

## 2. Crear y unirse

- **Enzo** → **Crear partida** y se queda **dentro del laberinto** (salir con Esc cierra la partida para todos; ahora pide apretar Esc **dos veces**).
- **Luis** → **Buscar partidas** → aparece **"Unirse a Enzo (1/4)"** → clic.
  - Si no aparece: esperá 10 s y buscá de nuevo.
  - Otra opción: **Shift+Tab** (overlay de Steam) → Amigos → clic derecho en Enzo → **Unirse a la partida**.
- Solo aparecen partidas de Labrynn: el AppID 480 lo comparten miles de juegos de prueba, y filtramos por una clave propia.

## 3. Checklist (15 minutos)

Anoten **la hora** de cada problema (aunque sea aproximada): con eso se encuentra en el log.

| Min | Qué probar | Bien si… |
|---|---|---|
| 0–2 | **Conexión** | Luis entra; los dos se ven con el cuerpo completo y el laberinto es el mismo. |
| 2–3 | **Lag** | Correr (Shift) y saltar uno al lado del otro: el otro se mueve fluido, sin teletransportes. |
| 3–5 | **Voz (V, mantener)** | Se escucha. Baja con la distancia (a ~15 m casi nada). Detrás de una pared suena apagada. La música baja mientras alguien habla. |
| 5–6 | **Faroles** | Uno toma un farol de la mesa (E) y se lo pasa al otro (E mirándolo). F apaga y reenciende. Los dos ven la luz y oyen el crepitar. |
| 6–7 | **Dar la mano** | En un borde alto: uno se agacha (C), el otro se para enfrente y toca E (pie de banco). Arriba, mantener E para tender la mano; el de abajo salta + E. |
| 7–8 | **Cargar al caído** | Que uno caiga (un foso sirve). El otro mantiene E sobre el cuerpo → lo carga al hombro → camina → E lo suelta → E lo levanta. |
| 8–9 | **Piedras y botellas** | Juntar con E, mantener click izquierdo (arco), soltar para lanzar. El otro oye el golpe o la botella rota. |
| 9–10 | **Escondites** | E en un barril o heno. El otro no te ve, y tu voz suena más baja. |
| 10–12 | **Reliquia + Minotauro** | Tomar la reliquia en el centro → stinger, se abre la salida y aparece el Minotauro. Hacer ruido → ruge y persigue. El farol lo frena. |
| 12–14 | **Victoria** | Llevar la reliquia a la salida (luz roja). Pantalla de premios en los dos. |
| 14–15 | **Derrota** | En la ronda siguiente, que caigan los dos. Pantalla de derrota en los dos. |

## 4. Plan B: si Steam no conecta (Tailscale + IP directa)

1. Los dos instalan **Tailscale** (https://tailscale.com/download, gratis) y entran con su cuenta.
   Enzo invita a Luis a su red (*Share* o *Invite*).
2. Enzo copia su IP de Tailscale (empieza con **100.**, se ve en el ícono de Tailscale).
3. Los dos abren **`Labrynn (IP directa).bat`** (sin Steam). El menú dice `servicio: NULL`.
4. **Enzo** → **Crear partida**. **Luis** → escribe la IP de Enzo (`100.x.y.z`) en la caja de IP → **Unirse por IP**.
5. Firewall: igual que antes, **Permitir**. La voz en este modo puede no andar (no usa Steam): si no anda, anótenlo.

## 5. Al terminar: mándenle a Claude

- **El log de cada uno:** `<carpeta del juego>\Labrynn\Saved\Logs\Labrynn.log`.
  Si abrieron el juego varias veces, también los `Labrynn-backup-*.log` de esa carpeta.
  **Mandarlo después de cerrar el juego** (así queda completo).
- **Las notas** con la hora de cada problema.

El log trae: entrada y salida de jugadores (`Red: entró…`), ping y pérdida de paquetes cada 10 s (`Red[…]: ping=…`), búsqueda y unión
(`Online: …`), errores de red (`Error de red`), Steam, sesiones y voz (`LogOnlineVoice`, `Voz: …`), y todo lo del juego.
