---
icon: material/security
---

# GPG

## [Subkeys](https://wiki.debian.org/Subkeys#:~:text=In%20other%20words%2C%20subkeys%20are,mails%20with%20GnuPG%20at%20all.)

Subkeys can be generated from root keys. Subkeys are separate public/private keypairs in their own right, but are mathematically bound to the parent keys.

When creating new keys, GPG creates the primary key as signing-only and automatically creates encryption subkeys.

### Why Use Subkeys?

Subkeys make management easier. Primary keys should be kept incredibly safe, which usually means storing the keys on a Yubikey device that can only be accessed through physical means (and possibly hidden behind lawyers and/or security guards). Subkeys can be generated from the primary key, which can then be used in your security infrastructure. Subkeys can be revoked and re-created if needed by gaining access to the primary key.

### Primary Key Replication

When generating your primary key, there are two main methods:

1. Generate the key on the Yubikey itself so that the private key at no point has ever touched an external device.
2. Generate the key on an external device and upload it to the Yubikey.

Option 1 is the safest, however it is impossible to copy the Yubikey HSM-generated private key to another Yubikey. Thus it concentrates your chain of trust to a single piece of hardware that if lost or stolen will compromise your entire chain of trust.

Option 2 at least allows you to copy the private key to multiple Yubikeys, however you should perform the private key generation on an airgapped, live-loaded OS that lives on a USB thumbdrive. After the key has been generated and copied to the Yubikeys, the thumbdrive's contents should be destroyed through a softwipe of the SSD, and possibly even further by physical destruction.

## Revocation Certificates

https://www.ias.edu/security/creating-revocation-key

Revocation certificates are used to invalidate a public key. This certificate is read by clients of a keyserver that they should no longer trust the key. The revocation itself is not necessarily doing anything to _mathematically_ invalidate the key, but it's rather an advisory signal to clients that it should be invalidated and not trusted.

These certificates must be signed by the private primary key, and thus if you are using a subkey scheme, you must gain access to the primary key. This is done because clients must use the public primary key to verify the authenticity of the certificate and that such a key does indeed have the authority to revoke the subkey.

## Key Sharding

If you're using a primary/subkey configuration, it's a good idea to shard your primary key so that it can only be reconstructed through the combination of the requisite hardware tokens.

I have not been able to find documentation on how to do this in GPG, so at the moment I only note this concept as a theory.

## GPG Server

- https://sequoia-pgp.org/blog/2019/06/14/20190614-hagrid/
- https://github.com/hockeypuck/hockeypuck

