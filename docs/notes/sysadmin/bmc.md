---
icon: octicons/server-16
---

# BMC

Baseboard Management Controller is an external computing system that lives on the same chassis as a server. It provides a remote monitoring solution for the host hardware. Dell's BMC is called iDRAC, but most other datacenter server manufacturers provide their own.

## `ipmitool`

This is a tool on most Linux distributions that can be used to interact with the BMC through the IPMI interface.

### Get BMC Address

```
ipmitool lan print
```

### Get Power Status

```
ipmitool -U ADMIN -H 10.8.54.133 -P <PASS> chassis power status
```

## Redfish

Redfish is an HTTP service that runs on a lot of modern BMCs that provides you with a REST endpoint for querying the BMC.

### Authentication

You must authenticate with Redfish to obtain an Auth token:

```
curl --insecure -H "Content-Type: application/json" -X POST -D headers.txt https://${bmc_ip}/redfish/v1/SessionService/Sessions -d '{"UserName":"admin", "Password":"password"}
```

A [Python script](https://github.com/LandonTClipp/dotfiles/blob/main/bin/redfish_auth.py), created by one of my coworkers at Lambda, can help automate this process.

