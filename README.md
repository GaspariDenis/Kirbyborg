# PROMPT PER ANTIGRAVITY — Progetto robot RoboCupJunior Rescue Maze 2027

> Prima di incollare: crea la cartella di progetto e copia i 3 PDF in `docs/inputs/`
> (`RCJRescueMaze2027-draft.pdf`, `Schematic_Kirby_2026-11-13.pdf`, `Schematic_Kirby_2026-11-13__1_.pdf`).
> Poi incolla tutto quello che c'è sotto questa riga.

---

## 0. Ruolo e modo di lavorare

Sei un ingegnere senior di robotica embedded (C++ su Arduino/AVR + Python su Raspberry Pi) con esperienza in RoboCupJunior Rescue Maze.

- Lavora in **modalità Planning**: prima produci un *Implementation Plan* e un file `docs/OPEN_QUESTIONS.md` con assunzioni e dubbi; poi implementa per milestone (sezione 13). A fine di ogni milestone: riepilogo, test eseguiti, cosa provare sul robot vero.
- **Non inventare dati hardware.** Se un'informazione non è leggibile dagli allegati o da questo prompt, crea una costante `TODO_...` in un file di configurazione, registrala in `OPEN_QUESTIONS.md` e **continua con un placeholder sensato** senza bloccarti.
- Codice, commenti e identificatori in **inglese**; documentazione (`docs/`, README) in **italiano**.
- Niente numeri magici: tutto ciò che è calibrabile va in file di config (firmware: `config.h` + EEPROM per le calibrazioni; Pi: `config.yaml`).
- Il codice sorgente verrà consegnato agli organizzatori prima della gara: deve essere leggibile, commentato, e ogni libreria/codice esterno va citato (README → "Credits").

## 1. Obiettivo

Creare l'**intero progetto** (firmware Arduino Mega + software Raspberry Pi + protocollo seriale + simulatore + documentazione) per un robot **completamente autonomo** che gareggia nella RoboCupJunior Rescue Maze 2027 (regolamento allegato, bozza aggiornata al 2026-09-08).

## 2. Allegati e come leggerli

1. `RCJRescueMaze2027-draft.pdf` — regolamento. La sintesi vincolante è nella sezione 4, ma in caso di dubbio **vale il PDF**.
2. `Schematic_Kirby_2026-11-13.pdf` e `Schematic_Kirby_2026-11-13__1_.pdf` — schema della PCB. Sembrano due copie dello stesso schema: confrontale e segnala qualsiasi differenza.

**Istruzioni per lo schema.** Il testo estratto dal PDF contiene i nomi dei pin e delle net ma **non** i collegamenti. Quindi:
- estrai il testo **e** renderizza le pagine come immagini, poi segui visivamente i collegamenti per ricostruire la netlist;
- produci `docs/hardware/PINOUT.md` con tabella *segnale → componente/pin → pin Mega → note → certezza (certo / dedotto / TODO)*;
- genera `firmware/include/pins.h` da quella tabella; per i pin non determinabili usa `TODO_PIN_...`.

