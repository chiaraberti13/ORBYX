# Aggiornamento verificato e ripristino

La procedura è disponibile dal pulsante **Aggiornamenti verificati…** dell’app.
Serve GitHub CLI (`gh`) con il comando `gh attestation verify`; se manca o è
obsoleta, il programma si ferma. Non viene richiesta la tua chiave GPG privata.
L’identità verificata è quella del workflow ORBYX del repository, non della persona.

## Aggiornare dall’app

1. Su GitHub apri **Actions → Signed artifact provenance** e scegli una build
   riuscita di `main`, controllando il commit che vuoi installare.
2. Scarica l’artifact per la **stessa versione Ubuntu e architettura** del tuo
   computer. Estrai lo ZIP in una cartella con il gestore file.
3. Nell’app premi **Aggiornamenti verificati…**. Con i selettori scegli il pacchetto
   binario `.tar.gz` della tua piattaforma e `provenance.sigstore.json` della stessa
   build. Non scegliere l’archivio dei sorgenti.
4. Incolla il commit completo della build approvata e premi **Verifica e installa**.
5. Dopo il messaggio di completamento chiudi e riapri l’app.

Puoi aprire la stessa finestra dal terminale, dalla cartella ORBYX:

```bash
bash update.sh
```

Il controllo usa repository, workflow, branch main, commit approvato e runner
GitHub ospitato. La verifica della firma precede estrazione ed esecuzione. La
piattaforma dichiarata **dentro il pacchetto firmato** deve coincidere con quella
locale, e CLI/GUI devono superare i controlli d’avvio. La disponibilità del servizio
Sigstore e delle sue radici fidate è necessaria: nessun fallback non verificato.

## Ripristinare

Premi **Ripristina versione verificata precedente**, oppure:

```bash
bash update.sh rollback
```

Il rollback riverifica l’archivio e ricostruisce una copia pulita. Non esegue i
file precedentemente installati fidandosi di un semplice hash locale. Una cache
manomessa viene rifiutata. Un rollback incompatibile con la distribuzione corrente
viene rifiutato. La prima installazione firmata non ha ancora una release firmata
precedente da ripristinare.

L’installazione sorgente precedente, quando presente, viene conservata in un
backup `legacy-*`, indicato nel log, ma non diventa una release firmata. Profili,
immagini e impostazioni esterni all’installazione non vengono spostati. Un’app già
aperta continua a usare i vecchi file conservati fino al riavvio.

Per conoscere lo stato:

```bash
bash update.sh status
```

Dopo il passaggio alla gestione firmata, `install.sh` rifiuta di scrivere nei file
della release: gli aggiornamenti successivi seguono questa procedura. Una modifica
locale dei file installati non viene presentata come un nuovo aggiornamento firmato.
Gli aggiornamenti operano come utente normale, senza sudo.

## Ambito e test

Sono supportati pacchetti **nativi Linux**, con build separate Ubuntu22.04,
24.04 e26.04 x86_64 nella workflow di attestazione. La piattaforma e le dipendenze
vengono controllate prima di cambiare la versione attiva. Non è un aggiornamento
interno dell’AppImage, un gestore APT/RPM o un servizio di aggiornamento automatico.

I test locali delle transazioni iniettano esplicitamente un verifier di prova e
non attestano una firma reale. La workflow OIDC prova invece installazione,
rollback e rifiuto di manomissioni con firme autentiche, senza secret personale.
Il pin del commit richiede una scelta dell’operatore: la provenienza dimostra
origine/integrità, non certifica la qualità o la freschezza di una versione.
