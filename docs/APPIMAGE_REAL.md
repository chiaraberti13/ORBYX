# AppImage reale / Real AppImage

## Italiano

Il workflow [`appimage.yml`](../.github/workflows/appimage.yml) costruisce un vero
eseguibile AppImage type-2 x86_64, partendo da Ubuntu 22.04. Non rinomina un TGZ e
non dichiara un AppDir equivalente a una AppImage. La build usa Qt 6 e OpenCV della
distribuzione e omette il runner Python (`ORBYX_INSTALL_BATCH=OFF`): il bundle offre
Workbench/CLI, non il batch Python. La coda non viene presentata come disponibile
senza il relativo runtime.

Il workflow deve passare prima di dichiarare questo formato verificato. La verifica
predisposta usa un **nuovo container Ubuntu 22.04**, privo di Qt e OpenCV installati:

1. Controlla che le librerie Qt/OpenCV non risultino installate nel sistema.
2. Estrae l'AppImage usando il suo runtime, senza FUSE né privilegi speciali.
3. Esegue il binario CLI dentro il bundle e `AppRun --smoke-test` offscreen.
4. Genera fixture sintetiche e confronta gli otto file prodotti dalla CLI e dalla
   GUI installate nel bundle, usando solo librerie raccolte nell'AppDir e i servizi
   di base del sistema.

Questo prova la meccanica reale di packaging/esecuzione in un userspace pulito.
Non dimostra portabilità universale, compatibilità con distribuzioni precedenti
alla baseline, integrazione desktop/FUSE, GPU, dati satellitari reali o qualità
radiometrica. La base container condivide il kernel del runner: non è una VM
grafica completa. Il modulo Python di test è solo un driver esterno di verifica e
non viene incorporato nell'AppImage.

### Provenienza e pin dei tool

I quattro asset vengono scaricati dai repository ufficiali e verificati con SHA-256
**prima** di impostare il bit eseguibile o avviarli. I digest seguenti sono stati
calcolati sui byte scaricati l'8 novembre 2026, non inventati né ricavati dal nome.
Le URL `continuous` possono cambiare: un cambiamento causa un errore di checksum;
serve allora una nuova verifica e un aggiornamento esplicito del pin.

| Asset | SHA-256 |
|---|---|
| `linuxdeploy-x86_64.AppImage` | `df908b19f1ed1cdafa79508d415aa12d6c56ce4d56d4149aa8c09286718706e5` |
| `linuxdeploy-plugin-qt-x86_64.AppImage` | `cfc1055b2b9dbc08412b579f20990b7b41a17b61beaa5847dc9477c96c9e9617` |
| `appimagetool-x86_64.AppImage` | `e8c1169c4af66c3184d7ee110713d4be04befe671fa358e2c1591ee840553b03` |
| `runtime-x86_64` | `617aa77314d57c8b8e4fae9c3f0b35af7b30d5529cb69e98f64f92510ed98a46` |

- [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy): asset ufficiale
  `releases/download/continuous/linuxdeploy-x86_64.AppImage`.
- [plugin Qt](https://github.com/linuxdeploy/linuxdeploy-plugin-qt): supporta Qt 5/6;
  `QMAKE` seleziona `qmake6`, `EXTRA_PLATFORM_PLUGINS` include il plugin offscreen.
- [appimagetool](https://github.com/AppImage/appimagetool): viene invocato
  direttamente con `--runtime-file`, impedendo il download implicito dell'ultimo
  runtime da parte del tool.
- [type2-runtime](https://github.com/AppImage/type2-runtime): runtime esplicitamente
  scaricato e verificato separatamente.

Gli strumenti sono estratti in modalità `--appimage-extract` dopo la verifica:
nessuna dipendenza da FUSE durante la build. Verifica locale effettiva degli asset:
checksum passati, estrazione dei tre tool riuscita; `linuxdeploy --version` riporta
commit `07333c6`, build 373 del 1 novembre 2026, `appimagetool --version` commit
`11d54c9`, build 370 del 6 novembre 2026. La build completa non è eseguita localmente
per assenza di Docker/Podman: i risultati della CI restano la prova necessaria.

L'AppRun controllato imposta i percorsi delle librerie e dei plugin del bundle,
include font DejaVu e configurazione fontconfig locale. Le librerie GLVND incluse
sono dispatch userspace necessarie a QtGui, **non** driver hardware e non una prova
GPU. Il bundle comprende LICENSE/NOTICE ORBYX, copyright della distribuzione e un
inventario delle versioni dei pacchetti di build (sovrainsieme, non SBOM runtime
esatta). L'artifact comprende il sorgente ORBYX corrispondente e `BUILD-TOOLS.txt`.
Le licenze/sorgenti delle dipendenze devono restare disponibili secondo le
rispettive condizioni prima di una distribuzione definitiva.

### Uso

```sh
chmod +x ORBYX-x86_64.AppImage
./ORBYX-x86_64.AppImage                   # GUI, FUSE se disponibile
./ORBYX-x86_64.AppImage --appimage-extract
./squashfs-root/AppRun                   # GUI senza FUSE
./squashfs-root/AppRun --cli --version   # CLI con librerie del bundle
```

Gli artifact sperimentali del workflow hanno durata di 14 giorni. Non viene creata
una release pubblica né attivato un aggiornamento automatico dall'esecuzione del
workflow.

## English

The dedicated workflow builds a genuine x86_64 type-2 AppImage on Ubuntu 22.04,
using Qt 6/OpenCV and omitting the optional Python batch runtime. Four official
upstream tools/runtime assets are SHA-256 pinned to their actual downloaded bytes
before execution; mutable `continuous` URLs fail closed if their content changes.
The explicit runtime file prevents appimagetool from fetching an unpinned runtime.
The Qt plugin officially supports Qt 6 and uses `qmake6` plus the offscreen plugin.

A separate fresh Ubuntu 22.04 container without system Qt/OpenCV extracts the
result without FUSE, runs the bundled CLI and offscreen GUI, and compares all eight
product files on synthetic fixtures. This is a real packaging/runtime test, not
real-acquisition/radiometry or GPU validation. The container shares its host kernel
and does not prove full desktop VM/FUSE integration or universal Linux portability.

Checksums, tool extraction and tool version execution have passed locally. Full
bundle construction and clean-container execution require the published CI run;
the existence of configuration is not a passing result. The artifact includes the
AppImage, corresponding ORBYX source, build-tool identities, distribution notices,
package inventory and runtime log, retained for 14 days. The package inventory is
a build-environment superset, not an exact runtime SBOM. Dependency licence and
source obligations remain applicable to final redistribution.

## Evidenza remota / Remote evidence

Verifica effettiva conclusa con successo sul commit `0d7bf195b6af4ab2d2392c87234945accf499ede`: [run 42501765193](https://github.com/chiaraberti13/ORBYX/actions/runs/42501765193). Il risultato vale per le verifiche descritte, senza estenderlo a piattaforme o identità diverse. / The successful run covers the checks described here, without implying other platforms or signer identities.
