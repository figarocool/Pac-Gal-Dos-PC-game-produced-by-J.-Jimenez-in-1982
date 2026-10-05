# Riscrittura della rappresentazione — 5 ottobre 2026

La prova mantiene il disegno, la musica e le regole della ricostruzione,
sostituendo le rappresentazioni usate per mappa, costruzione ed effetti.
Sono stati ricompilati i port Linux, Windows, DOS, Vita 1.17 e PSP 1.01.
I pacchetti aggiornati in `dist/` sostituiscono i precedenti nella release.
Le versioni Vita e PSP sono state installate come gioco nativo e in
Adrenaline: i file riletti via FTP risultano identici ai binari compilati.

## Modifiche

- `src/build.h`: 1965 registrazioni di scritture per cella sostituite da
  466 istruzioni di tratti con motivi ripetuti e didascalie. Una finestra
  sull'indice delle scritture conserva anche gli stati intermedi.
- `src/maze.h`: le matrici `original_screen` e `original_attr` sostituite
  dal costruttore procedurale condiviso con l'animazione.
- `src/audio.c`: cinque stringhe PLAY degli effetti sostituite da partiture
  strutturate; la cattura usa tre scale cromatiche generate da un ciclo.
  Frequenze, arrotondamenti, pause, articolazione e stato persistente restano
  gli stessi. Il parser PLAY resta disponibile, ma gli effetti non lo usano.
- `tools/extract.py`: le vecchie tabelle di riferimento vengono esportate
  in `/tmp/pacgal-extraction`, oppure nella directory scelta con
  `--output-dir`. Lo strumento non sovrascrive più i sorgenti procedurali.
- `Makefile`: il test differenziale collega anche il modulo traduzioni e
  SDL; aggiunto `make representation-dump` per confrontare le revisioni.

Il metodo adottato trasforma le rappresentazioni precedenti ricavate
dall'analisi dell'eseguibile. Grafica e musica mantengono quella provenienza.
Il font DOSBox con la sua licenza, i testi visibili, i crediti e i materiali
storici di analisi sono conservati. Le verifiche riguardano le sequenze
indicate in questo rapporto e non costituiscono un confronto esaustivo
di tutti i byte o una valutazione dei diritti sui contenuti.

## Verifiche

`make test` passa: regole, mappa, temporizzazione, effetti, speaker e avvio
SDL senza schermo. `make differential` passa: schermata, colori, 64 passi
giocatore/avversari, contatori, stato avversari e PRNG coincidono con le
acquisizioni DOS esistenti.

`tests/representation.c` è stato compilato contro i sorgenti precedenti e
contro quelli nuovi. I due file prodotti sono identici, verificati con
`cmp`; ciascuno misura 72389944 byte e ha SHA-256:

`4c5ce16e1a9c9e69410843245a438f70a79e534e956b7a05c36ae29aa59b2291`.

Il confronto copre:

- Tutti i 1966 stati della costruzione, incluso lo schermo iniziale, più
  aggiornamenti a blocchi, limite finale e conservazione di celle preesistenti.
- 441 casi di effetti singoli, combinando sette eventi, tre articolazioni,
  sette ottave iniziali e coda vuota/quasi piena/piena. Sono confrontati
  esito, stato musicale finale e toni, anche quando lo spazio finisce.
- 21 sequenze persistenti di sette effetti e relativi campioni PCM a
  22050, 44100 e 48000 Hz, identici byte per byte fra le due implementazioni.

Le schermate BMP prima/dopo (`--seed 1982 --frames 3`) coincidono:

| Modalità | SHA-256 comune |
| --- | --- |
| Normale (`--start`) | `6c944a4043ee600e877619af07bc26c61570f62a263abe4620551790faed5233` |
| Remix (`--remix`) | `0f0602198c3865ebb106caf53969df6d1bec3540164907804aa9c2a0f9a54656` |

Nel nuovo `pac-gal` non sono presenti le quattro stringhe musicali
riscontrate nell'EXE del 1982 dall'audit iniziale. Titolo e frase di vittoria
restano intenzionalmente uguali per preservare la schermata.

La compilazione è stata verificata su tutte e cinque le piattaforme. VPK,
metadati, immagini LiveArea e struttura PBP sono stati verificati. Su DOSBox
il nuovo eseguibile supera `--self-test` e produce una schermata di gioco.
La build Windows supera `--self-test` sotto Wine; non è stata provata su hardware Windows.
È stato segnalato il corretto funzionamento delle versioni installate su
Vita e in Adrenaline. I confronti non misurano la risposta di un altoparlante
fisico né ogni possibile esecuzione del gioco.

Il confronto esteso con `tools/deep_test.py --game-only` ha 168 casi su 171
superati (comprende gli esiti delle 120 schermate introduttive già presenti
nel rapporto). Tre confronti DOS relativi al testo di fine partita/replay
non coincidono: `ghost-collision-False-1`, `last-life` e `replay`. I risultati
nativi di questi tre casi sono identici prima e dopo la riscrittura, come
registrato in `replay-comparison.json`. Sono differenze preesistenti; il
comportamento del gioco non è stato cambiato per eliminarle dal rapporto.

## Ripetere il confronto locale

Una copia temporanea dei vecchi sorgenti e dell'eseguibile si trova in
`/tmp/pacgal-rewrite-before/`. I riferimenti temporanei possono scomparire
dopo una pulizia di `/tmp`.

```sh
make representation-dump
cmp /tmp/pacgal-rewrite-before/observable.bin /tmp/pacgal-representation.bin
./avvia.sh
```

Per guardare la stessa configurazione nelle due versioni, eseguirle una alla
volta con `--start --seed 1982`; il vecchio binario è
`/tmp/pacgal-rewrite-before/pac-gal`, quello nuovo è `./pac-gal`.