**Cosa si vede nello schema (da verificare, non darlo per scontato):**
- Arduino Mega 2560 (U11).
- Driver motori Pololu **Dual MC33926** (U3): segnali `M1PWM, M1DIR, M2PWM, M2DIR`, `M1FB` (current sense), `SF` (status flag), `D2`. Due canali → robot a trazione differenziale.
- **Il pin `D2` del driver NON è collegato sulla PCB**: verrà cablato a mano con un filo volante. Non assegnargli un pin nel firmware, a meno che io non indichi a cosa viene collegato. Verifica sul datasheet MC33926 la polarità di D2 (probabilmente va tenuto ALTO per abilitare le uscite) e documentala in `PINOUT.md`.
- Multiplexer I2C **TCA9548A** (U2) sul bus del Mega (SDA=20, SCL=21). Sei connettori a 6 pin `CN1..CN6`, uno per canale (`SDA1..SDA6`, `SCL1..SCL6`) con +5V e GND. Due resistor network `RN1/RN2` (verosimilmente pull-up I2C). Verifica indirizzo del mux (pin A0–A2) e se `RST` è collegato.
- IMU **MPU-6050** (U1) con pin `XDA, XCL, AD0, INT`. Le net `SDA6/SCL6` compaiono due volte: capisci se la MPU è sul bus principale o dietro il mux, e dove va `AD0` (indirizzo 0x68/0x69).
- Interruttore `SW1`, net `12V`, `12V-S`, `15V-S`, `GND-S1/GND-S2`, `+5V`; `U12` è un connettore a 2 pin "TCA9548A INTERNAL" (capisci a cosa serve).
- **Non compaiono nello schema**: servo, pulsante di start, LED di identificazione, interruttore di taglio alimentazione, encoder, camera. Per ognuno: `TODO_PIN_...` + domanda in `OPEN_QUESTIONS.md`.

## 3. Hardware e librerie

| Elemento | Libreria PlatformIO |
|---|---|
| Servo (dispenser kit di salvataggio) | `Servo` (core Arduino) |
| MPU-6050 | `adafruit/Adafruit MPU6050` |
| TCA9548A | `robtillaart/TCA9548` |
| APDS9960 (colore pavimento) | `adafruit/Adafruit APDS9960 Library` |
| VL53L1X (distanza ToF) | `adafruit/Adafruit VL53L1X` |
| Dipendenze | `adafruit/Adafruit BusIO`, `adafruit/Adafruit Unified Sensor` |

Calcolatori: **Arduino Mega 2560** (tempo reale) + **Raspberry Pi** (alto livello), collegati in **seriale tramite una singola porta** (assumi USB: Mega `Serial` ↔ Pi `/dev/ttyACM0`; se invece fosse UART di pin, ricorda che il Mega è a 5 V e il Pi a 3.3 V → serve level shifter; segnalalo).

Progetto firmware con **PlatformIO** (`platformio.ini`, `board = megaatmega2560`, `framework = arduino`, `lib_deps` come da tabella).

**Note tecniche che devi gestire:**
- **TCA9548A / libreria Tillaart**: il dubbio "non si sa se sia compatibile" è da chiudere. La libreria è pensata per la famiglia TCA9548, quindi dovrebbe andare, ma va verificato. Scrivi `tools/i2c_scan` (sketch che seleziona ogni canale e scansiona gli indirizzi) e un wrapper `I2CMux` con interfaccia astratta, così da poter passare a un'implementazione manuale (`Wire.beginTransmission(0x70); Wire.write(1 << ch);`) se la libreria desse problemi.
- Più VL53L1X hanno tutti indirizzo 0x29 e più APDS9960 tutti 0x39: sono separabili solo grazie al mux → **un sensore per canale**, selezione canale prima di ogni transazione (con cache del canale attivo), un oggetto driver per sensore.
- Assegnazione sensori ↔ canali in un'unica tabella di config. Non so quanti sensori di ogni tipo ci sono né dove sono montati: tabella con placeholder (tipico: 3–4 VL53L1X frontale/sinistra/destra/(posteriore) + 1–2 APDS9960 rivolti verso il pavimento).
- Nessun magnetometro: la MPU-6050 dà **yaw solo per integrazione del giroscopio** → calibrazione del bias a robot fermo, compensazione drift, riallineamento della direzione ai multipli di 90° usando i muri. Pitch/roll da accelerometro+giroscopio (filtro complementare) per riconoscere rampe.
- I2C: `Wire.setClock(400000)`, timeout e recupero da bus bloccato, watchdog hardware.
- SRAM del Mega = 8 KB: tieni un budget di memoria, riportalo nei log di build (`avr-size`) e a runtime (free RAM).
- **Encoder: non li vedo nello schema.** Se non ci sono, la distanza di una tile (28 cm) va stimata con ToF frontale (distanza dal muro modulo 28 cm), eventi di fine muro laterale e velocità calibrata nel tempo. Progetta il modulo di movimento con interfaccia astratta così da aggiungere encoder in seguito, e chiedimi se ci sono.

