# Guida alla lettura dei sorgenti C

Questa guida spiega com'è organizzata la riscrittura e come interpretare le
parti che a prima vista sembrano numeri senza significato. Il programma in
esecuzione è C nativo: non carica né esegue il file DOS. Il vecchio eseguibile
è usato solo dagli strumenti di analisi e confronto.

## La mappa: disegno nel sorgente e stato in memoria

La mappa storica viene costruita da `maze_initialize()` in `src/maze.h`,
usando le istruzioni di disegno in `src/build.h`. Dal 5 ottobre 2026 non sono
più incorporate le due matrici iniziali né la lista delle scritture per cella:
tratti con motivi ripetuti e didascalie producono gli stessi caratteri e colori.
I nomi `MAZE_DOT`, `MAZE_WALL` e gli altri glifi indicano i codici IBM.
La stessa procedura disegna finestre di scritture durante la costruzione
progressiva, mantenendone gli stati intermedi. La descrizione deriva dalle
precedenti tabelle: la riscrittura cambia la rappresentazione, non la
provenienza della mappa. `docs/maze.txt` resta una trascrizione Unicode
leggibile per gli occhi; il programma non la carica.

Lo schermo storico ha 80 colonne di testo. Un corridoio logico occupa due
colonne (glifo del muro o dell'attore, poi spazio), perciò il gioco ragiona su
40 colonne. Le righe del labirinto sono numerate 1–24. In C gli array partono
da zero: la riga logica 1 si trova all'indice 0. La riga 25 è la barra di stato
con contatore, vite, titolo e messaggi di fine partita; non fa parte del
labirinto.

In modalità Normale `game_init()` costruisce la mappa nello stato mutabile `Game`.
In Remix `game_init_remix()` genera invece una griglia con una ricerca casuale
in profondità dentro un bordo rettangolare. Visita tutti i nodi, apre i
collegamenti tra i nodi e allarga alcuni passaggi. Il bordo ha due aperture
laterali in righe variabili: `tunnel_left_row` e `tunnel_right_row` collegano
i due ingressi, e sia Pac-Man sia i nemici ricompaiono dentro lo stesso
rettangolo. La ricerca di verifica controlla che tutte le caselle percorribili
siano connesse. Al completamento viene creato un nuovo schema e il contatore
del livello aumenta. Da
quel momento la schermata di gioco è anche la rappresentazione della mappa:
muovere un attore sostituisce il carattere nella sua casella e salva in
`Actor.under` il carattere che dovrà essere ripristinato quando l'attore si
sposta. Per questo la schermata non è solo un'immagine disegnata sopra una
mappa separata. Le funzioni `tile()` e `attr()` traducono una coordinata logica
(riga, colonna) nella cella di schermo `(riga - 1, colonna * 2)`.

Gli attributi sono i byte colore della modalità testo DOS. `7` è il colore
normale; `138` (`0x8A`) è il pallino speciale lampeggiante. `put()` converte i
valori colore compatti usati dalla logica nei bit DOS: il bit 7 abilita il
lampeggio e i quattro bit bassi selezionano il colore in primo piano.

## Stato e turni di gioco

`src/game.h` definisce lo stato, `src/game.c` contiene le regole.

| Campo | Significato |
| --- | --- |
| `Game.ch`, `Game.attr` | Caratteri e attributi della schermata corrente (25 × 80). |
| `Game.player`, `Game.ghost[4]` | Posizione e stato del giocatore e dei quattro avversari. |
| `Game.lives`, `Game.dots` | Vite rimaste e pallini non ancora raccolti. |
| `Game.remix`, `Game.level` | Modalità e livello corrente; zero indica la mappa classica. |
| `Game.tunnel_left_row`, `Game.tunnel_right_row` | Righe delle uscite laterali dinamiche nel Remix. |
| `Game.requested_dy/dx` | Direzione chiesta dall'utente; può restare in attesa finché il passaggio si apre. |
| `Game.rng` | Stato a 24 bit del generatore casuale compatibile con il runtime BASIC. |
| `Game.aggression`, `Game.ticks` | Stato usato dalle decisioni degli avversari. |
| `Game.sound`, `Game.events` | Ultimo effetto e lista degli effetti prodotti durante il turno corrente. |
| `Game.ended` | `0` partita attiva, `1` vittoria, `2` partita persa. |
| `Game.effect_ch/attr`, `Game.effect` | Copia della schermata per mostrare temporaneamente morte o cattura. |

Un `Actor` ha coordinate logiche, direzione attuale `(dy, dx)`, il carattere
coperto (`under`), colore e timer. La direzione richiesta dal giocatore è
separata dalla direzione attuale: premere una freccia prima di un angolo
permette di svoltare appena il movimento diventa possibile.

Per ogni scatto temporale il ciclo principale chiama prima
`game_player_step()` e poi `game_ghost_step()`. Il giocatore può camminare,
fermarsi davanti a un muro, attraversare il tunnel, raccogliere un pallino,
attivare un pallino speciale, essere catturato o mangiare un avversario
vulnerabile. Il turno degli avversari aggiorna ciascuno dei quattro attori;
le decisioni casuali e i loro tentativi sono ordinati per riprodurre le
chiamate del programma BASIC, non per implementare un'intelligenza artificiale
moderna.

`game_restart()` ricostruisce la stessa mappa, come fa il programma originale.
Conserva il byte basso del generatore casuale fra le partite e anche alcuni
caratteri residui della barra inferiore. Il gioco classico non ha un generatore
di livelli: la mappa fissa contiene 468 pallini, di cui 10 speciali.
La modalità Remix è un'aggiunta del port e usa un generatore separato; non altera
la mappa né la progressione della modalità Normale.

## Timing e presentazione

`src/timing.c` decide quando avanza la simulazione, senza legarla al numero di
fotogrammi o alla velocità della CPU. Il valore inserito all'avvio è mappato
su `300 + velocità / 20` millisecondi per turno (entro l'intervallo 300–1800
ms); quindi un numero più alto rallenta. Il rendering SDL può aggiornarsi
molte volte fra due turni. Pausa e blocco della finestra non accumulano mosse
da recuperare tutte insieme.

`src/presentation.c` contiene i fotogrammi dell'introduzione e la sequenza di
scritture che costruisce la mappa. Non decide i movimenti. `src/main.c` legge
input, coordina introduzione/gioco/effetti, aggiorna SDL e chiama la logica.
Il renderer converte la schermata testuale in un'immagine 640 × 200: 80 × 25
caratteri, ognuno largo e alto 8 pixel. `src/font.h` contiene il font bitmap
incorporato. Le dimensioni della finestra, il riempimento dello schermo e i
controlli cambiano per PC e PS Vita, ma stato e regole sono condivisi.

## Suoni

`src/audio.c` conserva un parser BASIC (`PLAY`) per compatibilità, ma gli
effetti del gioco usano ora partiture di note, ottave, lunghezze e pause,
con scale generate tramite cicli. Le stringhe PLAY storiche non sono più
incorporate negli effetti. Un effetto diventa una lista di toni con frequenza e
durata, espressa in tick del timer PIT del PC. `src/speaker.c` converte quei tick in
campioni audio e applica il modello di risposta scelto per il port. Il
callback SDL consuma la coda di campioni; non avvia il gioco né decide quando
si muovono gli attori.

Il modello riproduce il segnale digitale del timer e un filtraggio acustico
stimato. Non può garantire che un altoparlante moderno o uno specifico PC del
1982 producano lo stesso timbro. Le motivazioni, le misure e i limiti sono
descritti più a fondo in `docs/reverse-engineering.md` e nei rapporti audio.

## Dove cercare

| File | Responsabilità |
| --- | --- |
| `src/main.c` | Avvio SDL, input, ciclo principale, rendering e coordinamento audio. |
| `src/game.h`, `src/game.c` | Strutture e regole della partita. |
| `src/maze.h`, `src/build.h` | Costruttore della mappa e istruzioni di disegno condivise con l'animazione. |
| `src/presentation.h`, `src/presentation.c` | Schermate iniziali e costruzione animata del labirinto. |
| `src/timing.h`, `src/timing.c` | Scadenze dei turni, pausa e velocità. |
| `src/audio.h`, `src/audio.c` | Partiture strutturate, parser compatibile `PLAY`/`SOUND` e sequenze di toni. |
| `src/speaker.h`, `src/speaker.c` | Timer PIT, coda audio e modello del PC speaker. |
| `src/font.h` | Glifi bitmap incorporati, indicizzati dai codici in `ch`. |
| `docs/maze.txt` | Vista Unicode della mappa per lettura umana; non è caricata dal gioco. |
| `docs/disassembly.txt`, `docs/reverse-engineering.md` | Riferimenti al binario DOS e spiegazione del reversing. |

Gli offset esadecimali citati nei documenti indicano indirizzi nel vecchio
eseguibile DOS, utili per mostrare da dove è stata ricostruita una regola.
Non sono offset del sorgente C e non vengono consultati a runtime. I test
confrontano il port con schermate e stati estratti dall'originale; vedere i
comandi e i limiti in `README.md`.
