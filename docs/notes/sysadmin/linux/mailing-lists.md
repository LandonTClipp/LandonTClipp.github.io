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

Use Google’s OAuth2 authentication script https://github.com/google/gmail-oauth2-tools/blob/master/python/oauth2.py to generate an OAuth2 refresh token.

`python oauth2.py --generate_oauth2_token --user={user} --client_id={client_id} --client_secret={client_secret}`

`{user}` is the email address you use to login to Gmail.
`{client_id}` and `{client_secret}` are the strings you got from Step 1.
The script will ask you to visit a URL in your browser and follow some directions, and prompt you for a verification code. After visiting the URL and agreeing to various things, you will be provided a verification code (from https://oauth2.dance). Enter this verification code into the script prompt. The script will then print the Refresh Token (in addition two other things that are not needed).

### Step 3: configure account in aerc

Edit the accounts.conf file in your aerc configuration directory (usually ~/.config/aerc/), and add the following:

```
[{account_name}]
source   = imaps+oauthbearer://{user}:{refresh_token}@imap.gmail.com:993?client_id={client_id}&client_secret={client_secret}&token_endpoint=https%3A%2F%2Foauth2.googleapis.com%2Ftoken
outgoing = smtps+oauthbearer://{user}:{refresh_token}@smtp.gmail.com:465?client_id={client_id}&client_secret={client_secret}&token_endpoint=https%3A%2F%2Foauth2.googleapis.com%2Ftoken
default  = INBOX
{account_name} is whatever you would like to name this account in aerc.
{user} is the email address you use to login to Gmail (same as in Step 2), except it should be URL-encoded (replace @ with %40).
{refresh_token} is the refresh token you got from Step 2, except it should be URL-encoded (replace / with %2F).
{client_id} and {client_secret} are the strings you got from Step 1.
```

!!! tip

    You need to follow the directions from the oauth2.py script regarding redirect URIs:

    ```
    NOTE: The OAuth2 OOB flow isn't a thing anymore. You will need to set the
    application type to "Web application" and then add
    https://google.github.io/gmail-oauth2-tools/html/oauth2.dance.html as an
    authorised redirect URI. This is necessary for seeing the authorisation code on
    a page in your browser.
    ```