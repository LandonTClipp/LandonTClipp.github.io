---
title: Bash
---

Bash
=====

Pipelines
------

Pipelines are the bread and butter of Unix scripting. The stdout of a process can be redirected to the stdin of another process like so:

```
cat out.csv | grep cookies | cut -d',' -f 2 | sort | uniq -c
```

You can also include stderr in the pipe as such:

```
cat out.txt |& grep cookies
```

Stdout/Stderr redirection
---------------------------

Redirects just stdout:

```
echo hello > hello.txt
```

Redirects stdout, then sets stderr to point to what stdout is pointing to:

```
grep chocolate cookies.csv > choco_cookies.csv 2>&1
```

This is more succinctly written as:

```
grep chocolate cookies.csv &> choco_cookies.csv
```

### Here String

```bash
$ foobar <<<"hello world"
```

Here, the string `"hello world"` is redirected to `foobar`'s stdin.

### Here document

```bash
$ foobar << END
> hello world
> today is a great day
> goodbye
> END
```

A heredoc is a multi-line string that is treated as a file literal. The heredoc itself is redirected into the command's stdin, as can be seen in this example:

```bash
$ strace cat << EOF                               
> hello world                                                
> EOF
...
read(0, "hello world\n", 131072)        = 12
write(1, "hello world\n", 12)           = 12
```

Backgrounding a Terminal Process
--------------------------------

Consider if we want to grep for the lines in  `cookies.csv` that contain `snickerdoodle`:

```
grep snickerdoodle cookies.csv
```

But we find this is taking a long time to complete because, as it turns out, our cookie addiction is 5 TiB large. You can send the ++ctrl+z++ combination to send the SIGTSTP signal (terminal stop), followed by executing `bg` to send the stopped process to the background.

```
$ bash /tmp/long_cookies.sh
snickerdoodle,cookies
snickerdoodle,cookies
snickerdoodle,cookies
^Z
[1]  + 82721 suspended  bash /tmp/long_cookies.sh
$ bg
[1]  + 82721 continued  bash /tmp/long_cookies.sh
snickerdoodle,cookies                                                                                                                 
$ jobs
[1]  + running    bash /tmp/long_cookies.sh
```

The `jobs` command indicates the background jobs being run. You can use the `fg` command to bring a specific background job to the foreground.

```
fg %1
[1]  + 83010 running    bash /tmp/long_cookies.sh
snickerdoodle,cookies
snickerdoodle,cookies
^C
```


Process Substituion
---------------

Process substition allows the stdout of a process to be used as a file for the input of another process.

```
cat <(echo hello)
```

Gets "rendered" as:

```
cat /dev/fd/11
```

Where `/dev/fd/11` is a file descriptor referencing the stdout of the `echo` command.

You may also use process substitution to write to a file and pipe that file as an input to the stdout of another process:

```
ncdu -o >(jq .)
```

This is roughly equivalent to:

```
ncdu -o out.json  &
jq . < out.json
```

Double Bracket Test `[[`
--------------------

This is an extended form of test.

### String Truthiness

Non-empty strings in `[[` are considered falsey:

```
 $ [[ "foobar" ]] && echo true
true
```

Versus:

```
 $ [[ "" ]] && echo true 
```
