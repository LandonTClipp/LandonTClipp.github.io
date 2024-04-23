---
date: 2024-04-23
title: Python Table-Driven Tests
categories:
    - Programming
    - Python
---

Table-driven tests (TDT) are a popular form of unit testing that allow you to rapidly iterate over multiple different inputs to your system in a way that is easily readable and understandable. This post will show my approach to TDT in Python and why all the current implementations of it are insufficient.

<!-- more -->

A table-driven test is a kind of unit test whereby a single body of test code is re-used over multiple test instances. The parameters to the test code are provided in a list whereby each entry contains all the relevant information to test the behavior you desire. Take for example this function:

```python title="path.py" linenums="1"
--8<-- "code/python_table_driven_tests/path.py"
```

This function does the following:

1. Attempt to return `path` relative to `root`.

    a. If `root` is `None`, set `root` equal to the home directory.

3. If this cannot be done:
   
    a. Remove the leading `/` component from path

    b. Return `#!python root + path`.

## Standard Approach

A common approach to testing this function is to create one test per case:

```python title="test_path_individual_functions.py" linenums="1"
--8<-- "code/python_table_driven_tests/test_path_individual_functions.py"
```

The issue with this approach is that it's a lot of boilerplate: we have to define one function per test, call our function under test, and assert the value is what we expect. It's also moderately ugly to look at because while each function is testing the same thing, they are dispersed across the file and this makes it harder to find a specific case that you may care about. This approach may be tractable For functions that have a small number of behavior permutations, but it quickly gets out of hand when the number of cases for your function increases beyond a small handful.

## `pytest` Parametrized Decorator

Another common approach that is supported in the Python community is to use [`pytest.mark.parametrize`](https://docs.pytest.org/en/7.0.x/how-to/parametrize.html). This would look something like this:


```python title="test_path_parametrize.py" linenums="1"
--8<-- "code/python_table_driven_tests/test_path_parametrize.py"
```

Already, we can see multiple benefits from this style of test:

1. All testcases reuse the same body of code
2. Each testcase is just a blob of data: we can clearly see what the inputs are to the test case without having to parse through internal details of the test logic. This gives us a higher-level picture of each case.
3. It has the potential to be less lines of code. Our toy example here is not a great representation of this point, but you can imagine when you have more enumerations that over time the parametrized version will be more compact and readable.

There are, however, some major disadvantages to `pytest.mark.parametrize`:

1. Type checking tools like `mypy` cannot properly detect when an argument to your testcase is of an incorrect type. For example, if we change the first testcase to be `#!python         (Path.home().joinpath("foobar"), 1, Path("foobar"))` (the `root` argument is an `int` instead of `Path | None`), mypy will not complain:
    ```bash
    $ mypy .
    Success: no issues found in 4 source files
    ```
1. The arguments to each testcase are positional instead of keyword arguments, which makes it difficult to remember which part of the tuple corresponds to what argument of the test case.
1. Because the arguments are positional, you _have_ to specify something even in the case you want to rely on default values.
1. Adding a new parameter to the test requires modifying all previously existing cases (yuk!). Changing the argument definition:
    ```python
    ("path", "root", "expected", "foo"),
    ```

    causes pytest to barf:

    ```
    _____ ERROR collecting test_path_parametrize.py _____
    test_path_parametrize.py::test_make_relative: in "parametrize" the number of names (4):
    ('path', 'root', 'expected', 'foo')
    must be equal to the number of values (3):
    (PosixPath('/Users/landon/foobar'), 1, PosixPath('foobar'))
    ```

    This point, in my opinion, is the most egregious.

## Go-like Table Driven Test

So what's the answer? Well, being a self-described Go language dogmatist, I like to draw on the Go way of doing things. Instead of relying on third-party packages like `pytest` and awkward implementations of TDT, we can utilize stdlib Python APIs:

```python title="test_path_tdt.py" linenums="1"
--8<-- "code/python_table_driven_tests/test_path_tdt.py"
```

1. We use a `unittest.TestCase`-style test so we can make use of a feature called `subTest` as you'll see below.
2. Create a dataclass that describes, and types, the parameters used in the subtest.
3. There's a few things going on here. The first thing happening is that we're calling [`self.subTest`](https://docs.python.org/3/library/unittest.html#unittest.TestCase.subTest) which is a context manager that creates, you guessed it, a sub-test. This is reported as a separate, inferior test that runs under the purview of the outer `test_make_relative` unit test.
    
There are a few key points to note here that make this style of test so powerful:

1. The use of the `subTest` context manager makes reporting the exact arguments used in the test easy to parse. `subTest` takes an arbitrary number of keyword arguments that are used when reporting test failures. We format our `Params` dataclass into a python dictionary, then use the unpacking operator `**` to send the attributes of `Param` as individual keyword arguments. Take for example this failed test:

    ```
    ====================================== short test summary info ======================================
    (expected=PosixPath('haha'), path=PosixPath('/Users/landon/foobar'), root=None) SUBFAIL test_path_tdt.py::TestMakeRelative::test_make_relative - AssertionError: assert PosixPath('haha') == PosixPath('foobar')
    ==================================== 1 failed, 9 passed in 0.05s ====================================
    ```

    !!! tip
    
        To get this output, you need to install the `pytest-subtest` plugin.

2. Adding a new parameter doesn't require modifying every existing test case. We can simply provide a default in the dataclass and override this for any new test. Take for example:

    ```python
    class TestMakeRelative(TestCase): 
        def test_make_relative(self) -> None:
            @dataclasses.dataclass
            class Params:
                path: Path
                expected: Path
                root: Path | None = None
                new_param: int = 0

            for test in [
                # ...
                Params(
                    path=Path.home().joinpath("foobar"), 
                    root=None, 
                    expected=Path("foobar"), 
                    new_param=1,
                ),
            ]:
                with self.subTest(**dataclasses.asdict(test)):
                    assert test.expected == make_relative(path=test.path, root=test.root)
    ```

3. `mypy` is able to properly detect incorrect value assignments. For example if we do:

    ```python
    Params(
        path=Path.home().joinpath("foobar"), root="wtflol", expected=Path("foobar")
    ),
    ```

    mypy tells us:

    ```shell
    $ mypy .
    test_path_tdt.py:17: error: Argument "root" to "Params" has incompatible type "str"; expected "Path | None"  [arg-type]
    ```

## Conclusion

Opinion alert: I've used `pytest` quite extensively in my professional career but I often disagree with the approach it takes to things. `pytest.mark.parametrize` is yet another one of those things that feels useful on the surface, but it has many shortcomings that cause endless pain and suffering. They tried to solve the question of table-driven tests but did it in a nasty way that for me engenders sadness and dismay. 

The methodology I propose here is not new in the broader software engineering community, but it is one I've rarely seen utilized in the Python community. TDT in the Go community [is everywhere](https://dave.cheney.net/2019/05/07/prefer-table-driven-tests) and whether you like it or not god dammit, you'll learn to like it because it's just the right mixture of elegance and power.

Let me know your thoughts below, and feel free to share your opinions on pytest. Also, don't you dare mention pytest fixtures in the comments. I _will_ rage :laughing:
