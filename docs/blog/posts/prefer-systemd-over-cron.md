---
date: 2023-06-22
categories:
  - sysadmin
title: Prefer Systemd Timers Over Cron
draft: true
---

Prefer Systemd Timers Over Cron
================================

![systemctl list-timers command output terminal](/images/prefer_systemd_timers_over_cron/Screenshot 2023-06-22 at 1.22.59 PM.png)

Systemd administrators often find themselves needing to run services on their bare-metal machines. Services can be broken down into roughly two broad categories:

1. Long-running services that are started once and will run for the lifetime of the machine.
2. Short-running services that are started at least once and will run for a short amount of time.

Long-running services comprise of the majority of the services in use by Linux. One of the challenging aspects of long-running services in a production environment is the dual question of monitoring and reliability: 

1. How do you know if your service is running? In other words, how do you query its process state?
2. How will you be alerted if your service dies (especially in ungraceful deaths where UDP logs might not have a chance to make it off the host)
3. How do you handle automatic retries should the service die?
4. How do you enable automatic start-up of your services when a machine boots, and how do you make them start up in the right order (assuming inter-service dependencies exist)?

<!-- more -->

??? note "containers are great too"
    This post only looks at non-virtualized, non-containerized services. Many excellent solutions exist if you have a container management system like Kubernetes or any of the cloud-hosted systems like Amazon ECS.

Running short-lived services as a cron job
--------------------------------------------

The default answer most system admins give is to simply create a cron job. cron is a simple utility in Linux for running commands periodically as defined in a cron table, or crontab for short. cron itself runs as a systemd daemon in RHEL Linux distributions and periodically polls all available crontabs on the system.

```yaml title=""
0 16 * * * /usr/bin/echo "hello world" # (1)!
0 12 * * 6 /usr/bin/curl https://www.google.com >/tmp/google_html.txt &2>/dev/null # (2)!
```

1. This line specifies that the command should run at 16:00 every day of the month, on every month of the year, and on every day of the week.
2. This line specifies that the command should run at 12:00 every day of the month, on every month of the year, but only on Saturdays. Note that the day-of-the-week specifier is AND-ed with the day-of-the-month specifier.


### Benefits

1. This requires minimal setup. Most linux distributions pre-install crond which allows you to define user-level crontabs with no prior configuration.
2. It's easy to use. The line in the crontab is exactly what will be run. There is no templating system in use, so you generally won't have to worry about escaping special values.

### Drawbacks

1. There is no easy way to determine if the command completed successfully.
2. There is no easy way to know if the command was run at all.
3. Log management is a pain. Your only solution for logging stdout/stderr is to redirect it to a file, however you'd also need to ensure you rotate the logs to ensure the log's size is bounded.
4. There is no way to specify if you want the command to be retried, should it fail.
5. It's not easy to get a quick glance of how long each line has until its next execution. Reasoning about all of your cron lines as a whole is basically impossible.


These drawbacks make a crontab wholly unsuitable for a reliable production system.

Running short-lived services as a systemd timer
--------------------------------------------

Let's take for example a service that needs to periodically scrape a user's home directory and send the total list of files to an external logging backend.

```bash
$ find . -type f
./file2.dat
./subdir/file3.dat
./file1.dat
```

We can use `ncdu` to give us a nice JSON representation of the file tree with each directory element's size. We pipe the JSON output to `jq` to format it into a more readable state.

```bash
$ ncdu -x -o - . |& jq .
[
  1,
  2,
  {
    "progname": "ncdu",
    "progver": "2.2.2",
    "timestamp": 1687457234
  },
  [
    {
      "name": "/Users/landonclipp/git/LandonTClipp.github.io/example_dir",
      "asize": 160,
      "dev": 16777231
    },
    {
      "name": "file2.dat",
      "asize": 2097152,
      "dsize": 2097152
    },
    [
      {
        "name": "subdir",
        "asize": 96
      },
      {
        "name": "file3.dat",
        "asize": 1048576,
        "dsize": 1048576
      }
    ],
    {
      "name": "file1.dat",
      "asize": 1048576,
      "dsize": 1048576
    }
  ]
]
```

