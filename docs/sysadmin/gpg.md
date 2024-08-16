# GPG

## [Subkeys](https://wiki.debian.org/Subkeys#:~:text=In%20other%20words%2C%20subkeys%20are,mails%20with%20GnuPG%20at%20all.)

Subkeys can be generated from root keys. Subkeys are separate public/private keypairs in their own right, but are mathematically bound to the parent keys.

When creating new keys, GPG creates the primary key as signing-only and automatically creates encryption subkeys.

### Why Use Subkeys?

Subkeys make management easier. Primary keys should be kept incredibly safe, which usually means storing the keys on a Yubikey device that can only be accessed through physical means (and possibly hidden behind lawyers and/or security guards). Subkeys can be generated from the primary key, which can then be used in your security infrastructure. Subkeys can be revoked and re-created if needed by gaining access to the primary key.

## Revocation Certificates

https://www.ias.edu/security/creating-revocation-key

Revocation certificates are used to invalidate a public key. This certificate is read by clients of a keyserver that they should no longer trust the key. The revocation itself is not necessarily doing anything to _mathematically_ invalidate the key, but it's rather an advisory signal to clients that it should be invalidated and not trusted.

These certificates must be signed by the private primary key, and thus if you are using a subkey scheme, you must gain access to the primary key. This is done because clients must use the public primary key to verify the authenticity of the certificate and that such a key does indeed have the authority to revoke the subkey.

## GPG Server

- https://sequoia-pgp.org/blog/2019/06/14/20190614-hagrid/
- https://github.com/hockeypuck/hockeypuck