## 4. Regolamento — sintesi vincolante per il design

**Campo**
- Tile **28×28 cm**; muri alti ≥ 12.5 cm; corridoio tra muri opposti 28 cm; altezza robot ≤ 25 cm; i muri non sono infinitamente sottili (margini ridotti).
- Pavimento: **nero** = buco (visitarla = LoP); **argento** = checkpoint; **blu** = pozza (dopo averla visitata bisogna restare fermi **5 s consecutivi** prima di cambiare tile, altrimenti LoP); **rosso** = ingresso della *Dangerous Zone* (area opzionale più difficile: detriti non fissati ≤ 1 cm, rampe/scale < 30°, dossi ≤ 2 cm).
- Dislivelli fra tile fino a 3 mm. Rampe ≤ 25°, sempre dritte. Scale: alzata ≤ 2 cm, ripiano ≥ 15 cm, inclinazione < 25° (< 30° in DZ). Dossi ≤ 1 cm (2 cm in DZ), mai su rampe/scale (in DZ sì sulle rampe).
- **Ostacoli** alti ≥ 15 cm, a ≥ 20 cm dal muro o appoggiati a un muro e a ≥ 20 cm dal bordo opposto; se spostati restano dove sono.
- **Ponti**: tile sopra ad altre tile, altezza libera ≥ 25 cm → **la mappa è multilivello** e una stessa (x,y) può avere due livelli.
- Una sola **start tile** (ovunque fuori dalla DZ). I muri possono essere cambiati prima della run. Le tile nere/blu/rosse/argento sono piazzate (casualmente) **dopo che il robot parte** → vietata la pre-mappatura. Illuminazione e campi magnetici variabili, possibili flash di fotocamere. Tolleranza di tutte le misure ±10%.

**Vittime** (a ~7 cm dal pavimento, mai su muri affacciati a tile nere/argento/blu/rosse, con ostacoli/dossi/scale, né su rampe)
- **Smiley victims**: emoji Ø 4 cm, di colore distinguibile dal muro, anche ruotata. Triste = *harmed*, neutra = *stable*, felice = *unharmed*.
- **Cognitive targets**: cerchio Ø 5 cm con 5 anelli concentrici (Ø 1,2,3,4,5 cm). Valori colore: nero −2, rosso −1, giallo 0, verde +1, blu +2. Si **sommano i 5 anelli separatamente** (anelli adiacenti dello stesso colore NON si fondono): somma 2 = harmed, 1 = stable, 0 = unharmed; **qualsiasi altra somma = bersaglio falso → non va identificato**. (Esempio dal regolamento: giallo, blu, rosso, nero, verde dal centro → 0+2−1−2+1 = 0 → unharmed.)
- Possono esserci **oggetti simili a vittime ma non vittime** (simboli o colori diversi): non vanno identificati.

**Kit di salvataggio**
- Almeno 1.5 cm per lato, volume ≥ 3.375 cm³ dopo il rilascio; deve **restare vicino alla vittima** (non rotolare/rimbalzare via); rilascio interamente entro **15 cm** dalla vittima; rilascio ben visibile all'arbitro. Non c'è più il limite di 8 kit, ma il magazzino è fisico: traccia i kit rimasti.

**Robot**
- Autonomo; avviato manualmente dal capitano; **vietato** dead reckoning pre-programmato e qualsiasi informazione sul campo data al robot; niente sensori che "vedano oltre" i muri; un solo robot; maniglia per sollevarlo; un solo pulsante fisico binario visibile all'arbitro per partire (all'inizio e dopo un LoP) e **al massimo un altro interruttore** per togliere alimentazione: **la procedura dopo LoP può contenere solo questo** e va comunicata all'arbitro prima di ogni run; un LED/display dedicato (**Identification Light**) usato solo per vittime ed exit bonus, ben visibile; vietato marcare il campo; laser solo classe 1–2; max 48 V DC.

