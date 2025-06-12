---
icon: simple/kubernetes
---

IRSA
----

AWS's EKS clusters provide the IAM Roles for Service Accounts (IRSA) mechanism for attaching k8s service accounts to an IAM role.

### IAM Authentication Flow

![](/assets/images/iam-authentication-flow.svg)

### IAM Token Authentication Process

AWS services use the following process to verify the validity of the IAM token passed to them:

![](/assets/images/iam-token-verification-process.svg)

### `AWS_WEB_IDENTITY_TOKEN_FILE`

This environment variable is injected into EKS pods that have an attached service account. This points to a file that contains a JWT. I created an example JWT:

=== "JWT"

    ```text title=""
    eyJhbGciOiJSUzI1NiIsImtpZCI6Ijg4MjAxNDY0N2JkZWQ0OWIzNzk3MWY0OGNhMTEwMzdlNWM3MzQ2MWIifQ.eyJhdWQiOlsic3RzLmFtYXpvbmF3cy5jb20iXSwiZXhwIjoxNzQ5ODM5NTA0LCJpYXQiOjE3NDk3NTMxMDQsImlzcyI6Imh0dHBzOi8vb2lkYy5la3MudXMtd2VzdC0yLmFtYXpvbmF3cy5jb20vaWQvMTIzNCIsImp0aSI6ImQyZTEyNzk5LTE3NDctNGFmMy05M2U5LWFlOTJlODcxMzI0OSIsImt1YmVybmV0ZXMuaW8iOnsibmFtZXNwYWNlIjoiazhzLW5hbWVzcGFjZSIsIm5vZGUiOnsibmFtZSI6ImlwLTE5Mi0xNjgtMC0xLnVzLXdlc3QtMi5jb21wdXRlLmludGVybmFsIiwidWlkIjoiZDJlMTI3OTktMTc0Ny00YWYzLTkzZTktYWU5MmU4NzEzMjQ5In0sInBvZCI6eyJuYW1lIjoicG9kLW5hbWUtNWJjNDc2ZjliLWNmdm1iIiwidWlkIjoiZDJlMTI3OTktMTc0Ny00YWYzLTkzZTktYWU5MmU4NzEzMjQ5In0sInNlcnZpY2VhY2NvdW50Ijp7Im5hbWUiOiJzZXJ2aWNlLWFjY291bnQtbmFtZSIsInVpZCI6ImQyZTEyNzk5LTE3NDctNGFmMy05M2U5LWFlOTJlODcxMzI0OSJ9fSwibmJmIjoxNzQ5NzUzMTA0LCJzdWIiOiJzeXN0ZW06c2VydmljZWFjY291bnQ6azhzLW5hbWVzcGFjZTpzZXJ2aWNlLWFjY291bnQtbmFtZSJ9.i4nzWi3liux_yKiRRUqaeB-hOOBy7gEG5oyhaxDLaSUpueybSUWQKy1BNWMi-CPOzp7J66k3JAb1AvUOMPv0Uolwu_gut6-mVlhyNp5DmwcCk6zpXlHqVF06yeSxj0Ix3v-K-J8Z18TwoHUMKgcDyic0c-YVTSTaM8q9x9oXfQr76HcHsI2JZLsdiYNquC4JS8sAq6xR1FJ3PmBUrq45cGnpVU73_q9E4dmtDzoo360k5RqG2kRWZmGBZw76WXdExY_ZkyA9WptEfk9cWajv_sFqaQonA5GqDuP57nwOrv2_h4Qi0Emju-soLJbn6vTYj4xGEcP7vyCPjCdk3pa5Eg
    ```

