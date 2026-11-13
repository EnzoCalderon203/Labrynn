# Plan Labrynn: del jueves 12/11 al domingo 15/11 (hora de Paraguay, UTC-3)

**Deadline duro: domingo 22:59.** Objetivo interno: entregado a las **21:00**.
Estado: ✅ hecho · 🔄 en curso · ⏳ pendiente. Responsables: **E** = Enzo + Claude (C++ y red), **L** = Luis (mapas, audio, BP, HUD).

**Regla de recorte:** si un bloque se pasa más de 1 h de lo previsto, se recorta la feature de menor prioridad del día.
Orden de recorte: dardos → tiza → amuleto/Corazón → gas → botiquín.

## Jueves 12: Hito 1 (dos jugadores online en un laberinto generado)
| Hora | Qué | Quién | Estado |
|---|---|---|---|
| 13:00–14:30 | Fase 0: instalar y verificar el toolchain; repo; CLAUDE.md | E | ✅ |
| 14:30–15:30 | Proyecto C++, laberinto con seed replicada, 2 jugadores con cuerpo completo (prueba de red automática) | E | ✅ |
| 15:30–17:00 | Pulido del Hito 1: oscuridad real (exposición fija), materiales, calibración con capturas | E | ✅ (14:50) |
| 17:00–20:00 | **Online por internet**: OSS Steam (AppID 480), Host/Join con menú mínimo y fallback por IP directa | E | ✅ código (15:00), falta probar entre 2 PCs |
| 15:03–15:09 | Adelanto del viernes: **luz compartida** (batería, 1 linterna cada 2, brillo personal, HUD, Esc) | E | ✅ |
| 15:09–15:16 | Adelanto del viernes: **reliquia + fase de escape + victoria + reinicio** | E | ✅ |
| 15:16–15:25 | Adelanto del viernes: **rescate** (caído, ragdoll, levantar, desangrado, derrota) | E | ✅ |
| 15:25–15:31 | Adelanto del viernes: **trampas 1 y 2** (pinchos con placa, foso) | E | ✅ **Hito 2 cumplido** |
| 15:31–15:33 | Adelanto del sábado: **Corazón del Laberinto** + **baterías** | E | ✅ |
| 15:33–15:41 | **Ensayo temprano de empaquetado** (zip 291 MB) + trampa de gas | E | ✅ |
| 20:00–22:00 | Prueba real Enzo ↔ Luis por internet. Luis instala con `PROMPT_LUIS.md` | E+L | ⏳ |

## Viernes 13: Hito 2 (se puede jugar de principio a fin)
| Hora | Qué | Quién |
|---|---|---|
| 09:00–11:00 | **Luz compartida**: batería de la linterna, apagado y parpadeo, linternas escasas | E |
| 11:00–13:00 | **Reliquia**: tomarla, peso (lento, sin linterna), soltarla y pasarla | E |
| 14:00–16:00 | **Rescate**: caído / ragdoll, levantar a un compañero, muerte si nadie llega | E |
| 16:00–18:30 | **Trampas 1 y 2**: pinchos con placa de presión y foso (piso falso), con aviso sonoro/visual | E (+L sonidos) |
| 18:30–20:30 | **Fase de escape**: se abre la salida (luz y sonido), aumenta el peligro | E |
| 20:30–22:00 | Victoria y derrota, reinicio de ronda. **Partida completa jugable** | E |
| todo el día | Mapa de menú y lobby, primeros sonidos, materiales del laberinto | L |

## Sábado 14: Hito 3 (juego completo)
| Hora | Qué | Quién |
|---|---|---|
| 09:00–11:00 | Trampa 3: gas que apaga la luz (opcional: dardos) | E |
| 11:00–13:00 | Items: baterías/bengalas, tiza (decals en paredes), botiquín | E |
| 14:00–15:30 | **Corazón del Laberinto** (ayuda del centro): revive a los muertos y recarga las luces, una vez | E |
| 15:30–18:00 | Audio de guía (pasos, goteo, avisos, gritos), HUD (luz, salud, item), menú (brillo, volumen) | L (+E hooks) |
| 18:00–22:00 | **Playtest online con 2 a 4 personas y balance** (tamaño del laberinto, batería, trampas) | E+L |

