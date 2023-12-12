---
title: systemd
---

## Verify a unit file is valid.

```
$ systemd-analyze verify my-server.service 
$ echo $?                                                            
0
```

## Verify a systemd calendar spec

```
$ systemd-analyze calendar '12:*:*'
Original form: 12:*:*
Normalized form: *-*-* 12:*:*
    Next elapse: Tue 2023-12-12 12:00:00 CST
       (in UTC): Tue 2023-12-12 18:00:00 UTC
       From now: 13h left
```
