# Identità della firma / Signing identity

## Italiano

ORBYX può firmare la provenienza dei suoi artifact senza richiedere una chiave privata personale all'utente. Il workflow [Signed artifact provenance](../.github/workflows/attest.yml) usa l'identità OIDC emessa da GitHub per il workflow del repository pubblico `chiaraberti13/ORBYX`. Sigstore certifica una chiave di breve durata e produce un bundle firmato con prova di provenienza. Non è una chiave effimera di test: l'identità certificata è quella del workflow GitHub e la verifica usa la catena di fiducia Sigstore. Non è la firma GPG personale di Chiara Berti e non la impersona.

Il job esegue build e test effettivi, prepara il TGZ binario, i sorgenti corrispondenti e `SHA256SUMS`, poi attesta tutti e tre. Prima di pubblicare gli artifact verifica crittograficamente ogni soggetto, imponendo repository, percorso del workflow, branch `main`, SHA del commit e runner GitHub ospitato. Verifica anche che una copia manomessa venga rifiutata. Il token OIDC e la chiave di breve durata non vengono salvati nel repository; non occorre configurare un secret di firma. I soli permessi aggiuntivi sono `id-token: write` e `attestations: write`. Nessuna release GitHub o aggiornamento dell'applicazione viene creato automaticamente.

**Stato della verifica:** la configurazione è implementata. La firma reale e la sua verifica devono essere dichiarate superate soltanto dopo il successo del relativo job GitHub; il runtime locale non dispone dell'identità OIDC del job. Il risultato remoto e il suo commit vanno registrati in `PROGRESS.md`. L'assenza di un secret GPG personale non blocca questa modalità di autenticazione, ma l'aggiornamento/rollback firmato resta una funzionalità distinta.

Dall'artifact del run si ottengono gli archivi, il bundle `provenance.sigstore.json` e i risultati `*.verification.json`. Dopo aver scelto lo SHA di una build approvata, verificare un archivio con una versione aggiornata della CLI GitHub:

```sh
gh attestation verify orbyx-0.1.0-Linux.tar.gz \
  --repo chiaraberti13/ORBYX \
  --signer-workflow chiaraberti13/ORBYX/.github/workflows/attest.yml \
  --source-ref refs/heads/main \
  --source-digest SHA_COMPLETO_DELLA_BUILD_APPROVATA \
  --deny-self-hosted-runners
```

Per usare il bundle scaricato aggiungere `--bundle provenance.sigstore.json`. La verifica del bundle richiede comunque una radice Sigstore fidata: una configurazione completamente offline richiede di predisporla secondo le istruzioni ufficiali. La verifica online predefinita consulta l'associazione corrente del repository; una verifica con il solo bundle confronta i nomi registrati nel certificato al momento della firma. Controllare sempre lo SHA approvato: la provenienza dimostra identità, origine e integrità, non la qualità del software né la freschezza di una versione. Un checksum da solo non sostituisce questa verifica. Gli artifact CI sono conservati per 14 giorni; salvarli insieme al bundle se serve conservarli più a lungo.

## English

The public repository can authenticate artifact provenance through GitHub OIDC and Sigstore without a personal signing secret. The certified signer is the ORBYX workflow, not Chiara Berti's personal GPG identity. This is genuine identity-bound signing, separate from the existing ephemeral-key regression test.

The workflow builds and tests the actual GUI/CLI, packages a binary TGZ and corresponding source, checks their digests, and signs provenance for both archives and the checksums. Verification constrains repository, workflow, main branch, source commit and hosted-runner origin. Authentic subjects must verify before the tampering test is accepted. No private key is committed, no paid service is provisioned, and no GitHub release or application update is published.

A real successful OIDC job is required before reporting this capability as remotely verified. Use the command above with an approved full commit SHA. The optional downloaded bundle retains signature evidence; fully offline verification needs a separately trusted Sigstore root. Provenance is not a software-quality assessment, an anti-rollback policy or proof of build reproducibility. Signed update/rollback logic and personal GPG signing remain separate requirements.

## Fonti ufficiali / Official references

- [GitHub: generating and verifying artifact attestations](https://docs.github.com/en/actions/how-tos/secure-your-work/use-artifact-attestations/use-artifact-attestations)
- [Official attestation action and its inputs](https://github.com/actions/attest)
- [GitHub CLI verification policy options](https://cli.github.com/manual/gh_attestation_verify)
- [Offline verification and trusted roots](https://docs.github.com/en/actions/how-tos/secure-your-work/use-artifact-attestations/verify-attestations-offline)

Action version: `actions/attest` v4 resolved to `1e69f48acb82d1966a394da916b4c1698aa569d6` on 2026-11-08. The workflow uses the immutable SHA.
