---
icon: octicons/repo-16
---

# Repositories

## apt

### Security

Readings:

- https://blog.cloudflare.com/dont-use-apt-key

To configure a GPG public key to be trusted by apt:

1. Download public key from web
2. Import to GPG
    ```
    gpg --import /tmp/key.pub
    ```
3. Export the key in binary format:
    ```
    gpg --export $KEY_ID > /etc/apt/trusted.gpg.d/key_name.gpg
    ```
