# Timekeeping

## `systemd-timesyncd.service`

Systemd has introduced their own SNTP implementation called timesyncd. Timesyncd only implements a subset of the NTP protocol, which is why it's called SNTP. It has been used since Debian "bookworm", and by extension, Ubuntu (which is downstream of Debian).

### `timedatectl`

This is a command that can be used to view the current status of time synchronization.

```
$ timedatectl timesync-status
       Server: 91.189.91.157 (ntp.ubuntu.com)
Poll interval: 34min 8s (min: 32s; max 34min 8s)
         Leap: normal
      Version: 4
      Stratum: 2
    Reference: 84A36001
    Precision: 1us (-24)
Root distance: 24.421ms (max: 5s)
       Offset: -15.315ms
        Delay: 48.943ms
       Jitter: 10.320ms
 Packet count: 9
    Frequency: -23.604ppm
```

## chrony

chrony implements the full NTP protocol and is often a replacement for timesyncd. A comparison of chrony against other implementations can be found [here](https://chrony-project.org/comparison.html).

## ntp

This is the reference implementation.

## OpenNTPD

https://www.openntpd.org/

