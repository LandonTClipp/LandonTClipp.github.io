---
title: CLI Tools
icon: octicons/terminal-16
---

## pdsh

Used to run commands to multiple nodes at once. By default uses `/etc/genders` to determine the hosts to run the commands on.

```
$ pdsh -g gpu "echo hello"
node-017: hello
node-018: hello
node-020: hello
```

## dshbak

Often used in conjunction with pdsh to aggregate command output.

```
$ pdsh -g gpu "echo hello" | dshbak -c
----------------
node-[001-032]
----------------
hello
```

## strace

strace can be used to inspect the system calls being made by an application.

```
$ strace -f cat /tmp/hello.txt |& egrep 'hello'
execve("/bin/cat", ["cat", "/tmp/hello.txt"], 0x7fffffffc1a0 /* 115 vars */) = 0
openat(AT_FDCWD, "/tmp/hello.txt", O_RDONLY) = 3
read(3, "hello world\n", 131072)        = 12
write(1, "hello world\n", 12hello world
```

## View listening ports

=== "netstat (linux)"

       ```shell
       $ netstat -ntulp
       ```

=== "netstat (MacOS)"

       ```shell
       $ netstat -anv
       Active Internet connections (including servers)
       Proto Recv-Q Send-Q  Local Address          Foreign Address        (state)      rhiwat  shiwat    pid   epid state  options           gencnt    flags   flags1 usscnt rtncnt fltrs
       tcp4       0      0  127.0.0.1.8001         127.0.0.1.60348        ESTABLISHED  407878  146988  34323      0 00102 00000004 000000000027587a 00000080 01000900      1      0 000001
       tcp4       0      0  127.0.0.1.60348        127.0.0.1.8001         ESTABLISHED  408300  146988  24807  24804 00102 00020000 0000000000275879 10180081 00080900      1      0 000001
       tcp4       0      0  192.168.50.89.60347    140.82.113.5.443       ESTABLISHED  131072  132432  24807  24804 00102 00020000 0000000000275878 10180081 00080900      1      0 000001
       tcp4       0      0  192.168.50.89.60344    149.137.136.16.443     ESTABLISHED 1511016  131768  24807  24804 00102 00020000 000000000027585b 10180081 00080900      1      0 000001
       ```

=== "lsof"

       ```
       $ sudo lsof -i -P -n | grep LISTEN
       systemd         1             root   79u  IPv4  116762      0t0  TCP *:111 (LISTEN)
       rpcbind      2942             _rpc    4u  IPv4  116762      0t0  TCP *:111 (LISTEN)
       systemd-r    2947  systemd-resolve   13u  IPv4  142435      0t0  TCP 127.0.0.53:53 (LISTEN)
       sshd         3175             root    3u  IPv4  273290      0t0  TCP *:22 (LISTEN)
       rpc.statd    3329            statd    9u  IPv4   18403      0t0  TCP *:35287 (LISTEN)
       virt-expo  135395             root    9u  IPv4  334360      0t0  TCP *:9411 (LISTEN)
       node_expo  178456         node-exp    3u  IPv4  593613      0t0  TCP *:9100 (LISTEN)
       systemd_e  178515 systemd-exporter    3u  IPv4  624258      0t0  TCP *:9558 (LISTEN)
       ```

## uniq

Use `uniq -c` to get counts of each occurence. This is more commonly used like `cat file.txt | sort | uniq -c`.

## sort

Can also use sort with `-u`, which is equivalent to `sort | uniq`.

## Describe a file handle using lsof

This is from a useful gist here: https://gist.github.com/tonyc/1384523?permalink_comment_id=3334070

```
root@ml-512-node-031:/home/ubuntu# lsof -p 51953 -ad 100
COMMAND   PID   USER   FD   TYPE     DEVICE SIZE/OFF NODE NAME
python  51953 ubuntu  100u  IPv4 2717208220      0t0  TCP ml-512-node-031:33914->ml-512-node-031:58209 (ESTABLISHED)
```

## tmux

### Layouts


Cycle through layouts: ++"PREFIX"+b++

Layouts:

- even-horizontal - All panes are arranged side by side, with equal width
- even-vertical - All panes are stacked on top of each other, with equal height
- main-horizontal - One large pane on top, with smaller panes arranged horizontally below it
- main-vertical - One large pane on the left, with smaller panes arranged vertically to the right
- tiled - All panes are arranged to use the available space as efficiently as possible, with roughly equal size

From the prompt (++"PREFIX"+colon++):

`select-layout even-vertical` or ++"PREFIX"+alt+2++

and

`select-layout even-horizontal` or ++"PREFIX"+alt+1++
