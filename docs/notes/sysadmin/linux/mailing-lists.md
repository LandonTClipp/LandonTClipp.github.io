---
icon: octicons/mail-16
---

## Setting Up AERC With Gmail

I followed [this](https://tilde.club/~djhsu/aerc-gmail-oauth2.html) guide. Below is a copy of the text:

### Step 1: setup Google Cloud project

Follow the instructions in https://alpineapp.email/alpine/alpine-info/misc/RegisteringAlpineinGmail.html with the following changes:

When you create the OAuth client ID, select Web application instead of Desktop app as the Application type.
When asked to list Authorized redirect URIs, add https://oauth2.dance/ (which will be crucial in Step 2).
The last bit is needed to use Google’s OAuth2 authentication script to generate the OAuth2 refresh token (in Step 2), and apparently it is not possible to authorize redirect URIs with credentials for Desktop apps.

This will generate two strings, client_id and client_secret.

### Step 2: generate OAuth2 token

Run the following script:

<script src="https://gist.github.com/LandonTClipp/84b4855fbca0f1d088990a8f7a6e00de.js"></script>

This creates a file at `~/.config/aerc/accounts.conf` with proper credentials. You might need to configure aerc to use this path instead of its default accounts.conf path.