=== "Decoded JWT"


    ```json title="Header"
    {
    "alg": "RS256",
    "kid": "882014647bded49b37971f48ca11037e5c73461b"
    }
    ```


    ```json title="Payload"
    {
    "aud": [
        "sts.amazonaws.com"
    ],
    "exp": 1749839504,
    "iat": 1749753104,
    "iss": "https://oidc.eks.us-west-2.amazonaws.com/id/1234",
    "jti": "d2e12799-1747-4af3-93e9-ae92e8713249",
    "kubernetes.io": {
        "namespace": "k8s-namespace",
        "node": {
        "name": "ip-192-168-0-1.us-west-2.compute.internal",
        "uid": "d2e12799-1747-4af3-93e9-ae92e8713249"
        },
        "pod": {
        "name": "pod-name-5bc476f9b-cfvmb",
        "uid": "d2e12799-1747-4af3-93e9-ae92e8713249"
        },
        "serviceaccount": {
        "name": "service-account-name",
        "uid": "d2e12799-1747-4af3-93e9-ae92e8713249"
        }
    },
    "nbf": 1749753104,
    "sub": "system:serviceaccount:k8s-namespace:service-account-name"
    }
    ```

=== "Private Key"

    ```text title=""
    -----BEGIN RSA PRIVATE KEY-----
    MIIEpQIBAAKCAQEAmKQLQ5xf0WdB9Wo1H0ahUlJfvQA2UyIOI4lmXiRUHudVCSl5
    zayH8R0aVY+rjFvpjNb4CUV8AOBk9W7dZ96WUF3zysFSUfFTVucPTdi1vZ416rhP
    Ft9faJbYrV/zgLzo3pMxaooZZLY/e2U74+GJx+Obd+MpBh55ZGFSpGzhEPivAhmt
    3qGdPvotsmDpl0BU1hzrUBP9Tlfk0AiQUtBWBc9B69l7UQ+IfUnpjubVvw+uKW8e
    wODe5srfXMM+G4P8imxbZ5bE5M71HblyP5EwpykGQk8KZfxbOkMQduUO3hL+o3+o
    J70hHwCwy/mM7Q9X/nB8W/CazKbkDuaqlmHDjwIDAQABAoIBAGTHeoBrUIArksr8
    EpLRyVVW+csJxtRgmcEwyJvDlJ7K0cQ41CjNjvvM4UQ4lWUYkjzg+eb1L4hMn5vE
    VP5tYgUx4vKtbTKZCRNJfztAp83IFYUhp2ZMmOuvHORrg5QuJfo+aNQU1es+wO8x
    ybcNhDCrwEwJeV6G5FM4Rp41HOxcmVg1heUqdncId5sLgZ5DqL/bElQ0Ur8cwkP6
    9d/IKHzCC2Emg2oXnaWYLrZCnf8UlPD+VaCjhhDiUaQMpcwCtsPP9J2LmsJkdDVq
    lFHrIz/49etkSeFdny6m5Ko1IM7xg8L2OEO4pSmWd1c+lPlGodltoOR4Gbpw5kWI
    a5QVntECgYEA94Hi8HqG3SO0lW/ux9tv0W4V+HioQNwPlfff7cfYr5SCUuJUov52
    7HFqdYGmcf4+QFcC9kILEqhXJmAWHIbL5ztSW1w07WMBoWaRWWQjKhlNR65V5LMO
    04mgJriteCfxRKVz7qGSNkxtXjW9D16GRnDObH7LsM3xLhZ1mWaOuw0CgYEAneDY
    nz/Gq2oGT1BgzgfX/1U22J/i36z2sJoBmYutWedE6Q37ALdAKW2Kcg+uYn0nyKPp
    msULH6BdfGPCyS/as3kpl9ZGnO9vvSIwvIwnb8+ijsijpgzP06d3MgKxryI4qnoW
    fzyWL+Z9+JDIKX+Ovdk5KdL8Ys2r5bZ4a6UIIgsCgYEAiuk9I1bBRm/5tD1kOIsy
    EMrGTfP6Cpz3qmW6KkwVk0W4dyhL9Eb5G+7znwurDSNycM3k/a1HZatRblpTTqNg
    4WBr8panpewBSEK2pQMMsV4N/4Ma9gaA9byyQ8k2os9YG/z4OiA4XX07jLqAZ5XX
    qsU5Na9cRtju2b0FN5lV1ikCgYEAmBbo19C6dJs45ONLvm7z97eBlIHCLzQSmeJi
    tCZIoxkV22VqbjAakU5DVsogdiSXVMQC4OP9aUQ1iwUXunRtPQP1u53ifIB0pkDv
    rlNeEmp18RL8A5TICN+FBhpuTYasDB582anmDNMAe5oOGVwWiHFgjhgAprX3aN21
    eA2Nni0CgYEA7PM1oM86XqPrNbZFKExMWlifGOoqervoqB/fm9rUJt14AvfyE5ST
    gt/UnvS43aI32Adw1nG991DskYLrQ8WcmirCZIbaxFreuM2aKgbpBqit4q1AFcp0
    eO2DcbvHyDU0aucSkJZAPYxXujFLWQ+3wnVnrz9eN+nqKiKvQxcHADs=
    -----END RSA PRIVATE KEY-----
    ```