**Gara**
- **8 minuti** totali (calibrazione + run). Fine partita: tempo scaduto, il capitano chiama la fine, oppure il robot torna alla start tile e ottiene l'exit bonus.
- **Lack of Progress (LoP)**: il capitano la dichiara, il robot visita una tile nera, visita un'altra tile senza fermarsi 5 s dopo una blu, danneggia il campo, un membro tocca campo/robot senza permesso. Dopo un LoP il robot viene rimesso **sull'ultimo checkpoint visitato** (o sulla start tile se non ne ha visitati) **in qualsiasi orientamento**. Il tempo continua a scorrere.
- "Tile visitata" = più di metà robot dentro la tile vista dall'alto.

**Punteggio**
| Voce | Punti |
|---|---|
| Identificazione vittima su **tile lineare** | smiley 5 / cognitive 10 |
| Identificazione vittima su **tile flottante** | smiley 15 / cognitive 30 |
| Kit (solo se la vittima è stata prima identificata) | harmed: 10 con 1 kit, **30 totali** con 2 kit; stable: 10 con 1 kit; unharmed: 0 kit |
| Tile blu | 30 alla prima visita, −10 per ogni rivisita (minimo 0) |
| Checkpoint visitato | 10 |
| Tile con dossi attraversata | 5 |
| Rampa (su o giù) | 10 |
| Scala (su o giù) | 10 |
| **Exit bonus** | 10×SVI + 10×SBV + 5×SSN + 5×SRN |
| **Reliability bonus** | max(0, SVI×10 + SRD×10 + SBV×10 − LoP×15) |
| Misidentificazione | −5 (totale mai < 0) |

- **Identificare una vittima** = robot **fermo entro 15 cm** dalla vittima, Identification Light che lampeggia **ON 500 ms / OFF 500 ms per 5 secondi completi**, restando fermo.
- **Exit bonus** = robot sulla **start tile**, fermo, con Identification Light **accesa per almeno 5 s**. Farlo su una tile diversa dalla start conta come **misidentificazione**.
- **Nessun premio duplicato** (stesso elemento/tile premiato una volta sola).
- **Tile lineare** = raggiungibile dalla start tile seguendo costantemente il muro più a sinistra o più a destra; **tutte le altre sono flottanti**. Le tile nere contano come muri virtuali, quindi la classificazione dipende dalla posizione delle nere.

**Conseguenze di design (da rispettare):**
1. Le vittime su tile flottanti valgono il **triplo** → un semplice wall-follower non basta: serve **esplorazione completa** (frontier-based) con calcolo dinamico di quali tile sono flottanti sulla mappa corrente.
2. Il robot deve **tornare alla start tile entro gli 8 minuti** per l'exit bonus: il Pi calcola una *deadline di rientro* = costo del percorso più breve verso la start × margine + 5 s di exit + margine di sicurezza, e avvia la fase "return home" in tempo.
3. Ogni blu va visitata **una sola volta** (fermandosi bene dentro la tile per 5 s) e conta sia nel punteggio che nell'exit bonus.
4. Nero = riflesso di sicurezza **nel firmware** (non può dipendere dalla latenza del Pi): il sensore pavimento deve vedere il nero prima che più di metà robot sia entrato.
5. Un LoP costa 15 punti di reliability: evitare i LoP vale più di tentare tile rischiose.
6. La Dangerous Zone è opzionale: il Pi deve valutare costo/rischio/punti prima di entrarci.

## 5. Architettura — chi fa cosa

**Raspberry Pi = cervello (stato autorevole)**
Mappa multilivello, localizzazione a livello di tile, pianificazione/esplorazione, visione (camere), logica vittime/kit, tempo di gara, stato della run, persistenza su disco, log.

