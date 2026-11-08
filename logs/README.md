# Log locali / Local logs

Gli script di installazione, avvio desktop e CLI creano automaticamente qui un log completo e un file `.errors.log` per ciascuna esecuzione. Il nome contiene data UTC e identificativo del processo. Sono inclusi dipendenze, attesa APT, configurazione, compilazione, test, installazione e avvio. In caso di errore viene conservato anche il contesto della fase fallita, compresi messaggi emessi su stdout.

I file generati sono privati per l'utente e ignorati da git. Non vengono caricati automaticamente né inviati a terzi. Prima di condividerli controllare eventuali nomi di file o dati locali presenti nei messaggi delle dipendenze. Non cancellare i blocchi APT/dpkg e non interrompere gli aggiornamenti per risolvere una contesa: l'installer attende e segnala un errore se il limite viene raggiunto.

Generated installation/desktop/CLI logs and per-session error logs stay local and are ignored by git. No automatic upload occurs. Setup waits for package-manager locks; it never removes locks or kills package managers.
