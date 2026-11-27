# Security Policy

## Supported versions

Materialized Linux is alpha software. Only the latest alpha release line receives security fixes.

| Version | Supported |
|---|---|
| 0.2.x (current: 0.2.1) | Yes |
| 0.2.0 and earlier (incl. 0.1.x) | No - upgrade to the latest release |

## Reporting a vulnerability

**Please do not open a public GitHub issue for security vulnerabilities.**

Email **jihoo125@j1t.xyz** with:

- A description of the issue and its impact.
- Reproduction steps or a proof of concept.
- The affected component (e.g. `matcomp`, `matsessiond`, `matinstall-core`, the ISO build) and the ISO version / SHA-256.
- For VM-related issues: hypervisor, firmware and guest configuration.

This is a small alpha project, so response is best-effort. You will get an acknowledgment as soon as possible, and credit in the fix notes if you want it. If GitHub's private vulnerability reporting is enabled on the repository, you may use that instead of email.

## Scope and known intentional behaviors

The following are **documented, intentional** properties of the evaluation image, not vulnerabilities:

- The live session auto-logins as `materialized` / `materialized`. This is a public evaluation credential for isolated VM use only; remove or change it before any other use.
- The `matbrowser` wrapper relaxes the Firefox content sandbox in the live image for evaluation. Do not use the live image as a daily browser environment.
- The GRUB images in the ISO are unsigned, so Secure Boot must be off. Signed, reproducible release images are on the roadmap.

Security-relevant design properties worth knowing when auditing:

- Only the installer core (`matinstall-core`) is privileged, gated by polkit (D-027). Destructive disk operations require explicit opt-in, root, and a real disk probe; the installer test suite is dry-run only.
- No telemetry is collected; crash and test artifacts stay local unless explicitly shared (D-028).
- The compositor, theme service and shell run per-user; theme artifacts are written atomically under `/run/user/$UID/materialized/`.

## Handling

Confirmed vulnerabilities are fixed on `main`, released in the next alpha tag, and documented in the release checklist under `docs/`. Reporters are credited unless they prefer otherwise.