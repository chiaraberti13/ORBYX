# Verification record / Registro verifiche

Date: 2026-11-08. This record concerns this initial source implementation, not a production certification.

| Check | Evidence | Limit |
|---|---|---|
| Native build | GCC 13.3, CMake 3.28, OpenCV 4.6, JsonCpp 1.9.5, Qt 6.4.2 | Ubuntu 24.04 x86_64 only locally |
| Core | Synthetic grayscale PNG 8/16 bit; affine generation, ROI/padding, separate CBOR provenance, known-shift ECC and invalid input | No real satellite acquisitions |
| ECC | Recovered known synthetic shifts within 0.1 px; regenerated pixels within 1 level | Not a claim about real scene accuracy |
| Safety | Snapshot PNG/JSON reads, pre-decode dimensions, truncated and oversized files, duplicate JSON keys, invalid ROI, symlink and competing output rejection | Native codecs are not sandboxed; dependencies not instrumented |
| Batch | 14 unittest: schema, lock, drift, resume, lingua e protezione dati | Coda GUI e filesystem remoti non verificati |
| Desktop | GUI worker→core→8 file confrontati byte per byte con CLI, overwrite rifiutato; offscreen smoke; visual IT/dark 1280×860 and EN/light 960×680 | No real display, user study or screen-reader verification |
| ASan/UBSan | Five headless CTest groups passed remotely with detect_leaks=1 and UBSAN halt-on-error; locally detect_leaks=0 required | Native distro dependencies are not instrumented |
| Static analysis | Cppcheck 2.13 warning/performance/portability checks passed | Not a proof of absence of vulnerabilities |
| Install/package | Local install prefix and CPack TGZ generation succeeded | No clean-machine binary installation or AppImage test |
| Remote CI | Ubuntu 22.04/24.04/26.04 x86_64 build + seven CTest groups + DEB install smoke passed on 4190002; ASan/UBSan with leak checking and Cppcheck passed | Offscreen and synthetic data; not real GPU/SatDump validation |

Ubuntu 26.04 runner labels were checked against [GitHub's September 17 announcement](https://github.blog/changelog/2026-09-17-ubuntu-26-generally-available-and-latest-migration/). This establishes runner availability, not ORBYX compatibility.

## Open gates / Verifiche aperte

- Redistributable real RDAS datasets with provenance, pixel/geometric references and SatDump reimport tests.
- CPU/OpenCL equivalence and memory/throughput benchmarks on actual hardware.
- Clean-machine dependency-resolution installs, ARM64 and other distributions/macOS.
- Dependency vulnerability inventory for actual release binaries, SBOM and signed reproducible releases.
- Full keyboard/screen-reader checks, guided ROI calibration, onboarding and long-running usability.

I test sintetici consentono di verificare regressioni note e flussi riproducibili. Non attestano precisione su acquisizioni reali, interoperabilità geometrica con SatDump, accelerazione GPU o supporto alle piattaforme non eseguite. I dati e le cartelle parent devono restare invariati e controllati dall'utente durante l'elaborazione; non è implementato un confinamento filesystem basato su descriptor.

Remote build/package evidence: [4190002 / run 42477106167](https://github.com/chiaraberti13/ORBYX/actions/runs/42477106167). Ubuntu22.04 initially failed Qt/OpenGL configuration; adding libgl-dev fixed it and the full matrix was rerun successfully.

CodeQL completed successfully on revision 4190002: [run 42477106212](https://github.com/chiaraberti13/ORBYX/actions/runs/42477106212). A successful analysis run is not a claim that all possible vulnerabilities are absent.
