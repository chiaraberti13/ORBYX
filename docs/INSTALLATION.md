# Installazione guidata Ubuntu / Guided Ubuntu setup

## Italiano

Dopo aver aggiornato il clone con `git pull`, dal terminale del repository:

```bash
bash install.sh
bash start.sh
```

Non occorre specificare cartelle, configurare PATH o cercare gli eseguibili. Lo script ricava automaticamente il progetto e installa in uno spazio dedicato dell'utente. `bash orbyx.sh --help` avvia la CLI installata. Il desktop permette di scegliere dati, profilo e destinazione tramite selettori.

### Errore risolto

Il blocco APT/dpkg dell'esempio era occupato da `unattended-upgrades`. Va lasciato terminare: non cancellare lock e non uccidere il processo. Installare soltanto CMake non compila ORBYX e non installa Ninja, compilatore, librerie OpenCV/JsonCpp e Qt. I successivi tentativi di avviare i binari falliscono perché configurazione e compilazione non erano state eseguite con successo.

L'installer richiede solo i privilegi APT necessari, attende i lock tramite il timeout di dpkg e ritenta i lock degli indici. L'attesa è limitata (circa15minuti di contesa); `ORBYX_APT_LOCK_TIMEOUT` permette di modificarla fino a3600secondi. Non esegue upgrade generale, autoremove, rimozione lock o interruzione di processi. Gli errori di rete/pacchetti diversi dalla contesa non vengono ignorati.

Poi esegue in sequenza configurazione Release/Qt, compilazione con due worker, CTest, installazione e smoke degli eseguibili installati. Una fase fallita interrompe la procedura e impedisce di dichiarare completata l'installazione. I launcher controllano la conferma finale e segnalano di ripetere l'installer se l'installazione è incompleta. Un lock locale evita installazioni concorrenti e sostituzione dell'app mentre è avviata tramite il launcher.

### Log

La cartella **logs** contiene file separati per installazione, desktop e CLI. Ogni esecuzione ha un log completo e un `.errors.log`, con data UTC e processo. Vengono conservati stderr, fase/codice di uscita e contesto anche quando un tool stampa l'errore su stdout. I log generati sono ignorati da git e non vengono inviati automaticamente. Le credenziali sudo non sono registrate.

`--no-deps` è disponibile per ambienti di sviluppo/CI con dipendenze già predisposte. `--lang en` seleziona i messaggi inglesi; la lingua predefinita dei nuovi script è italiano. Questi script di installazione automatica sono destinati a Ubuntu con APT, non un nuovo installer Fedora/macOS.

### Verifica

Test standard-library con comandi APT/build simulati controllano contesa e retry, dipendenze complete, stop dopo errore di compilazione/test, registrazione di errori stdout, avvio incompleto e rifiuto di cartelle log symlink. Una CI Ubuntu26.04 esegue inoltre il vero installer come utente normale, installa realmente le dipendenze e avvia CLI/GUI installate. Esiti osservati in `PROGRESS.md`; la sola configurazione del workflow non è un pass.

## English

Run `bash install.sh`, then `bash start.sh`, from the repository terminal; no manual paths or PATH edits are needed. Use `bash orbyx.sh --help` for CLI. Setup installs complete Ubuntu dependencies, waits for bounded package-manager lock contention without deleting locks or killing updates, then configures/builds/tests/installs for the current user. Only APT uses sudo. General upgrades/autoremove are not performed.

Failed phases stop installation. Launchers require its final success marker and share a local lock with setup. Per-session complete/error logs remain in logs, ignored by git with no automatic upload. Both stdout-only errors and stderr are retained. Stubbed control-flow regressions are distinct from the actual Ubuntu26.04 installation/installed-CLI/GUI workflow; observed remote results are recorded in PROGRESS.md.
