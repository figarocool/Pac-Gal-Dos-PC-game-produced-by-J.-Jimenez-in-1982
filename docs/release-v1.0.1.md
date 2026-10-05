# PAC-GAL 1982 — v1.0.1

Updated C recreation with Normal and endless Remix modes. The maze now uses
drawing instructions and sound effects use structured musical scores.
Construction states and tested audio samples match the previous implementation.
These changes preserve the historical content and its documented provenance.

Downloads:

- Linux x86_64: `PAC-GAL-Linux-x86_64.tar.gz` (requires SDL2).
- Windows x86_64: `PAC-GAL-Windows-x86_64.zip` (includes SDL2.dll).
- DOS / DOSBox: `PAC-GAL-DOSBox.zip` (requires a separately installed DPMI
  host, VGA/VESA and Sound Blaster emulation).
- PS Vita 1.17: `PAC-GAL-PSVita.vpk`.
- PSP / Adrenaline 1.01: `PAC-GAL-PSP-EBOOT.PBP`.
- `SHA256SUMS.txt`: package checksums, with paths relative to the repository.

All five builds have been updated. Linux tests and DOSBox startup/self-test
pass; Windows self-test passes under Wine. Vita/PSP transfers were verified
by reading back the installed files, and gameplay was reported working.
The extended DOS comparison has three preexisting end/replay text differences;
the before/after outputs for those cases are identical. Details are in
`docs/riscrittura-rappresentazione.md`.

The packages include the applicable license notices. The historical 1982
executable and a third-party DPMI host are not bundled. The GPL license of
the C implementation does not relicense the historical game.

---

Ricostruzione C aggiornata con modalità Normale e Remix a livelli infiniti.
La mappa usa istruzioni di disegno e gli effetti sonori usano partiture
strutturate. Gli stati della costruzione e i campioni audio verificati
coincidono con la precedente implementazione. La provenienza dei contenuti
storici resta documentata.

Sono aggiornate tutte e cinque le build: Linux, Windows, DOS, Vita 1.17 e
PSP/Adrenaline 1.01. I test Linux e l'avvio/autotest DOSBox passano; Windows
supera l'autotest sotto Wine. I file installati su Vita e in Adrenaline sono
stati verificati mediante rilettura e il gioco è stato segnalato funzionante.
Tre differenze preesistenti nei testi di fine partita/replay del confronto
DOS sono documentate nel rapporto tecnico e risultano invariate.

I pacchetti includono le relative licenze. L'eseguibile storico del 1982 e
un host DPMI di terze parti non sono inclusi. La GPL dell'implementazione C
non estende la propria licenza al gioco storico.