**Arduino Mega = esecutore in tempo reale**
Motori (PWM/DIR, rampe di accelerazione, lettura `M1FB`/`SF`), controllo di heading (PID su yaw), centraggio nel corridoio con i ToF laterali, lettura ToF/APDS9960/MPU, riconoscimento eventi di basso livello (colore pavimento, rampa, scala, dosso, stallo), blu (attesa 5 s), **riflesso nero**, servo dei kit, Identification Light (blink 500/500 e fisso), pulsante di start/interruttore, watchdog.

Il Mega espone al Pi **primitive** (es. "avanza di una tile", "ruota di ±90°", "rilascia N kit") e restituisce **eventi/telemetria**. Non decide la strategia.

> Nota: assumi una o due **camere** collegate al Pi (CSI o USB), montate basse e puntate lateralmente. Non sono nello schema: chiedimi modello/posizione.

## 6. Protocollo seriale Pi ↔ Mega

Definiscilo in un'unica fonte di verità `shared/protocol.yaml` e **genera** da lì l'header C++ (`firmware/include/protocol_gen.h`) e il modulo Python; documenta in `docs/PROTOCOL.md`.

Requisiti:
- Framing robusto (COBS o delimitatori + lunghezza), **CRC16**, numero di sequenza, ACK/retry con timeout, handshake con versione protocollo, parsing **non bloccante**, dimensione massima frame compatibile con gli 8 KB del Mega.
- **Nessun `Serial.print` di debug sulla stessa porta del protocollo**: i log del Mega viaggiano come messaggi `LOG` del protocollo.
- Heartbeat bidirezionale: se il Mega non sente il Pi per T ms → ferma i motori (fail-safe); se il Pi non sente il Mega → stato `LINK_LOST`.

Messaggi minimi:
- **Pi → Mega**: `PING`, `RESTORE_BEGIN/CHUNK/END` (pose + mappa + contatori), `MOVE_TILE`, `TURN(delta)`, `STOP`, `DEPLOY_KIT(n, lato)`, `LED(mode: off/on/blink)`, `CALIBRATE(what)`, `CONFIG_SET`.
- **Mega → Pi**: `BOOT(reset_cause, fw_version, boot_counter)`, `BUTTON_PRESSED`, `TELEMETRY(tof[], yaw, pitch, roll, floor_class, motor_current)`, `EVENT(tile_entered{floor}, ramp_up/down, stairs, speedbump, black_avoided, blue_wait_done, stall)`, `CMD_DONE/CMD_FAIL(reason)`, `ACK`, `LOG`.

## 7. Mappa

Modello (Pi, in `world_map`):
- Griglia **multilivello** (x, y, z) con dimensioni massime configurabili (`MAX_W`, `MAX_H`, `MAX_LEVELS`).
- Per ogni cella: muri N/E/S/O (sconosciuto/aperto/muro), tipo pavimento (normale, nero, argento, blu, rosso), flag `visited`, `checkpoint`, `speedbump`, `ramp`, `stairs`, `obstacle`, `in_dangerous_zone`, `blue_visit_count`, vittima (tipo, salute, `identified`, `kits_delivered`), collegamenti fra livelli (rampe/scale), `confidence`.
- Calcolo dinamico di **lineare/flottante** secondo la definizione del regolamento (wall-following sinistro e destro dalla start tile, nere come muri virtuali).
- Pose del robot a livello di tile + heading discreto (N/E/S/O), con odometria raffinata dai ToF e dalla MPU.

## 8. **Ripartenza dopo LoP (cuore del progetto)**

Scenario reale: dopo un LoP il capitano **toglie l'alimentazione** (l'Arduino si riavvia da zero), rimette il robot sull'ultimo checkpoint in un orientamento qualsiasi, riaccende e preme **l'unico pulsante**. Nessun'altra azione è consentita. Il Pi deve far ripartire il robot **da solo**, ripassando al Mega **l'intera mappa fatta fino a quel momento** e il **punto da cui riparte**.