## Domingo 15: pulido y entrega
| Hora | Qué |
|---|---|
| 09:00–13:00 | Bugs del playtest, pulido de "momentos graciosos" (ragdoll, gritos) |
| 13:00–15:00 | Últimos ajustes; ensayo de la build empaquetada |
| **15:00** | **FEATURE FREEZE**: solo bugfixes desde acá |
| 15:00–17:00 | Build **Shipping** empaquetada, zip `LabrynnTeam_Labrynn.zip` (< 1 GB), prueba en la PC de Luis |
| 17:00–19:00 | Video de gameplay (30–60 s), capturas, página de itch, lista de herramientas e IA, CREDITS |
| 19:00–21:00 | **Subir a itch y enviar a la jam.** Verificar que el envío figure |
| 21:00–22:59 | Margen de emergencia. Nada nuevo |

## Decisiones del plan
- **Ayuda del centro:** *Corazón del Laberinto*. Es un objeto de un solo uso junto a la reliquia que revive
  a todos los muertos y recarga todas las linternas. Es simple, tiene impacto social ("¡volvé, te revivo!")
  y premia llegar juntos. Plan B: un *amuleto de cuerda* que deja tirar de un compañero caído desde lejos.
- **La salida no es la entrada**: queda en el borde norte, cerrada hasta que se toma la reliquia.
  Obliga a explorar de nuevo en la fase de escape, con la tiza como ayuda.

## Alcance ampliado (aprobado por Enzo, jueves 12/11 a la noche)
Orden de trabajo, con commit y prueba en LAN (con input real) después de cada punto:
| # | Qué | Estado |
|---|---|---|
| 0 | Bug: Shift no corría en PIE + migración a Enhanced Input | ✅ |
| 1 | Farol medieval, mesa con jugadores-1, aceite, corriente de aire, reencendido de 3 s | ✅ |
| 2 | Chat de voz por proximidad (V, atenuación 3D, oclusión; la voz hace ruido) | ✅ lógica (falta probar el audio con 2 personas) |
| 3 | Minotauro (fase de escape, BFS por la grilla hacia el ruido, golpe = heridas grandes + ragdoll) | ✅ |
| 4 | Si sobra tiempo: premios graciosos al final + ping con la rueda del mouse | ⏳ |
Regla de recorte si se atrasa: 4 → antorchas → oclusión de voz → resoplidos 3D del Minotauro.

## Diseño v2 (pedido de Enzo, jueves 12/11 a la noche)
Mismo procedimiento: compilar, probar en LAN con input real (host + cliente), commit y DEVLOG.
| # | Qué | Estado |
|---|---|---|
| 1 | Visibilidad mínima sin farol (adaptación, musgo, luna, brillo personal) | ✅ |
| 2 | El Minotauro le teme al fuego | ✅ |
| 3 | Escondites (barril, heno, nicho) | ✅ |
| 4 | Obstáculos + agacharse (C), con validación de camino | ✅ |
| 5 | Pie de banco + dar la mano (bordes en el camino principal) | ✅ |
| 6 | Cargar a un caído al hombro | ✅ (pose sin verificar con captura) |
| 7 | Piedras y botellas (arco, ruido, activan trampas) | ✅ |
| 8 | Voz procesada por estado | ✅ lógica · falta escucharlo con micrófono (Luis, PENDIENTES 2f) |
| 9 | Premios graciosos al final + ping con la rueda del mouse | ✅ |

## Audio completo (viernes 13/11, sin Luis)
| Parte | Qué | Estado |
|---|---|---|
| 1 | Inventario y fuentes: Kenney CC0 (Impact Sounds, RPG Audio) + síntesis propia; `CREDITS.md` | ✅ |
| 2 | `ULabAudio` + `DA_Audio` (44 eventos), música adaptativa en 3 capas, ducking, ambiente, stingers, volúmenes en el menú | ✅ |
| 3 | `setup_audio.py`: sound classes, submixes, atenuación = ruido de la IA, concurrencia | ✅ |
| 4 | Prueba en LAN por logs (host y cliente) | ✅ |
| 5 | Escucharlo con auriculares y ajustar (PENDIENTES 2g) | ⏳ Enzo |