# Fedora44 in VM pulita / Clean Fedora44 full VM

## Italiano

Il workflow dedicato costruisce l'RPM in Fedora44 nativo, poi avvia una nuova VM x86_64 da Fedora Cloud Base Generic44-1.7. Il guest ha disco, boot e kernel propri: non è un container che condivide il kernel Ubuntu del runner. L'immagine ufficiale scaricata realmente è583729152byte, SHA256 `28680fe5b371a5a82ebf43a31926e086a168e59949d03969c5093e7071f90b7f`, corrispondente al CHECKSUM pubblicato da Fedora. Non viene avviata se il digest cambia. La verifica qui è del digest fissato e dei byte scaricati; non viene dichiarata una verifica della firma GPG Fedora.

L'avvio usa QEMU/KVM quando `/dev/kvm` è disponibile, altrimenti TCG dichiarato nei log. Cloud-init prepara un account temporaneo con chiave SSH generata per il singolo job, accesso soltanto via porta inoltrata su127.0.0.1. Nessuna password o chiave personale richiesta; disco guest e chiavi private non vengono pubblicati. L'identità SSH è memorizzata in un file known_hosts effimero tramite accept-new sul collegamento locale.

Nel guest vengono controllati Fedora44, virtualizzazione VM effettiva, assenza di container e kernel `fc44`. Si installa l'RPM con le sole dipendenze runtime dichiarate e si verifica l'assenza di toolchain/pacchetti-devel. CLI e GUI offscreen generano gli stessi otto prodotti sintetici; esportano inoltre Natural Color identico. Disinstallazione verificata con conservazione dei dati originali. Log seriale, inventario runtime e log del guest sono conservati negli artifact, esclusi dischi e chiavi.

**Stato iniziale:** script Bash/YAML e pin verificati; l'immagine è stata scaricata e verificata realmente. Il risultato completo deve essere osservato nel run remoto e registrato in `PROGRESS.md`. L'esistenza del workflow non certifica da sola l'avvio della VM. GUI offscreen non significa test interattivo con display, screen reader o FUSE.

## English

A dedicated workflow builds a Fedora44 RPM then boots a fresh Fedora Cloud Base Generic44-1.7 virtual machine with its own kernel and disk. The official image's actual downloaded bytes match the pinned digest above; no unobserved Fedora GPG signature verification is claimed. QEMU uses KVM if available or explicitly logged TCG. Per-job SSH keys and loopback-only port forwarding require no personal credentials; private keys/disks are excluded from artifacts.

The guest asserts Fedora44, a genuine VM rather than a container, and an fc44 kernel. It installs only runtime requirements, checks no development packages are present, compares installed CLI/offscreen GUI outputs and Natural Color, and uninstalls while retaining inputs. Serial/guest logs and package inventory form the evidence. Remote success must be observed before claiming the complete VM check passed. This does not cover interactive display, screen readers or FUSE.

- [Workflow](../.github/workflows/fedora-vm.yml)
- [Test script](../scripts/ci/fedora-vm.sh)
- [Official Fedora44 cloud images](https://download.fedoraproject.org/pub/fedora/linux/releases/44/Cloud/x86_64/images/)