Implementa questo flusso e documentalo in `docs/LOP_RESTORE.md`:

1. **Macchina a stati della run (Pi):** `IDLE → ARMED → RUNNING → (LOP_WAIT → RESTORING → RUNNING)* → ENDED`.
   - Una nuova run si arma in fase di calibrazione (comando CLI `new_run`) e inizia al primo `BUTTON_PRESSED`.
   - `ENDED` quando: exit bonus completato, 8 min trascorsi dall'inizio, o chiamata di fine da CLI.
   - Se in `RUNNING` arriva un `BOOT` dal Mega ⇒ **non è una nuova run ma un riavvio da LoP** → ripristino.
2. **Snapshot ai checkpoint:** ogni volta che il robot visita una tile argento, il Pi salva su disco in modo **atomico** (file temporaneo + fsync + rename) lo stato completo (mappa, pose del checkpoint, vittime identificate, kit rimasti, contatori punteggio, `lop_count`, tempo di run). Tieni anche una copia in RAM.
3. **Sequenza di ripristino:**
   a. Mega si riavvia → invia `BOOT` → resta **fermo** (nessun movimento) e calibra il bias del giroscopio mentre aspetta.
   b. Il Pi invia `RESTORE_BEGIN/CHUNK/END` con mappa completa + pose di ripartenza + contatori; il Mega verifica CRC e risponde `READY`.
   c. Solo dopo il `BUTTON_PRESSED` il Pi pianifica e riparte.
4. **Cosa si conserva e cosa no:** la conoscenza del campo (muri, tile speciali, vittime già identificate, kit consegnati, blu già visitate) **si conserva**, perché i muri non cambiano durante la run e i premi non sono duplicabili. Si **resetta** solo la pose: posizione = ultimo checkpoint, azione in corso annullata. Le celle esplorate *dopo* l'ultimo checkpoint si marcano `unverified` (la pose potrebbe essere stata sbagliata prima del LoP) e vengono ri-verificate quando costa poco. La tile che ha causato il LoP, se nota (es. nera/blu), deve già essere nella mappa **prima** del riavvio (scrivi sul disco appena arriva l'evento dal Mega).
5. **Heading incognito:** dopo il riavvio la yaw del Mega riparte da 0 e il robot può essere in una direzione qualsiasi. Il Pi **ri-orienta** confrontando i muri visti dai ToF (4 lati) con i muri noti della tile di checkpoint, provando le 4 rotazioni e scegliendo quella coerente. Se più rotazioni sono compatibili (tile simmetrica) → procedura di disambiguazione sicura (es. muoversi verso un lato aperto in tutte le ipotesi e riconfrontare con la tile adiacente).
6. **Mirror della mappa sul Mega:** il Pi tiene la mappa autorevole; il Mega ne riceve una copia **compatta** (bit-packed; calcola il costo in SRAM per `MAX_W×MAX_H×MAX_LEVELS`) usata per i riflessi di sicurezza (es. rifiutare un comando che entrerebbe in una tile nera nota). Se la copia completa non entra nei limiti di SRAM insieme al resto, scegli e documenta l'alternativa (finestra locale attorno al robot, oppure solo bitmap delle tile nere/blu) — ma il **protocollo di RESTORE completo deve esistere comunque**.
7. **Alimentazione del Pi:** l'interruttore di taglio post-LoP deve togliere corrente a **Mega e motori**, non al Pi. Segnala in `OPEN_QUESTIONS.md` che la linea del Pi deve essere separata. Come rete di sicurezza, rendi il Pi robusto anche a un proprio riavvio: servizio `systemd` con `Restart=always`, ripresa da `state/` su disco se trova una run `RUNNING`, timer di run persistito periodicamente (best effort, il Pi non ha RTC).
8. Registra tutto (ogni LoP, ogni ripristino, ogni snapshot) in un log JSONL con timestamp per l'analisi post-gara.

