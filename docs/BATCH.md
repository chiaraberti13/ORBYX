# Batch and verified resume / Batch e ripresa verificata

The optional batch runner requires Python 3.10+ on POSIX Linux/macOS. It runs jobs sequentially through the independent CLI; it is not yet integrated into the desktop queue. It never invokes a shell or downloads data. macOS has not been tested.

Il batch opzionale richiede Python 3.10+ su POSIX Linux/macOS. Esegue i lavori in sequenza attraverso la CLI indipendente; la coda non è ancora integrata nel desktop. Non esegue shell né scarica dati. macOS non è stato verificato.

## Manifest

Save `batch.json` outside the input/output directories. Relative paths are resolved against the manifest directory. Each job selects its reusable calibration profile through `config`. Input and output directories cannot overlap, even across jobs.

Salvare `batch.json` fuori dalle cartelle di input/output. I percorsi relativi sono risolti rispetto alla cartella del manifest. Ogni lavoro sceglie il proprio profilo riutilizzabile tramite `config`. Input e output non possono sovrapporsi, anche fra lavori diversi.

```json
{
  "jobs": [
    {
      "id": "acquisition-001",
      "satellite": "L3",
      "input": "data/acquisition-001",
      "output": "results/acquisition-001",
      "config": "profiles/calibration.json",
      "mode": "generate",
      "cpu": true
    }
  ]
}
```

```bash
python3 scripts/orbyx_batch.py batch.json --orbyx ./build/orbyx --lang it
python3 scripts/orbyx_batch.py batch.json --orbyx ./build/orbyx --resume
```

Installed runner / comando installato: `orbyx-batch batch.json --orbyx /usr/local/bin/orbyx`.

## State and failure / Stato e fallimenti

Default state is `batch.state.json`; `--state PATH` selects another file. The runner uses an advisory non-blocking lock, atomic state replacement and directory fsync. State and lock must be outside job directories and cannot alias inputs, manifests, profiles or the engine. An existing state requires `--resume`; choose a new state file for an independent run.

Lo stato predefinito è `batch.state.json`; `--state PERCORSO` seleziona un altro file. Il runner usa un lock consultivo non bloccante, sostituzione atomica e fsync della cartella. Stato e lock devono restare fuori dalle cartelle dei lavori e non possono coincidere con dati, manifest, profili o motore. Uno stato esistente richiede `--resume`; per una nuova esecuzione indipendente scegliere un altro stato.

Resume skips a completed job only after comparing its normalized parameters, the content digests of input/config/engine and the complete output digest. Changed input/output or a changed binary prevents skipping. Existing products are never overwritten: choose a new destination after a genuine change. A failed verification preserves the previous successful provenance and records the failed attempt separately. Restoring the original bytes allows verification again.

La ripresa salta un lavoro completato soltanto dopo aver confrontato parametri normalizzati, digest di input/profilo/motore e digest completo dell'output. Modifiche ai dati o al binario impediscono di saltarlo. I prodotti esistenti non vengono sovrascritti: dopo una modifica effettiva scegliere una nuova destinazione. Una verifica fallita preserva la provenienza del completamento precedente e registra il tentativo separatamente. Ripristinando i byte originali la verifica può tornare valida.

Errors do not block later independent jobs. Exit codes: `0` all completed/verified, `1` at least one failed job, `2` usage/schema/control-path failure, `130` interrupted. Ctrl+C terminates the active process and records interruption. A hard crash can leave hidden staging directories; these are not complete products and are not automatically resumed. Do not modify acquisitions while processing: per-file snapshots and before/after fingerprints are not a transactional snapshot of a live producer.

Un errore non blocca i lavori indipendenti successivi. Codici: `0` completati/verificati, `1` almeno un lavoro fallito, `2` errore di uso/schema/percorsi di controllo, `130` interruzione. Ctrl+C termina il processo attivo e registra l'interruzione. Un arresto improvviso può lasciare cartelle temporanee nascoste, che non sono prodotti completi e non vengono riprese automaticamente. Non modificare l'acquisizione durante l'elaborazione: snapshot dei file e digest prima/dopo non sono uno snapshot transazionale di un produttore attivo.
