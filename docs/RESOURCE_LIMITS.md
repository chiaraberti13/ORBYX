# Limiti delle risorse / Resource limits

## Italiano

Il motore controlla la firma PNG e il blocco IHDR **prima della decodifica**:
file massimo 1 GiB; canali grayscale a 8 o 16 bit senza segno; dimensioni positive
non superiori a 100.000 pixel per asse; altezza superiore a 3.606; massimo
150 milioni di pixel per canale. Il profilo JSON è limitato a 1 MiB.
Questi controlli rifiutano intestazioni palesemente eccessive e formati inattesi,
ma **non costituiscono un limite globale di memoria o una sandbox**.

Un solo canale da 150 Mpx a 16 bit occupa circa 300 MB prima dei buffer temporanei.
Sei canali, immagini allineate, canvas di fusione, copie CPU/GPU e buffer ECC possono
richiedere diversi GB. La calibrazione usa due calcoli ECC concorrenti per prodotto.
Non esiste ancora una pianificazione basata sulla RAM disponibile; non sono
implementate elaborazione a tasselli, quote per processo o isolamento del decoder.
I file devono provenire da acquisizioni attendibili. Per input non attendibili
eseguire il programma come utente senza privilegi con limiti di memoria/processo
esterni; nessun limite interno garantisce resistenza a ogni vulnerabilità del codec.

L'output viene scritto in una cartella temporanea con permessi del proprietario,
poi pubblicato con rinomina atomica esclusiva (Linux `renameat2` o macOS
`renamex_np`). Un errore ordinario rimuove lo staging; un arresto forzato o perdita
alimentazione può lasciare una cartella `.orbyx-*`, che non è un prodotto completo.
Non viene fornita una garanzia di durabilità dopo perdita di alimentazione, né
ripresa automatica di lavori interrotti.

## English

Before decoding, the engine validates the PNG signature and IHDR: maximum 1 GiB
encoded file, unsigned 8/16-bit grayscale, dimensions at most 100,000 pixels per
axis, height above 3,606, and at most 150 million pixels per channel. Calibration
JSON is limited to 1 MiB. These checks are **not a global memory quota or sandbox**.

A 150 Mpx 16-bit channel needs approximately 300 MB before temporary allocations.
Six channels, aligned products, merged canvases, CPU/GPU copies and ECC buffers
may require several GB. Two ECC computations run concurrently per VIS product.
RAM-aware scheduling, tiled processing, process quotas and decoder isolation are
not implemented. Use trusted acquisitions; for untrusted data apply external
unprivileged process/memory isolation. Header checks cannot guarantee protection
against every codec vulnerability.

Output is written to an owner-only staging directory and published through an
exclusive atomic rename (Linux `renameat2`, macOS `renamex_np`). Ordinary errors
remove staging. Forced termination or power loss may leave `.orbyx-*` directories;
these are not completed products. Power-loss durability and automatic recovery
of interrupted jobs are not provided.