## 9. Firmware Mega — moduli richiesti

`firmware/src/` e `firmware/include/`:
- `drivers/`: `MotorDriver` (MC33926: PWM, rampe, soglie di corrente da `FB`, gestione fault `SF`), `I2CMux`, `ToFArray` (VL53L1X multipli, budget di timing, letture non bloccanti), `FloorColor` (APDS9960: RGB + clear → classi normale/nero/argento/blu/rosso con isteresi, soglie calibrabili in EEPROM, procedura di calibrazione pre-run), `Imu` (MPU-6050, bias giroscopio, yaw/pitch/roll), `KitDispenser` (servo, N kit, sequenza di rilascio ripetibile), `IdentificationLight` (blink 500/500 per 5 s non bloccante; modalità fissa per l'exit), `Button`/`PowerSwitch` con debounce.
- `control/`: heading PID, centraggio col muro, `move_one_tile`, `turn_90`, sempre non bloccanti (state machine).
- `behaviors/`: salita/discesa rampa (rilevazione da pitch, velocità adattiva, stallo da corrente), scale, riconoscimento dossi (picco su asse Z), riflesso nero, attesa blu (≥ 5 s con margine configurabile, dopo essere entrato abbastanza nella tile).
- `link/`: protocollo, parser, mirror mappa.
- Scheduler cooperativo, **nessun `delay()` bloccante**, loop di controllo ≥ 100 Hz, watchdog hardware, recupero I2C, report della RAM libera.
- Sketch di bring-up separati (`tools/`): scan I2C col mux, test singolo di ogni sensore, test motori, test servo, test LED.
- Test unitari su ambiente PlatformIO `native` per la logica pura (CRC, parser, codifica mappa, classificatore colori).

## 10. Software Pi — moduli richiesti (Python ≥ 3.11)

`raspberry/src/rescue/`:
- `serial_link` (pyserial/asyncio, riconnessione, protocollo generato), `run_state` (macchina a stati), `world_map`, `localization` (pose + ri-orientamento dopo LoP), `exploration` (frontier exploration su grafo multilivello con pesi per tile flottanti, vittime note, costo, rischio; Dijkstra/A*; evita nere; gestione blu; return-home con deadline), `mission` (sequenza vittima: ferma entro 15 cm → blink 5 s → kit giusti → conferma), `scoring` (replica esatta del punteggio del regolamento, per stima in tempo reale e per i test), `persistence`, `logging`, `cli` (`new_run`, `status`, `calibrate`, `end_run`, `replay`).
- `vision/`: OpenCV, acquisizione camere, rilevamento e classificazione di
  - smiley (cerchio Ø 4 cm, colore distinto dal muro, **anche ruotata** → classifica triste/neutro/felice, es. dalla curvatura della bocca; approccio classico prima, piccola rete TFLite solo come alternativa);
  - cognitive target (campionamento radiale dei 5 anelli, classificazione colore in HSV/LAB con calibrazione pre-run, somma separata dei 5 anelli);
  - **rigetto** di falsi (somma diversa da 0/1/2, simboli o colori non previsti);
  - robustezza a variazioni di luce e flash; soglie di confidenza; stima distanza combinata con i ToF per fermarsi entro 15 cm.
  - generatore di immagini sintetiche per i test.

## 11. Struttura del repository

```
rescue-maze-2027/
├─ README.md                  (credits, setup, come buildare/avviare)
├─ docs/
│  ├─ inputs/                 (i 3 PDF)
│  ├─ hardware/PINOUT.md
│  ├─ ARCHITECTURE.md  PROTOCOL.md  LOP_RESTORE.md
│  ├─ RULES_SUMMARY.md  TESTING.md  PRE_GARA_CHECKLIST.md
│  └─ OPEN_QUESTIONS.md
├─ shared/protocol.yaml       (+ script di generazione C++/Python)
├─ firmware/                  (PlatformIO: platformio.ini, include/, src/, lib/, test/)
├─ raspberry/                 (pyproject.toml, src/rescue/, tests/, config.yaml, systemd/)
├─ sim/                       (simulatore del campo + Mega virtuale)
└─ tools/                     (i2c_scan, bring-up, strumenti di calibrazione)
```

## 12. Simulatore e test

- **Simulatore Python** del campo (tile, muri, nero/argento/blu/rosso, rampe/scale/dossi, vittime di entrambi i tipi, falsi bersagli) con un **Mega virtuale** che parla lo stesso protocollo (via pty/loopback) e rumore sui sensori.
- Il simulatore può iniettare **LoP casuali** e riavvii del Mega in qualunque momento (anche durante un trasferimento `RESTORE_CHUNK`).
- Test automatici: ripristino dopo LoP in vari punti; heading ambiguo; corruzione/perdita di frame; classificazione lineare/flottante (con i casi della figura del regolamento); punteggio (inclusi esempi del regolamento, tipo il cognitive target 0+2−1−2+1 = 0); deadline di rientro; l'esplorazione completa di campi generati a caso senza mai entrare in una nera.

## 13. Milestone (fermati a fine di ciascuna con un riepilogo)

- **M0** — Analisi: `PINOUT.md`, `RULES_SUMMARY.md`, Implementation Plan, `OPEN_QUESTIONS.md`.
- **M1** — Scheletro repo, `protocol.yaml` + generazione codice, CRC/parser con test.
- **M2** — Driver Mega e sketch di bring-up (scan mux, ogni sensore, motori, servo, LED).
- **M3** — Movimento e comportamenti del Mega (tile, rotazioni, rampe, scale, dossi, nero, blu).
- **M4** — Mappa, persistenza, macchina a stati, **ripartenza dopo LoP** + simulatore.
- **M5** — Esplorazione, tile flottanti, return-home, logica vittime/kit, scoring.
- **M6** — Visione (smiley, cognitive target, rigetto falsi).
- **M7** — Integrazione, test hardware-in-the-loop, checklist pre-gara, pulizia finale.

## 14. Vincoli e cose da NON fare

- Nessun layout del labirinto cablato nel codice, nessun movimento pre-programmato, nessun dato sul campo passato al robot.
- Nessun sensore/camera che veda oltre i muri; altezza totale ≤ 25 cm.
- Il robot non deve mai danneggiare il campo: velocità prudenti su rampe/scale e fail-safe sui motori.
- La procedura post-LoP deve richiedere **solo** pulsante + (al massimo) l'interruttore di alimentazione: nessun comando da laptop/SSH durante la run.
- Nessuna modifica di codice o di parametri dopo l'inizio della run (la calibrazione è ammessa solo prima).
- Non toccare/assegnare il pin `D2` del driver (cablato a mano) senza mie istruzioni.
- Non inventare mapping di pin: `TODO_PIN_...` + domanda.

## 15. Domande che devi riportarmi alla fine di M0 (nel file `OPEN_QUESTIONS.md`)

1. Quanti VL53L1X e APDS9960 ho montato, dove, e su quali canali CN1–CN6 / MPU?
2. Ci sono encoder? Se sì, su quali pin?
3. Pin di servo, Identification Light, pulsante di start, interruttore di taglio, `D2` (a cosa lo collego).
4. Quali camere (modello, CSI/USB) e dove sono montate? Il dispenser dei kit serve un lato o entrambi? Quanti kit contiene?
5. Collegamento seriale Pi↔Mega: USB o UART di pin?
6. Architettura di alimentazione: il Pi è su una linea separata da quella tagliata dall'interruttore?
7. Dimensione massima del campo (tile e livelli) da assumere per la mappa.
8. Configurazione ruote/motori (2 motori, 4 ruote accoppiate?), diametro ruote, larghezza robot.

---
*Fine del prompt.*
