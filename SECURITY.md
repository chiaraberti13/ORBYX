# Security

Treat satellite images and calibration profiles as untrusted input. Run with ordinary user permissions, keep OpenCV, Qt and image-codec libraries updated, and use a dedicated output directory. ORBYX does not need network access or elevated privileges.

Report reproducible security defects through the repository's private GitHub vulnerability reporting facility when available. If unavailable, contact the repository owner privately using a verified channel; do not publish sensitive image data or exploit details in a public issue. Include revision, dependencies, minimal synthetic reproducer and observed impact. No response SLA is promised.

The archived upstream snapshot is reference material, not the supported runtime. Only `src/` is built by default. See docs/AUDIT.md for original findings and PROGRESS.md for verification status. Input limits reduce resource exhaustion but do not sandbox native image decoders. Real-data SatDump and GPU equivalence remain release gates.

## Private reporting process

Use **Report a vulnerability** in the repository Security tab if private reporting is available. Otherwise use a verified private contact published by the repository owner. If no private contact is available, open a public issue asking only for a private contact, without vulnerability details. Do not publish exploit details, credentials, personal data or sensitive logs in public issues.

Include the affected revision, environment, impact and minimal synthetic reproduction. Coordinate disclosure with the maintainer. Acknowledgement and remediation depend on availability and severity; no fixed response deadline is guaranteed.
