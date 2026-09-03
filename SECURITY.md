# Security and Trust

## SmartScreen

ColorblindAssist is distributed as a new Windows desktop application. A new
or unsigned download can trigger the Windows Defender SmartScreen message
`Windows protected your PC` because Microsoft has not established reputation
for the publisher or the exact file hash yet.

The official downloads are published only through the project's GitHub
Releases page. Verify the SHA-256 value shown next to each release asset before
running it.

## Code signing

The release workflow supports Authenticode signing when these GitHub Actions
secrets are configured:

- `WINDOWS_SIGNING_CERTIFICATE_BASE64`
- `WINDOWS_SIGNING_CERTIFICATE_PASSWORD`

The certificate must identify the publisher and be issued by a trusted
certificate authority. A self-signed certificate does not establish public
trust. Signing can show a verified publisher name, but a new publisher or
file can still receive an initial SmartScreen reputation warning.

## Reporting issues

Please open a GitHub issue with the release version, Windows version, and the
exact warning text. Do not include medical records, private files, passwords,
or other personal information.
