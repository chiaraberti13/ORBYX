# Stato delle cinque verifiche reali

Questo documento riguarda i cinque limiti di verifica iniziali. Non sostituisce la roadmap delle funzionalità ancora pianificate.

| Verifica | Risultato osservato | Limite residuo |
|---|---|---|
| M3-01 RDAS/SatDump | CADU reale con provenienza e SHA256; decodifica autentica, calibrazione e output nativo16bit; confronto di tutti i pixel dei sei canali; caricamento dei due prodotti in SatDump | Il campione non fornisce proiezione/calibrazione di riferimento. Precisione assoluta, geolocalizzazione e ROI di unione non certificate |
| M3-04 Natural Color | Core, CLI e desktop EN/IT; RGB321 con correzione hue floating point; fixture8/16bit e due scene reali; riferimento scalare indipendente e ispezione visiva | Resa visiva, non colore vero scientificamente calibrato; artefatti della sorgente mantenuti |
| M3-02 piattaforme | Ubuntu22.04/24.04/26.04, Fedora44 in userspace pulito, ARM64 Ubuntu nativo e AppImage in userspace pulito; VM KVM Fedora44 con proprio kernel e test installazione/rimozione riusciti | Display interattivo/FUSE/macOS non implicati |
| M4-02 firma reale | Attestazioni autentiche Sigstore/GitHub OIDC con identità vincolata al workflow, SHA/branch e rifiuto di manomissioni | Non è una firma GPG personale. Aggiornamento/rollback automatico dell'app resta un task distinto |
| M3-03 GPU | Gate hardware reale, kernel esplicito preparato, CPU fallback escluso. Ambiente locale Linux e runner pubblico macOS Intel: nessuna GPU OpenCL utilizzabile | Test GPU saltato, non superato. Restano profilazione/equivalenza e benchmark su hardware effettivo |

## Prove osservate

- VM Fedora44 completa: [run42517397293](https://github.com/chiaraberti13/ORBYX/actions/runs/42517397293), commit `9a52e07d1e941b155ce9194fe019fba4de1d4549`: KVM, kernel6.19.10-300.fc44.x86_64, cloud-init `done`, installazione e rimozione RPM, CLI/GUI e Natural Color equivalenti, input preservati.

- Natural Color e pipeline RDAS: [run42516055833](https://github.com/chiaraberti13/ORBYX/actions/runs/42516055833), commit `a531cecdcacccd9b6164e8cfe344158bef95db58`.
- Inventario nativo Apple OpenCL su runner standard `macos-15-intel`: [run42517297975](https://github.com/chiaraberti13/ORBYX/actions/runs/42517297975), commit `f2bf5b54011a8360f7c2e648b7b88dc13b556af7`:0GPU disponibili, build/esecuzione del verifier GPU saltata. Il successo del job d'inventario non equivale a un test GPU superato né a compatibilità ORBYX/macOS verificata.
- Regressioni, sanitizers, fuzzing e analisi statica: [run42517397183](https://github.com/chiaraberti13/ORBYX/actions/runs/42517397183), commit `9a52e07d1e941b155ce9194fe019fba4de1d4549`.
- Fedora/ARM64 installati: [run42517397266](https://github.com/chiaraberti13/ORBYX/actions/runs/42517397266), stesso commit.
- AppImage in userspace pulito: [run42517397247](https://github.com/chiaraberti13/ORBYX/actions/runs/42517397247), stesso commit.
- Firma OIDC reale e verifica: [run42517397338](https://github.com/chiaraberti13/ORBYX/actions/runs/42517397338), stesso commit.
- CodeQL: [run42517397274](https://github.com/chiaraberti13/ORBYX/actions/runs/42517397274), stesso commit.

Report numerici e limiti metodologici: [RDAS reale](REAL_RDAS.md), [evidenza RDAS](REAL_RDAS_EVIDENCE.md), [Natural Color](NATURAL_COLOR.md), [evidenza colore](NATURAL_COLOR_EVIDENCE.md), [GPU](GPU_REAL.md), [VM Fedora](FEDORA_VM.md), [firma e identità](RELEASE_IDENTITY.md).

## English

The table records the original five real-validation gaps, not completion of every planned roadmap feature. Authentic RDAS execution, real-scene Natural Color, supported Linux packaging/runtime checks and identity-bound OIDC provenance are verified to their stated scopes. Absolute geometry still lacks ground truth. Actual local and standard Intel macOS runner inventories expose no usable OpenCL GPU; GPU execution remains skipped/unverified. Personal GPG signing, automatic update/rollback and interactive desktop/FUSE/macOS support are not inferred from these results.
