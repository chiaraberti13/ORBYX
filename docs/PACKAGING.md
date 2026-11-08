# Packaging / Distribuzione

## Stato verificato / Verified status

| Formato / Format | Stato / Status |
| --- | --- |
| Installazione CMake / CMake install | Layout verificato localmente: CLI, GUI, batch, profilo, licenze, desktop e icona / Layout checked locally |
| Archivio sorgenti TGZ / Source TGZ | Generato e ispezionato: sorgenti e licenze presenti, `.git` e cartelle build esclusi / Generated and inspected |
| DEB | Configurazione CPack con dipendenze native automatiche e Python esplicito; installazione su sistema pulito ancora da verificare / CPack configured; clean-system install pending |
| RPM | Procedura predisposta; generazione e installazione non verificate / Procedure prepared; generation and install unverified |
| AppImage | Procedura di assemblaggio proposta, nessuna AppImage verificata / Proposed assembly; no verified AppImage |

## Italiano

### Build e archivio sorgenti

Da una copia del repository, con dipendenze native installate per la distribuzione:

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build-release --parallel 2
ctest --test-dir build-release --output-on-failure
cpack --config build-release/CPackSourceConfig.cmake -G TGZ -B build-release/packages
```

`CPackSourceConfig.cmake` produce un archivio dei sorgenti, distinto dal pacchetto
binario generato da `CPackConfig.cmake`. Conservare il commit ORBYX e la provenienza
upstream insieme alle evidenze di release. Non distribuire una build con sanitizer
come release ordinaria.

### DEB

Eseguire la build sulla versione Ubuntu minima destinata al pacchetto: una build
su una versione più recente può richiedere simboli di libreria assenti sulle versioni
precedenti. Sono necessari `dpkg-shlibdeps` e i pacchetti di sviluppo reali della
distribuzione; librerie estratte in un prefisso privato non equivalgono a dipendenze
installate e registrate da dpkg.

```sh
cpack --config build-release/CPackConfig.cmake -G DEB -B build-release/packages
# Sostituire FILE.deb con il file appena prodotto:
dpkg-deb --info FILE.deb
dpkg-deb --contents FILE.deb
```

`CPACK_DEBIAN_PACKAGE_SHLIBDEPS` ricava le dipendenze ELF dalla distribuzione di
build. Quando `ORBYX_INSTALL_BATCH=ON` (predefinito), viene aggiunto esplicitamente
`python3 (>= 3.10)`: il rilevamento delle librerie ELF non può dedurlo dal sorgente
Python. Con `-DORBYX_INSTALL_BATCH=OFF` il batch non viene installato e questa
dipendenza non viene aggiunta. I test di sviluppo continuano a richiedere Python.

Prima di una release installare il DEB in una VM pulita della piattaforma dichiarata,
verificare `orbyx --version`, `orbyx-batch --help` se presente, avvio GUI dalla voce
del menu e un'elaborazione completa con confronto CLI/GUI. Verificare anche i plugin
di piattaforma Qt e i codec: le dipendenze ELF da sole non dimostrano la completezza
dei plugin caricati dinamicamente. Documentare installazione e rimozione del pacchetto.

### RPM

Compilare nativamente su Fedora o un'altra distribuzione RPM scelta come baseline,
con `rpmbuild` e dipendenze di sviluppo installati. Non convertire il DEB.

```sh
cpack --config build-release/CPackConfig.cmake -G RPM -B build-release/packages
rpm -qp --requires FILE.rpm
rpm -qpl FILE.rpm
```

CPack usa il rilevamento automatico RPM per le librerie native e aggiunge
`python3 >= 3.10` se il batch è installato. Questa procedura deve ancora essere
eseguita e verificata su una distribuzione RPM; non implica compatibilità Fedora,
Arch o altre piattaforme. Ripetere i controlli di installazione e del flusso completo
in una VM pulita prima di pubblicare un RPM.

### AppImage (procedura proposta, non verificata)

Usare una baseline Linux compatibile con il pubblico previsto. Il bundle non deve
includere Python implicitamente: per il primo esperimento escludere il batch e
distribuirlo separatamente tramite pacchetto nativo o sorgenti.

```sh
cmake -S . -B build-appimage -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr -DORBYX_INSTALL_BATCH=OFF
cmake --build build-appimage --parallel 2
DESTDIR="$PWD/AppDir" cmake --install build-appimage --prefix /usr
```

Preparare una versione verificata e fissata di
[linuxdeploy](https://github.com/linuxdeploy/linuxdeploy) e del suo
[plugin Qt](https://github.com/linuxdeploy/linuxdeploy-plugin-qt), verificandone
supporto Qt 6, provenienza e checksum prima dell'esecuzione. Con gli strumenti
predisposti nel `PATH`, il comando di assemblaggio da validare è:

```sh
linuxdeploy --appdir AppDir --plugin qt \
  --executable AppDir/usr/bin/orbyx-gui \
  --executable AppDir/usr/bin/orbyx \
  --desktop-file AppDir/usr/share/applications/orbyx.desktop \
  --icon-file AppDir/usr/share/icons/hicolor/scalable/apps/orbyx.svg \
  --output appimage
```

I nomi degli eseguibili scaricati possono differire; predisporli come `linuxdeploy`
e plugin secondo le istruzioni delle versioni scelte. Testare il bundle su sistemi
puliti senza Qt/OpenCV preinstallati, verificando i plugin, la ricerca del profilo
di calibrazione, caricamento delle immagini e correttezza numerica. Non affermare
portabilità universale: driver GPU, glibc e plugin richiedono verifica. Licenze,
attribuzioni e disponibilità dei sorgenti devono accompagnare anche il bundle.

## English

Build with the commands above and run all tests before packaging. The source TGZ
uses `CPackSourceConfig.cmake`; binary packages use `CPackConfig.cmake`. Build DEBs
on the oldest target Ubuntu baseline with real distro development packages so
`dpkg-shlibdeps` can resolve native requirements. CPack explicitly adds
`python3 (>= 3.10)` for DEB and `python3 >= 3.10` for RPM when
`ORBYX_INSTALL_BATCH=ON`. Set it to `OFF` to omit the optional script and its Python
runtime dependency; development tests still use Python.

Build RPMs natively on the chosen RPM distribution using `rpmbuild`; do not convert
DEBs. Inspect package file lists and dependencies using the commands above. Neither
a generated archive nor ELF dependency discovery proves a working clean-system
installation: test desktop launching, Qt platform plugins/codecs, calibration-profile
lookup, complete processing and numerical equivalence on every claimed platform.

The AppImage procedure above is an **unverified proposal**. Use pinned, provenance-
and checksum-verified linuxdeploy/Qt-plugin versions compatible with Qt 6. The proposed
AppDir excludes the Python batch runner. Verify the final bundle on clean machines
without Qt/OpenCV; GPU drivers, glibc and dynamically loaded plugins need separate
coverage. Preserve licenses, attribution and corresponding-source availability in
every distribution format. RPM and AppImage support must remain unverified until
those builds and complete installed workflows have actually passed.
