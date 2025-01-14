---
icon: octicons/clock-16
---

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

![](https://media.fs.com/images/community/erp/6R5Yb_3rAfAa.jpg)

Network Time Protocol gives millisecond, and sometimes sub-millisecond, level of precision. It's based off of software timestamping which contributes to the reduced precision as compared to PTP. This is the standard implementation that is used when millisecond-level precision is acceptable.

## ptp

<div class="grid cards" markdown>
- ![](https://media.fs.com/images/community/erp/rsMQn_1tYnh6.jpg)
- ![](https://moniem-tech.com/wp-content/uploads/sites/3/2023/02/Master%E2%80%93Slave-Hierarchy-in-PTP.png)
</div>

Precision Time Protocol is a much more accurate form of network timekeeping, capable of reaching nanosecond and sometimes sub-nanosecond precision. It is typically driven off of GPS signals into a datacenter and replicated through a series of boundary clocks. By default it uses multicast routing. Managing PTP is a more complex protocol. It can be used with either software or hardware timestamping.

The PTP protocol uses a multi-step, two-way message exchange that allows clocks to account for network latency.

## OpenNTPD

https://www.openntpd.org/

