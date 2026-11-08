# Fedora44 in VM pulita / Clean Fedora44 full VM

## Italiano

Il workflow dedicato costruisce l'RPM in Fedora44 nativo, poi avvia una nuova VM x86_64 da Fedora Cloud Base Generic44-1.7. Il guest ha disco, boot e kernel propri: non è un container che condivide il kernel Ubuntu del runner. L'immagine ufficiale scaricata realmente è583729152byte, SHA256 `28680fe5b371a5a82ebf43a31926e086a168e59949d03969c5093e7071f90b7f`, corrispondente al CHECKSUM pubblicato da Fedora. Non viene avviata se il digest cambia. La verifica qui è del digest fissato e dei byte scaricati; non viene dichiarata una verifica della firma GPG Fedora.

L'avvio usa QEMU/KVM quando `/dev/kvm` è disponibile, altrimenti TCG dichiarato nei log. Cloud-init prepara un account temporaneo con chiave SSH generata per il singolo job, accesso soltanto via porta inoltrata su127.0.0.1. Nessuna password o chiave personale richiesta; disco guest e chiavi private non vengono pubblicati. L'identità SSH è memorizzata in un file known_hosts effimero tramite accept-new sul collegamento locale.

Nel guest vengono controllati Fedora44, virtualizzazione VM effettiva, assenza di container e kernel `fc44`. Si installa l'RPM con le sole dipendenze runtime dichiarate e si verifica l'assenza di toolchain/pacchetti-devel. CLI e GUI offscreen generano gli stessi otto prodotti sintetici; esportano inoltre Natural Color identico. Disinstallazione verificata con conservazione dei dati originali. Log seriale, inventario runtime e log del guest sono conservati negli artifact, esclusi dischi e chiavi.

**Verifica effettiva completata:** [run42517397293](https://github.com/chiaraberti13/ORBYX/actions/runs/42517397293), commit `9a52e07d1e941b155ce9194fe019fba4de1d4549`, `success`. Acceleratore KVM, kernel guest6.19.10-300.fc44.x86_64, cloud-init `done` senza degradazione; RPM installato, otto prodotti CLI/GUI identici, PNG Natural Color identici, disinstallazione e input preservati. Il test OpenCL è saltato per assenza del runtime, senza etichettarlo GPU superata. GUI offscreen non significa test interattivo con display, screen reader o FUSE.

Durante la verifica sono stati corretti due problemi dell'harness: chiamata di enumerazione OpenCL4.13 prima del controllo runtime e `datasource_list` inserito nei user-data cloud-init (lo schema ufficiale25.3 lo rifiuta). Lo schema dei campi utilizzati è stato verificato localmente e la nuova VM è stata inizializzata senza ignorare codici di errore.

## English

A dedicated workflow builds a Fedora44 RPM then boots a fresh Fedora Cloud Base Generic44-1.7 virtual machine with its own kernel and disk. The official image's actual downloaded bytes match the pinned digest above; no unobserved Fedora GPG signature verification is claimed. QEMU uses KVM if available or explicitly logged TCG. Per-job SSH keys and loopback-only port forwarding require no personal credentials; private keys/disks are excluded from artifacts.

The guest asserts Fedora44, a genuine VM rather than a container, and an fc44 kernel. It installs only runtime requirements, checks no development packages are present, compares installed CLI/offscreen GUI outputs and Natural Color, and uninstalls while retaining inputs. Serial/guest logs and package inventory form the evidence. Remote success must be observed before claiming the complete VM check passed. This does not cover interactive display, screen readers or FUSE.

- [Workflow](../.github/workflows/fedora-vm.yml)
- [Test script](../scripts/ci/fedora-vm.sh)
- [Official Fedora44 cloud images](https://download.fedoraproject.org/pub/fedora/linux/releases/44/Cloud/x86_64/images/)

Observed remote success: run42517397293 on commit9a52e07; actual KVM/fc44 kernel, clean install, installed CLI/GUI/colour equivalence and uninstall/input preservation passed.