=== "Public Key"

    ```text title=""
    -----BEGIN PUBLIC KEY-----
    MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAmKQLQ5xf0WdB9Wo1H0ah
    UlJfvQA2UyIOI4lmXiRUHudVCSl5zayH8R0aVY+rjFvpjNb4CUV8AOBk9W7dZ96W
    UF3zysFSUfFTVucPTdi1vZ416rhPFt9faJbYrV/zgLzo3pMxaooZZLY/e2U74+GJ
    x+Obd+MpBh55ZGFSpGzhEPivAhmt3qGdPvotsmDpl0BU1hzrUBP9Tlfk0AiQUtBW
    Bc9B69l7UQ+IfUnpjubVvw+uKW8ewODe5srfXMM+G4P8imxbZ5bE5M71HblyP5Ew
    pykGQk8KZfxbOkMQduUO3hL+o3+oJ70hHwCwy/mM7Q9X/nB8W/CazKbkDuaqlmHD
    jwIDAQAB
    -----END PUBLIC KEY-----
    ```


This JWT is generated by the OIDC provider.

## OIDC Provider

The EKS OIDC provider acts as the trust bridge between your Kubernetes cluster and AWS IAM. Here's what it does:

### Identity Provider 

1. Each EKS cluster gets a unique OIDC provider URL (like https://oidc.eks.us-east-1.amazonaws.com/id/EXAMPLED539D4633E)
2. This provider issues and signs JWT tokens for service accounts
3. The tokens contain claims about which service account is making the request

### Cryptographic Trust

1. The OIDC provider has a private key for signing tokens
2. It publishes the corresponding public key at /.well-known/jwks.json
3. AWS STS fetches these public keys to verify token signatures

### Token Contents

The JWT tokens include critical claims:

- **sub** (subject): `system:serviceaccount:namespace:serviceaccount-name`
- **iss** (issuer): The OIDC provider URL
- **aud** (audience): `sts.amazonaws.com`
- **exp** (expiration): Token validity period


### Trust Establishment
When you create an IAM role for IRSA:

```json
{
  "Version": "2012-10-17",
  "Statement": [{
    "Effect": "Allow",
    "Principal": {
      "Federated": "arn:aws:iam::123456789012:oidc-provider/oidc.eks.region.amazonaws.com/id/EXAMPLED539D4633E"
    },
    "Action": "sts:AssumeRoleWithWebIdentity",
    "Condition": {
      "StringEquals": {
        "oidc.eks.region.amazonaws.com/id/EXAMPLED539D4633E:sub": "system:serviceaccount:default:my-service-account",
        "oidc.eks.region.amazonaws.com/id/EXAMPLED539D4633E:aud": "sts.amazonaws.com"
      }
    }
  }]
}
```

### Why This Design?

**Security Benefits**:

- No AWS credentials stored in the cluster
- Tokens are short-lived and auto-rotate
- Fine-grained access control per service account
- Standard OIDC protocol that AWS already supports

**Operational Benefits**:

- No credential rotation needed
- Kubernetes-native (uses service accounts)
- Auditable through CloudTrail
- Works with existing IAM policies

The OIDC provider is essentially what makes it possible for AWS to trust that a request is genuinely coming from a specific service account in a specific EKS cluster, without requiring long-lived AWS credentials to be stored anywhere in the cluster.
