# Analisi dell'eseguibile originale

Input: `Pac-Gal_DOS_EN.zip` in Scaricati, contenente un solo MZ di 39296 byte.
SHA-256: `624989e20e11e0559401e1f21a8cfdd86b5d4dfde1272fcebb459b372b02b49f`.
Non contiene sorgenti né asset separati. Gli errori e le routine incorporati
identificano un runtime di BASIC compilato. Titolo, autore e data sono stringhe
interne all'eseguibile.

Per la struttura dei sorgenti C e la rappresentazione interna della mappa,
vedere [la guida al codice](guida-al-codice.md). Questa pagina descrive invece
come le regole sono state ricavate dal binario DOS e quali limiti restano.

## Metodo

`ndisasm -b 16 -e 5662` produce `disassembly.txt`. Gli offset seguenti sono
relativi a questa disassemblazione, non offset nel file. Il programma genera
il labirinto con PRINT, LOCATE, CHR$ e STRING$. `tools/extract.py` valuta solo
le istruzioni di costruzione del labirinto; non è un interprete utilizzato dal
gioco nativo. Originariamente generava le tabelle nei sorgenti. Dopo la
riscrittura procedurale del 5 ottobre 2026 esporta i riferimenti separatamente
in `/tmp/pacgal-extraction` (o nella directory scelta con `--output-dir`),
senza sovrascrivere i nuovi costruttori.

`tools/dos_oracle.py` modifica una copia temporanea del MZ, salta la richiesta
di velocità, conserva l'inizializzazione BASIC e intercetta la fine della
costruzione del labirinto. Scrive i 4000 byte di B800:0000 e dati di stato DOS.
Rimuove le relocation che cadono nelle istruzioni sostituite. Il confronto
con il C verifica caratteri e attributi delle 25 righe, contatori e,
indirettamente, movimento/PRNG tramite le posizioni degli avversari.

La seconda cattura esegue quattro serie di 16 passi (sinistra, alto, destra,
basso), ciascuno seguito dal movimento dei quattro avversari. Il seme BASIC
RANDOMIZE è fissato a 1982; le RETURN BASIC vengono sostituite con ritorni al
driver temporaneo. Suoni e letture del labirinto restano quelli del DOS.

## Corrispondenze

| Offset | Funzione |
| --- | --- |
| 0x022E–0x02E3 | Definizione delle stringhe di celle e muri |
| 0x02FC–0x1CC4 | Costruzione del labirinto |
| 0x1CC5–0x1E1A | Dieci pallini con attributo 0x8A |
| 0x1E1E–0x1FCC | Tre vite, 468 pallini, posizioni/direzioni |
| 0x2026–0x2076 | Eventi direzione tastiera |
| 0x20E1–0x2416 | Movimento giocatore, tunnel e consumo |
| 0x2417–0x2647 | Collisione letale e riposizionamento |
| 0x2648–0x2872 | Avversario mangiato, recupero vita |
| 0x2875–0x2D0B | Movimento dei quattro avversari |
| 0x2D35–0x2E73 | Vittoria, replay e uscita |

Le coordinate logiche sono di riga 1–24 e colonna 0–39. Il carattere su cui
si muove un attore si trova alla colonna video `2*colonna+1` (LOCATE conta da
uno). I muri non sono ricostruiti con un labirinto generico: sono esattamente
le sequenze di caratteri stampate dall'originale.

- Pallino: CHR$(249); giocatore: CHR$(1); avversari: CHR$(3)–CHR$(6).
- Speciale: stesso CHR$(249), attributo video 0x8A (verde chiaro lampeggiante).
- Tunnel: riga 12, ingresso contro CHR$(196), destinazione `39-colonna`.
- Giocatore iniziale: riga 19, colonna 19; avversari riga 14, colonne 18–21.
- Dopo morte: avversari alle colonne 19–22; conservano la direzione corrente.
- Timer speciale: arrotondamento BASIC di `(pallini/5 + 20) / vite²`.
- Il carattere sottostante agli avversari viene conservato/restaurato. L'uso
  della memoria video come stato, incluse le variazioni di attributo dopo il
  ripristino, è riprodotto nella struttura C `Game`.
- Il contatore originale di difficoltà parte da zero e viene moltiplicato
  per 0.5 in certi rami. Non è stato sostituito con una difficoltà inventata.
- Dopo ogni 180 secondi il ciclo principale esegue un turno aggiuntivo degli
  avversari per movimento del giocatore; morte e avversario mangiato azzerano
  il riferimento temporale.

## Casuale BASIC

La routine del runtime al suo offset 0x09C1 usa:

```
state = (state * 214013 + 1744579) & 0xFFFFFF
RND = state / 16777216
```

RANDOMIZE scrive il seme nei 16 bit superiori dello stato lasciando il byte
inferiore corrente. Alla prima partita è 5; al replay conserva il byte della
sequenza precedente. La conversione a intero di RND seleziona asse e segno.
Il seme automatico segue la conversione TIME$ originale: ore×360 + minuti×60
+ secondi (il fattore 360 è presente nel programma storico).
Il C conserva anche l'ordine delle chiamate: il tentativo di girare consuma
un numero casuale solo dopo il fallimento del movimento diritto.

