---
date: 2024-05-03
title: Python, you have problems.
description: Let's talk. You've got problems, a lot of them.
categories:
    - Programming
    - Python
---


It's been said that the key to any healthy relationship is the ability to see eye-to-eye on life's important issues. The approach one has to politics, to love, to religion, to animals and children (sometimes they're the same thing), and to those less fortunate in society are key metrics to consider when evaluating the strength of your compatability to a significant other. Well, I've been pondering my relationship with Python, and I'm realizing that we just aren't compatible anymore. It's not me, it's you.

<!-- more -->

I've programmed in Python for a long time. That is not to say I have been around for a long time, nor that the span of my career grants me a position in the same echelon as some of the true fabled engineering wizards of our time. In fact some of my coworkers have been software engineers for longer than I've been alive. Nonetheless, I've been around long enough to come to some stark conclusions about some serious shortcomings about one of the most popular languages in the world: Python. And boy are there a lot of them.

Before I dive into my treatise on Python, I should first dive into my background with it and provide a sort of disclaimer to current and future employers who may be looking at this post with a raised eyebrow.

## My History with Python

I was first introduced to Python in my college years during my work with the National Center for Supercomputing Applications. I learned Python because it had proven itself to be an invaluable tool when running large batch workloads on supercomputing clusters. You think you're going to create a huge meta-scheduler job pipeline in something like C? Ha! Good luck. One of my first proud accomplishments as a young, green grasshopper was writing a [TORQUE metascheduling package](https://github.com/TerraFusion/batch4py) in Python that took a DAG of TORQUE job definitions, performed a topological sort, and translated that graph into a tree of job dependencies in TORQUE. I also took pride (oh how cute of me) in some other small Python projects I authored like a [lightweight authentication wrapper over the Globus API](https://github.com/TerraFusion/globuslite). You know, just cute stuff that a college student might think is cool.

This dovetailed into a mostly Python position at Jump Trading where most of my work involved either system administration in Python, using tools written in Python, or creating large, hundred-thousand-job metascheduling pipelines that Extract/Transform/Load petabytes of live HFT market data. There, my Python chops were refined in a professional and high-stakes environment, allowing me to grow my practices and strategies to be an effective and high-functioning Python developer.

Thus for the longest time, my opinions of Python were glowing as I felt it had liberated me from the constraints of antiquated languages like C/C++. I balked at the idea of ever having to deal with stupid things like _typing_, or _memory management_, or _pointers_ ever again. I was free to do what I wanted when I wanted, just like a true red-blooded American. Who doesn't like a good dose of freedom?

## My Disclaimer

My disclaimer to all readers past, present, and future is that I do not _hate_ Python. Python has done more good for the world than almost any other language. It is a true force to be reckoned with and I enjoy using it in the environments where it shines. I'll continue to use Python for the foreseeable future not just because it's in high demand, but because it's still fun to me and it has its proper place.

With that said, let's begin.

## The Problems Begin

It's been said that all good things must come to an end. This is true for a lot of things, but not all. For example, wearing pajamas while working remotely doesn't have to end. Coffee doesn't have to end. The invariable spread of democracy doesn't have to end (Helldivers anyone? I'm just joking).

When it comes to my unconditional infatuation with Python, well, that indeed must come to an end.

### TypeError

Type errors, we all love them, we learn to get along with them. It's a fact of life, it happens. Let's say I wrote a stupid program like this:

```python title="stupid_type_error.py" linenums="1"
--8<-- "code/python_you_have_problems/stupid_type_error.py"
```

The keen observer may notice right away that this program is going to fail 14% of the time at [this exact line](#__codelineno-0-5). Why 14%? Well we're generating a random number between 0 and 99, and we're asking to run the `type_error` function whenever that number is divisible by 7. There are 14 such numbers between 0 and 99, which gives us, you guessed it, 14%.

Let's see what happens in the happy path:

```
running function with no type error
0
1
2
```

And now what happens when we have the unhappy path:

```
  File "/Users/landon/git/LandonTClipp/LandonTClipp.github.io/code/python_you_have_problems/stupid_type_error.py", line 22, in <module>
    main()
  File "/Users/landon/git/LandonTClipp/LandonTClipp.github.io/code/python_you_have_problems/stupid_type_error.py", line 16, in main
    type_error()
  File "/Users/landon/git/LandonTClipp/LandonTClipp.github.io/code/python_you_have_problems/stupid_type_error.py", line 5, in type_error
    for i in 123:
TypeError: 'int' object is not iterable
```

Well of course, the `int` type doesn't have the `__next__` magic method so it can't be used as an interable, so therefore it's not iterable. This kind of error would have been easy to catch in such a trivial program such as this, but imagine if you have a project with hundreds of thousands of lines of code and you just happened to have missed the one line that contains a type error. Well now you won't find this bug until you discover it in production, which is never fun.

There are, however, some strategies to overcome this. Some of them that I've seen:

#### Require 100% testing coverage

This has been done at various places, but sometimes it's not feasible because if you don't set this requirement from the inception of your project, getting up to 100% coverage can be like Sisyphus rolling a boulder up an infinitely long slope. This is also easily defeated if you do your tests wrong, like if you were to mock an external dependency incorrectly and you return a type that the real dependency doesn't actually return (hello `unittest.mock.Mock`). Of course unittest mocks often take a `spec` argument that allows the shape of your mock to be exactly like the real dependency, but this only works if your dependency correctly annotates your types, otherwise how can the Mock know what it's supposed to return? 

#### Static Type Checkers

Use a static typechecker like `mypy`. This is one of my favorite options, but this too is easily and often defeated. Take for example the [`pytest.mark.parametrize`](/blog/2024/04/23/python-table-driven-tests/#pytest-parametrized-decorator) decorator. `mypy` is not able to correctly inspect any typing issues in the decorator because it's just a weird f-ing way to implement table-driven tests. `mypy` also doesn't work in cases where, again, your external dependency you rely heavily on doesn't provide type annotations (which in my experience is quite a lot of them). When `mypy` works, it works fantastically, but the tool is limited by how well-written your code and your dependencies are. And please, let's not argue on whether or not you should use type annotations: you should.

Let's see what mypy says when we run it on our `stupid_type_error.py` program:

```
 $ mypy code/python_you_have_problems/stupid_type_error.py 
Success: no issues found in 1 source file
```

Oh, that's embarrasing actually. In my initial iteration of this post, I forgot to annotate my functions (tee-hee) so mypy by default will not perform type checking on it. Let me add `#!python -> None` as the return type, which tells mypy to check for types:

```
 $ mypy code/python_you_have_problems/stupid_type_error.py
code/python_you_have_problems/stupid_type_error.py:5: error: "int" has no attribute "__iter__"; maybe "__int__"? (not iterable)  [attr-defined]
```

Yeeeaaah, much better. But again, that again brings up the great point that if your external dependency forgot to add annotations, or if you're someone who has some weird thing against typing, then mypy will totally ignore it, or get it totally wrong.

Can we also just sit down for a second and appreciate the fact that tons of super talented people had to spend a large amount of their time actually creating `mypy` and supporting it through various iterations of the Python language because the language itself just doesn't have static typing? We're shoehorning a really important concept into a language that doesn't provide it. Props to everyone who develops and maintains `mypy`, it's a wonderful tool, but I posit that it shouldn't have to exist.

### Parallelism

Woo boy here we go. Let's get one thing straight: Python does _not_ have a decent parallelism story. Because CPython requires the use of a [Global Interpreter Lock](https://en.wikipedia.org/wiki/Global_interpreter_lock), that means that only one thread of execution within the interpreter process can run at a time. This is the source of all frustration and pain with parallelism in CPython, and it's probably never going to go away.

For some context, in most programming languages you are able to create multiple threads of execution that are able to run simultaneously. The benefit of this, as opposed to multiprocessing, is that the threads can share memory, which means that your data structures don't have to be serialized over a communication channel (often a unix pipe or socket). This lends itself to a much more efficient style of parallelism because the shared datastructure has much better locality properties, and it's much easier to keep it internally consistent. This improves latency, it reduces complexity, and by extension makes your system more performant and robust.

So what does Python do about this? Well, it provides us with something even worse than multiprocessing... and it's truly horrifying: _`asyncio`_

![Scary Hamster Thing](https://f005.backblazeb2.com/file/landons-blog/assets/images/blog/2024-05-03-python-you-have-problems/dun-dun-dun.jpg)

#### `asyncio`: The Concurrency

For those unfamiliar with `asyncio`, it is a Python package, created by the maintainers of CPython, that gives us a way to run multiple "coroutines" of execution within a single Python interpreter. Both asyncio and Python's `threading` package can be used to implement concurrency. `asyncio` makes use of the idea of a "coroutine" which can be thought of as a thread-like concept, but instead of being maintained by the OS, a coroutine is managed by Python itself. Coroutines are _not_ OS threads, but they behave in a similar way. `asyncio` also makes use of special keywords like `#!python await`, `#!python async def`, `#!python async for`, `#!python async with` that are used to inform the interpreter on what functions are coroutines, and how to interact with those coroutines. This makes it easier to reason about where your concurrent code can block because you've explicitly defined it with the `await` and `async` keywords.

`threading` differs from `asyncio` in that it uses actual OS threads to multiplex operations. With `asyncio`, you can reasonably submit hundreds of thousands, even millions of coroutines, because a coroutine is an internal concept in memory, not an actual OS thread. It would not be responsible, however, to treat threads the same way. Go ahead and try, see what happens.

If you're confused about the difference between `threading` and `asyncio` and `multiprocessing` and `concurrent.futures.ThreadPoolExecutor`, you're not alone. The glut of options at our disposal at first glance might make a layman believe that parallelism in Python is flexible and well-conceived, but it's actually quite the opposite. Not only is it confusing to understand the differences between these options, it's confusing to understand in what situations each solution is appropriate for. Let's not get too into the weeds, however. Let's instead focus on why `asyncio` is just plain awful.

#### `asyncio`: What Color Is Your Function

Bob Nystrom wrote a fantastic blog post called [What Color is Your Function](https://journal.stuffwithstuff.com/2015/02/01/what-color-is-your-function/). In this post, he describes a theoretical programming language that I'll name `Shithon` (pronounced _shy-thon_, or _shit-thon_, or _shit-hon_, whatever you prefer). This language has one particular feature: every function has a color. These are the properties of these colors:

1. Every function has a color.

    ```title=""
    blue_function doSomethingAzure() {
        // This is a blue function...
    }

    red_function doSomethingCarnelian() {
        // This is a red function...
    }
    ```

2. The way you call a function depends on its color.

    ```title=""
    doSomethingAzure()blue;
    doSomethingCarnelian()red;
    ```

3. You can only call a red function from within another red function.

    This can be done:

    ```title=""
    red_function doSomethingCarnelian() {
        doSomethingAzure()blue;
    }
    ```
    
    But this cannot:

    ```
    blue_function doSomethingAzure() {
        doSomethingCarnelian()red;
    }
    ```

4. Red functions are more painful to call.

5. Some core library functions are red.


I highly recommend you read the entirety of the post as it describes _in detail_ the exact problem that `asyncio` presents. As you may already tell, shithon is basically just Python asyncio. Take for example this async function:

```python
async def get_item_from_server(value: int) -> int:
    return await client.get(value)
```

The only way this function can be called is either in the context of another async function:

```python
async def get_item() -> int:
    return await get_item_from_server(value=5)
```

Or if called from a synchronous function, through `asyncio`:

```python
def get_item() -> int:
    return asyncio.run(get_item())
```

But wait! The asyncio docs say about `asyncio.run`: "This function cannot be called when another asyncio event loop is running in the same thread." So what do we do if there is already an event loop running somewhere? Well, you can't. If your synchronous function calls async code, or otherwise _adds_ an async function to the event loop, then by definition your function has to be `async` as well. Synchronous functions cannot call async functions if an event loop is already running in the same thread.[^1]

I'm not going to go into detail the intricacies of asyncio, for one because it's just incredibly frustrating to me personally, and for two because it's already been well-documented in various places on the interwebz. Here are some good resources:

- https://bbc.github.io/cloudfit-public-docs/asyncio/asyncio-part-5.html
- https://docs.python.org/3/library/asyncio.html
- https://realpython.com/async-io-python/

Going back to "What Color Is Your Function", the point can be distilled down to the fact that it's _really hard_ to call `async` functions from non-`async` because considerations have to be made about whether or not an event loop is already running, and if it is, how you should structure your code in a way that allows you to call `async` functions. It bifurcates your world into two realms, so much so that crossing the boundaries between these realms is _super_ difficult. Managing this boundary cannot simply be done through semaphores because the entire model itself is just bad. It all goes back to the GIL, baby.

### Packaging and Environment Management

Now let's get on to another horrendous aspect of the Python ecosystem: environment management. Let's say you make a fun little CLI tool that walks down a filesystem path and sums the total size of each layer, kind of like `ncdu`. Let's call it pydu, and let's say it looks exactly like ncdu:

```
$ pydu .
--- /Users/landon/git/LandonTClipp/LandonTClipp.github.io --------------
  232.9 MiB [################################] /ve                                                                                                                                                                                
  149.8 MiB [####################            ] /.git
   94.6 MiB [#############                   ] /code
   79.5 MiB [##########                      ] /docs
   14.8 MiB [##                              ] /.cache
    7.0 MiB [                                ] /.mypy_cache
```

pydu's project structure looks like any normal Python project: it's got a set of modules for shared code, an entrypoint for the CLI, and it pulls in some dependencies through a pyproject.toml file. The structure may look something like this:

```
pyproject.toml
src/main.py
src/__init__.py
src/module1.py
src/module2.py
tests/test_module1.py
tests/test_module2.py
```

You have a few choices at your disposal for how to package this. One of the default options is to publish it on the Python Package Index, or Pypi. There are many guides on how to do this, like [this one here](https://realpython.com/pypi-publish-python-package/). The gist of it:

1. You define all your package metadata in pyproject.toml, including your package name, the dependencies, any CLI entrypoints, and all other ancillary associated metadata.
2. You run `python -m build` to build a series of distributions, like:
    1. A source code distribution (which is just a tar.gz archive).
    2. A [Wheel](https://www.geeksforgeeks.org/what-is-a-python-wheel/), which is a binary file format that contains your code and all dependencies (both pure Python and pre-compiled). Wheels that contain platform-specific dependencies are called Platform Wheels (not surprisingly) because they will contain pre-compiled code that is meant for specific platforms.

This is all fine and dandy, and in fact my criticisms of Pypi and the associated build tools are quite minimal. My main point of ire lies downstream from when you try to download and install these packages. Here's where it gets _really_ fun.

So, let's say you want to now download `pydu`. What would you do?

#### pip

This option is the "default" choice for installing packages. This looks something like:

```
python3 -m pip install pydu
```

`pip` will look on PyPI (or any other package index you've configured) for `pydu` and install the appropriate distribution for your platform, whether that's a source install (if you've specifically requested it) or one of the wheels. By default, this will attempt to install in a site-wide location, depending your [site-specific configuration](https://pymotw.com/2/site/). However, this almost immediately becomes a problem if you've got multiple CLI tools that have common dependencies: what if one CLI tool you've installed depends on `foo>=1.5.0` but another CLI tool depends on `foo==1.3.0`.  It's possible to get into situations where the installation of one CLI tool will break the dependencies of a pre-existing CLI tool, because in essence what is happening is that the dependencies are dynamically linked at runtime. This issue only becomes more pronounced the more projects you have installed on your host. 

What's ~~the~~ a solution to this madness? Well, virtual environments of course.

#### [virtualenv](https://docs.python.org/3.10/tutorial/venv.html)

To get around the issue of the dependencies in your site-wide install location, you can make a virtual environment! Woo-hoo, this is fantastic. Hooray.

```
$ python3 -m venv ve
$ source ve/bin/activate
$ which python3
/Users/landon/git/LandonTClipp/LandonTClipp.github.io/code/python_you_have_problems/ve/bin/python3
```

This creates a directory where all dependencies will live:

```
$ ls ve 
bin        include    lib        pyvenv.cfg
```

So whenever you `pip install` something, it gets placed here instead of your site-wide location.

```
$ python3 -m pip install httpx
$ ls ve/lib/python3.12/site-packages 
anyio                      certifi                    h11                        httpcore                   httpx                      idna                       pip                        sniffio
anyio-4.3.0.dist-info      certifi-2024.2.2.dist-info h11-0.14.0.dist-info       httpcore-1.0.5.dist-info   httpx-0.27.0.dist-info     idna-3.7.dist-info         pip-24.0.dist-info         sniffio-1.3.1.dist-info
```

This solves the issue of having too many conflicting dependency requirements throughout all the projects you're managing, so problem solved, right? Wrong.

What if we wanted to install _two different projects_ that have conflicting dependencies? Well god dammit, that's just not possible without some serious finagling. The thing you have to realize is that a lot of these dependency hell headahces come from the fact that when Python installs a package, _only one version of that package can be installed at a time_.[^2] This makes sense in the context of a singular project that has an internal dependency inconsistency, but it doesn't make sense when you're running multiple CLI applications. (1) Each CLI application does not and should not have to coordinate with other, completely separate CLI apps. Yet Python is opinionated in that sense that packages _shall be treated as dynamically-loaded and shared dependencies_.
{ .annotate }

It turns out that pip is just a total mess to use because it doesn't handle multi-user or multi-project environments well at all. This is where other projects have come to the rescue, like poetry or flit.

1. And to be clear, it doesn't even have to be CLI applications. This applies for _any_ conflicting dependencies whether that arises from a development environment, or an install of a CLI app, or anything where two packages depend on conflicting versions of a dependency.

#### [pyenv](https://realpython.com/intro-to-pyenv/)

So far we've only talked about installing _packages_, but we have not yet talked about whether or not the version of Python running on your local system is actually capable of running the packages. Or in other words, what if your package used a language feature from Python 3.12, but you only have Python 3.8 available on your system? Well lucky for you, there's _pyenv_.

There's a whole set of steps you must go through to get pyenv installed, but for what it's worth, it does in fact work, but it poses yet another hurdle that users of Python must overcome if they find their system-provided Python is too old. I won't go into the details of how it works, but I just wanted to mention it.

#### [nuitka](https://nuitka.net/)

Let's say for example, you went to your boss one day and you said "hey boss man, I'm sick and tired of placing all these dependencies down on disk. Why can't we just package our CLI tool as a single executable and in that executable include both the Python interpreter itself and all of our project's dependencies?"

Your boss would say: "great idea Bob, why don't you give nuitka a shot?"

So you run along to CI/CD-land and you introduce nuitka into your workflow so that instead of producing a Wheel or a source archive, you produce a single, beautiful executable that can be shipped as a single blob to your customers. This solution works great for a while, but after a certain point you realize that nuitka gets slower and slower, and after some digging you realize that the more dependencies your project gains, the slower nuitka gets. In fact it gets so slow that you find your builds taking tens of minutes, sometimes more! In addition to your testing infrastructure, you find that from end-to-end it's now taking almost an hour or more just to produce a single artifact!

Of course, you could spend your days optimizing Nuitka and figuring out where you can prune unnecessary dependencies, but this takes time and energy, and I doubt your company enjoys paying you gobs of money to figure this out when you could have been focusing on building products.

There are a few other projects like nuitka as well like:

1. pyinstaller
2. pyoxidizer
3. py2exe
4. py2app

Pick your poison.

#### Misc

There are so many other dependency management tools that try, with varying levels of success, to do what `pip` does but better. Some examples:

1. conda
1. poetry
1. pipenv
1. flit
1. ???

I could spend a whole week writing out the differences of these all, but the point I want you to take away from this is that dependency management in Python is truly in a state of, absolute, unmitigated, unqualified cluster-fuckery. And yes I'm sure some of you will angrily write to me "but you can do this one cool thing and it's super easy and it works so well for me." I'm sure that works for you, but it doesn't take away from my point that there are _so many_ ways of doing package management in Python and there is no universally accept way, or limited set of ways, of doing it. This all comes down to the fact that the Python language itself doesn't provide a sufficient, cross-platform, sustainable method for us. _All_ of the methods out there exist because the whole model of how Python does package distribution, with its opinionated take that packages should essentially be shared libraries (`.so` for you Linux dorks) whose version is not part of the on-disk namespace, is wholly insufficient. 

The result of this is that the open source community, as well-intentioned and exuberant as it is, is left to fill in the void with dozens of completely separate implementations that don't inter-operate and don't agree on basically anything. Don't you think this is an enormous waste of human ingenuity? Don't you think this is such a silly thing to fight over? All of these bright, intelligent developers working on these projects could have been working on sending rockets into space or curing cancer, but instead we're focusing on how to handle dependencies in Python.

There is no other word for this but absolute madness.

## My Guiding Principles for Evaluating Languages

Python, while you have taught me a great deal about software engineering and have made me a better professional, a lot of the things you taught me were taught because you just suck at a lot of things. You're flaky, you fail when I don't expect it, and you always surprise me with another one of your "quirks" that end up biting me in the butt. There are a lot of people that love you and that have tried to fix you, but maybe you're just not meant to be fixed.

Here are some of the things I learned about what a good language should have:

### Strict Typing is Good

Python, as we know, is duck-typed. If it looks like a duck, swims like a duck, and quacks like a duck, then it's probably a duck. Variables in Python are pointers to underlying objects. The variable itself does not have a type. It may be pointed to anything and it can be reassigned to any type at any point in time.

What this means, essentially, is that there is no protection against accidentally using the wrong type. There are tools that might warn you (even though those tools can be fooled), but ultimately there still exist categories of type errors that you won't be made aware of until you actually run the damn thing.

Strictly typed languages on the other hand will tell you _immediately_ if something is wrong. They won't even let you run the program, because syntactically, it's not valid. This is an incredibly important feature: you want your program to fail fast and fail hard so that you have no opportunities to send your code to production until the issue has been fixed.

Python, in this category of errors, has the approach of "fail hard only if you happen to test this one particular line of code, and if you're not diligent enough to test every single god damn line of code, well sucks to suck."

### Garbage Collection is Good (where real-time latency isn't necessary)

I love collecting garbage. Er, I mean, I love it when my languages collect my garbage. Python does this so well, in fact, that allocation or deallocation is rarely something that ever comes to mind. Go also does this really well, as do most GC languages. This is, in fact, is one of the things I love about Python so, good job Python.

### Function Coloring is Bad

All functions should be callable from all other functions. Bifurcating your language sucks and it causes endless frustration. This is why asyncio is an awful idea, at least in the way it was implemented. We have to remind ourselves that asyncio is necessary because language itself was not designed with proper thread-based parallelism in mind. If CPython didn't have a GIL and its internal data structures were thread-safe, true thread-based parallelism in Python would be a breeze and function coloring would thus become unnecessary. 

### GILs are bad

There are a number of notable exapmles of interpreted languages, or implementations of Python, that lack a GIL:

- IronPython
- IronRuby
- Jython
- Tcl
- Pike

While this blog is taking a massive dump on Python, it should be noted that a lot of popular packages like pandas/numpy allow you to run data processing workloads outside of the interpreter in native C/C++, which means they are _super_ fast and don't have the GIL limitation. That's great and works quite well for the domains that justify the cost pulling in those heavyweight dependencies.

### Composition is (Usually) Better Than Inheritance

This is a philospohy recently championed (but certainly not invented) by Go. Go does not have true inheritence in its OOP model. It favors composition, which essentially means that instead of being able to inherit attributes and functionality, you create an attribute on your object that stores the dependency. This reduces the complexities involved with things like multiple inheritance and helps discourage needless abstractions. It's also quite telling how an entire diagraming language, UML, was created specifically to aid us in understanding inheritance models. Composition, on the other hand, shouldn't require diagramming because the object relationships can almost always be described in simple English.

This is a whole blog post unto itself so I will not enumerate all my thoughts, but here are some suggested readings:

- https://en.wikipedia.org/wiki/Composition_over_inheritance
- https://www.digitalocean.com/community/tutorials/composition-vs-inheritance
- https://en.wikipedia.org/wiki/Multiple_inheritance#The_diamond_problem

## Parting Thoughts

Hopefully by this point, I've successfully enumerated my numerous complaints with Python. And if you've made it this far, congratulations, you either totally agree with me, you learned something new, or you've become so miffed that you can't wait to leave a comment telling me how stupid my arguments are. In any scenario: well-done.

Any time a huge amount of complexity is introduced into a system to solve some fundamental problem, that usually means that the foundational model on which that system depends is inherently flawed. In Python's case, package management, lack of strict typing, its GIL, its concurrency primitives, are all examples of things that are just foundationally bad. Their existence necessitates a huge amount of complexity in the Python ecosystem that should have, and could have, been solved by changing the underlying model, but for reasons that I totally understand, at this point in Python's life, doing such a thing would be totally infeasible. Just look at how painful the Python 2->3 conversion was, we're still dealing with the affects of it 16 years later!

It is totally absurd to me how much talent is wasted in solving these kinds of problems. In my view, the only reason why these problems are worth solving at all is because Python's momentum is so great that asking people to use something else is often just not practical. If it had obtained less popularity, my reaction would be to totally abandon the language altogether because these issues are real dealbreakers from a reliability and time efficiency perspective. The proliferation of packaging tools, of the existence of asyncio and function coloring, the large number of different implementations of the language itself (I mean come on, _seriously?_), all point to the fact that what was given to us in the reference implementation of the language (obviously, CPython) is just simply inadequate. So much incredible talent, creating admittedly incredible solutions, has been spent working around what is fundamentally a big stinky :poop:.

It should also be noted that one of Python's main selling points is that it's so easy to use, so much so that it's often the first language developers learn. This is only a shallow marketing tactic because once you peel back the veil and see the man behind the curtain, you realize how unfriendly becomes. This is in fact a common complaint amongst newcomers that the proliferation of tooling, and the lack of any real standard for just about _anything_, can make it difficult to become effective at the language in a professional environment.

It's also my general stance that languages are simply a means to an end. My main priority is not being an expert in a specific language, but to be an expert in delivering business results. When I find myself fighting with and being confused at a tool, it either means I'm just too stupid to understand the tool (which very well may be true), or the tool itself is just inherently confusing. In either case (me being dumb or the tool being dumb), it's serving as an obstacle towards my end goal. I certainly don't think I'm too stupid to understand Python because I have indeed used it to create huge, production-grade, business-critical applications in a number of domains. It's just that I think the complexity I was required to wrangle is not justified by the benefits of what Python offers, especially in comparison to other modern languages today. 

I think it's great that some people really _really_ love Python, so much so that they market themselves as ~~Python experts~~ Pythonistas. That expertise is clearly in high demand and I'm by no means attempting to diminish the importance of that, nor am I try to say that people are _wrong_ for enjoying Python. My only stance, enumerated ad nausaem in this post, is that businesses need to think critically about these shortcomings before deciding they want to accept Python and all its baggage, especially when there are a number of better languages out there for backend-y kind of work (cough \*Go\* cough).

[^1]: If I am somehow wrong about this, please let me know! I could not find a way to do it.
[^2]: Per environment