Il font compatibile IBM è estratto dal BIOS incorporato in DOSBox e salvato
come array C. È coperto dalla GPL di DOSBox; il gioco non lo carica da file.

## Validazione e limiti

`make test`: regole, raggiungibilità di tutti i 468 pallini, ordine dei suoni,
20 semi simulati, timing a 30/60/144 FPS e smoke test SDL. I controlli sulle
regole passano anche con AddressSanitizer/UndefinedBehaviorSanitizer.
`make differential`: tutte le 25 righe dopo 64 turni, contatori, otto campi
per avversario e PRNG. `make deep-test`: 171 confronti, inclusi tutti i 120
fotogrammi dell'introduzione, costruzione, collisioni nei due sensi, vite,
pallini sottostanti, animazioni, ultima vita, vittoria e replay.
Non è una prova esaustiva di tutte le partite possibili.

L'oracolo chiama direttamente le routine BASIC dopo aver sostituito RETURN
con RET: è un harness di sviluppo, non una modalità del gioco finale.
Per la logica di cattura sostituisce con NOP la chiamata PLAY a 0x268F:
la forma compilata X con puntatore a variabile causa «Illegal function call»
nel percorso di chiamata diretta. L'audio è verificato separatamente usando
il parser originale, espandendo la variabile della scala ascendente presente
nel programma (cc#dd#eff#gg#aa#b) nelle tre ottave/lunghezze originali.
Questa prova non verifica autonomamente il percorso X compilato.

Nel test di replay si fornisce y al codice di assegnazione e confronto
originale, si salta solo l'attesa con orologio fissato, e si lascia eseguire
il salto reale a 0x006D e tutta la ricostruzione del labirinto. Si confronta
nuovamente l'intera schermata e lo stato: la mappa è la stessa, PRNG e residui
della riga 25 sono conservati esattamente.

## Audio

`tools/audio_oracle.py` intercetta l'ingresso alla coda SOUND del runtime
(offset runtime 0x1EEE), conservando le istruzioni originali in un trampoline.
Registra CX (Hz) e DX (durata in tick di 2048 cicli PIT). I dati sono in
`audio-oracle.json`; `tests/audio-fixtures.h` conserva i riferimenti per i
test C. Si verificano sette effetti singoli e una sequenza con stato PLAY
persistente. MB significa esecuzione in background, non una nota B;
l'ottava O4 usa C a 1047 Hz. SOUND usa 4000 Hz per il muro e 150 Hz sul vuoto.
La nota normale dura 7/8, con la pausa residua e arrotondamento intero BASIC.

`src/audio.c` genera PCM con il divisore intero del timer PIT in modalità 3,
compreso il semiperiodo alto più lungo dei divisori dispari. La coda di toni conserva l'ordine degli effetti invece di cancellare il
suono precedente. `src/speaker.c` integra il PIT con fase intera nel callback
SDL, emettendo PCM signed-16 a 48 kHz stereo su Vita e 44,1 kHz mono su PC.
Non alloca memoria e non calcola floor/double per ogni campione. I test
confrontano i campioni con il sintetizzatore di riferimento, su blocchi di
137 campioni e nelle due frequenze; differenza massima float 1,49e-8.
Il timbro acustico di una cassa PC storica non è riproducibile identicamente
su qualunque altoparlante moderno.

## Timing nativo e misura del ciclo DOS

`tools/benchmark_timing.py` esegue quattro chiamate alla routine originale
0x2E73 con velocità 6000 e misura i tick BIOS prima e dopo. Le misure sono
conservate in `timing-benchmark.json`: 329,6 / 96,1 / 41,2 ms per attesa a
300 / 1000 / 3000 cycles DOSBox. È la sola attesa BASIC; non include rendering,
suoni e logica. La risoluzione del timer BIOS è circa 55 ms: sulle quattro
chiamate il margine di quantizzazione è circa 14 ms per chiamata. Questi
profili non dichiarano un'equivalenza esatta con un IBM PC a una certa MHz.

Il ritmo nativo usa `300 + velocità/20` ms. Dal video YouTube
`2ZmQ5DXRr_Q` è stato misurato l'evento audio associato a ogni turno: le
cadenze mediane sono 350 ms a 14–24 s e a 24–34 s, 350 ms a 34–44 s.
Il campo iniziale del video mostra velocità 1000. Il port ora precompila
1000 e imposta 350 ms per turno; 6000 dà 600 ms. Le scadenze SDL restano
indipendenti dal rendering. Dopo un blocco lungo si esegue un solo turno,
senza recuperare una raffica di mosse. Il tempo attivo esclude le pause,
anche per l'accelerazione degli avversari ogni tre minuti.

Le animazioni al profilo 300 cycles sono misurate con il timer BIOS:
prima passata dell'introduzione circa 165 ms, introduzione completa 934 ms,
morte 330 ms e cattura 165 ms. Intro più costruzione misura circa 2142 ms;
la costruzione nativa distribuisce le scritture originali su 1208 ms.
La quantizzazione BIOS è circa 55 ms. Dati in `animation-timing.json`.
I fotogrammi e l'ordine delle scritture sono recuperati dal codice; il loro
ritmo su altre CPU storiche può differire.

## Modello acustico dello speaker (01.03)

Il percorso di riproduzione usa `speaker_pc_init`; `speaker_init` conserva
il timer grezzo per il confronto indipendente con i dati del parser DOS.
Il gate disabilitato è il livello basso, non il punto medio dell'onda. Le
nuove frequenze su un timer abilitato si caricano a un confine di semiperiodo;
le pause BASIC usano il divisore 2, integrato come segnale ultrasonico.
La risposta acustica usa filtri Butterworth di ordine 3, HP 120 Hz e LP
4300 Hz, come riferimento dal modello pubblico di DOSBox Staging:
https://github.com/dosbox-staging/dosbox-staging/blob/main/src/hardware/audio/pcspeaker_impulse.cpp
Il calcolo dei filtri è originale, con un polo reale e una coppia Q=1,
coefficienti calcolati una sola volta. Nessuna allocazione nel callback.

Questa risposta è una scelta di simulazione, non una misura del circuito
e dell'altoparlante di uno specifico IBM 5150. La registrazione DOSBox
`original-empty-speaker.wav` conferma l'importanza di fase e gate, ma
non è una registrazione dell'hardware del 1982. La temporizzazione nominale
dei toni viene conservata; la fase IRQ iniziale del DOS non è deterministica
e non si può affermare identità PCM con quella singola registrazione.

### Verifica rispetto ad altri progetti

Confrontati i sorgenti ufficiali DOSBox Staging (pcspeaker_impulse.cpp e
pcspeaker_pit.cpp), ScummVM (audio/softsynth/pcspk.cpp e driver Player_V2)
e 86Box (src/sound/snd_speaker.c). Confermano l'approccio timer/onda quadra
e filtraggio, ma hanno modelli e dettaglio differenti. Il nostro sintetizzatore
non è una copia completa del modello sinc/impulse di DOSBox Staging: integra
il pin su ciascun campione. Non emula tutti i modi PIT o le tecniche PCM/PWM
di altri giochi; il percorso PAC-GAL usa SOUND/PLAY in modo 3.

`python3 tools/check_speaker_response.py` confronta i coefficienti effettivi
del C con filtri Butterworth di ordine 3 progettati indipendentemente da
SciPy. Risultati in `speaker-response-check.json`: entrambi i sample rate
passano, tagli a -3,010 dB, massimo errore della risposta complessa <0,000223.
Questa verifica riguarda il filtro matematico, non l'identità acustica con
uno specifico speaker del 1982. Il confronto PCM in Vita3K verifica il trasporto
audio del nostro modello; non sostituisce quella misura fisica.

## Correzione dopo il riferimento video DOSBox (01.04)

Il runtime 1F53..1F5A scrive 2048 alla porta 40h senza un nuovo control word.
DOSBox conserva l'IRQ0 già programmato, aggiornando la frequenza degli IRQ
successivi. Alla fine la porta 61h viene disabilitata e il timer 0 viene
riportato a 65536 (1B88..1BA4), ancora senza cancellare l'evento in attesa.
Il modello ora conserva `irq_until` e modifica solo `irq_period`. Ogni
tono termina dopo il numero di IRQ indicato dal runtime, anziché sempre
dopo ticks*2048 cicli dall'avvio. Il primo SOUND breve può quindi essere
molto più lungo a seconda della fase BIOS, coerentemente con il video.
Questo corregge la precedente limitazione sulla fase IRQ iniziale.

`speaker_video_init` è ora il percorso predefinito: HP del primo ordine a
3 Hz stimato dalle code della registrazione, LP4300 di ordine 3 conservato.
Il modello HP120 descritto sopra resta disponibile in `speaker_pc_init`,
ma non è il timbro della registrazione fornita. La stima dal video è una
scelta di riproduzione del riferimento, non una misura di un altoparlante IBM.
Non è dichiarata identità PCM con il filmato YouTube compresso.

## Ritmo misurato nel video 2ZmQ5DXRr_Q

La traccia audio non compressa del video contiene un evento di gioco ogni
turno. Sono stati misurati i picchi RMS a 10 ms: 29 intervalli tra 14 e 24 s,
29 tra 24 e 34 s, 28 tra 34 e 44 s; mediane rispettivamente 350, 350 e
350 ms. L'intervallo è dunque coerente con 350 ms/turno, vicino al ritmo
DOS di riferimento e più lento dei 175 ms che il mapping precedente dava
quando si inseriva 1000.

Pac-Man si sposta di una casella per turno. Il gioco aggiorna ciascuno dei
quattro fantasmi nello stesso turno; quando si mangia un fantasma, la scena
rimane ferma per circa 165 ms, poi entrambi riprendono al ritmo del turno.
La traccia audio permette di misurare il ritmo dei turni, ma non ricostruisce
le coordinate individuali dei fantasmi con accuratezza fotogramma per
fotogramma. Il nuovo valore iniziale è calibrato sul passo del video.
