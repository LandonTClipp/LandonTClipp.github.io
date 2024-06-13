---
title: Python
---

## Floor Division

`5 // 2` will divide 5 by 2 and round the result down to the nearest integer.

## Lists

Lists are implemented as an array of pointers. 

https://github.com/python/cpython/blob/5c22476c01622f11b7745ee693f8b296a9d6a761/Include/listobject.h#L22

```C
typedef struct {
    PyObject_HEAD
    Py_ssize_t ob_size;

    /* Vector of pointers to list elements.  list[0] is ob_item[0], etc. */
    PyObject **ob_item;

    /* ob_item contains space for 'allocated' elements.  The number
     * currently in use is ob_size.
     * Invariants:
     *     0 <= ob_size <= allocated
     *     len(list) == ob_size
     *     ob_item == NULL implies ob_size == allocated == 0
     */
    Py_ssize_t allocated;
} PyListObject;
```

A list is grown not by doubling, but by a pattern that looks like `0, 4, 8, 16, 25, 35, 46, 58, 72, 88, ...`:

https://github.com/python/cpython/blob/5c22476c01622f11b7745ee693f8b296a9d6a761/Objects/listobject.c#L22

```C
/* Ensure ob_item has room for at least newsize elements, and set
 * ob_size to newsize.  If newsize > ob_size on entry, the content
 * of the new slots at exit is undefined heap trash; it's the caller's
 * responsibility to overwrite them with sane values.
 * The number of allocated elements may grow, shrink, or stay the same.
 * Failure is impossible if newsize <= self.allocated on entry, although
 * that partly relies on an assumption that the system realloc() never
 * fails when passed a number of bytes <= the number of bytes last
 * allocated (the C standard doesn't guarantee this, but it's hard to
 * imagine a realloc implementation where it wouldn't be true).
 * Note that self->ob_item may change, and even if newsize is less
 * than ob_size on entry.
 */
static int
list_resize(PyListObject *self, Py_ssize_t newsize)
{
    PyObject **items;
    size_t new_allocated, num_allocated_bytes;
    Py_ssize_t allocated = self->allocated;

    /* Bypass realloc() when a previous overallocation is large enough
       to accommodate the newsize.  If the newsize falls lower than half
       the allocated size, then proceed with the realloc() to shrink the list.
    */
    if (allocated >= newsize && newsize >= (allocated >> 1)) {
        assert(self->ob_item != NULL || newsize == 0);
        Py_SIZE(self) = newsize;
        return 0;
    }

    /* This over-allocates proportional to the list size, making room
     * for additional growth.  The over-allocation is mild, but is
     * enough to give linear-time amortized behavior over a long
     * sequence of appends() in the presence of a poorly-performing
     * system realloc().
     * The growth pattern is:  0, 4, 8, 16, 25, 35, 46, 58, 72, 88, ...
     * Note: new_allocated won't overflow because the largest possible value
     *       is PY_SSIZE_T_MAX * (9 / 8) + 6 which always fits in a size_t.
     */
    new_allocated = (size_t)newsize + (newsize >> 3) + (newsize < 9 ? 3 : 6);
    if (new_allocated > (size_t)PY_SSIZE_T_MAX / sizeof(PyObject *)) {
        PyErr_NoMemory();
        return -1;
    }

    if (newsize == 0)
        new_allocated = 0;
    num_allocated_bytes = new_allocated * sizeof(PyObject *);
    items = (PyObject **)PyMem_Realloc(self->ob_item, num_allocated_bytes);
    if (items == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    self->ob_item = items;
    Py_SIZE(self) = newsize;
    self->allocated = new_allocated;
    return 0;
}
```

## Decorators

Decorators are a special syntax in python that allow you to wrap a function with another function. For example, consider if we have the function:

```python
def addFoo(string: str) -> str:
    return string + "foo"
```

We can wrap this function in another function that extends the behavior. 

```python
from typing import Callable

def addBarDecorator(func: Callable[[str], str]) -> Callable[[str], str]:
    def wrapper(string: str) -> str:
        string += "bar"
        return func(string)
    return wrapper

addFoo = addBarDecorator(addFoo)
print(addFoo("hello"))
```
<div class="result">
```title=""
hellobarfoo
```
</div>

You can see that our decorator returns a function `wrapper`, which contains the same signature of the wrapped function. `wrapper` performs some operation on the input before passing it along to the wrapped function, which is why the result becomes `hellobarfoo`: `bar` is added to the string first (in `wrapper`), then `foo` is added in `addFoo`.

There is syntatic sugar you can use to avoid having to specify `#!python addFoo = addBarDecorator(addFoo)`:

```python
from typing import Callable

def addBarDecorator(func: Callable[[str], str]) -> Callable[[str], str]:
    def wrapper(string: str) -> str:
        string += "bar"
        return func(string)
    return wrapper

@addBarDecorator
def addFoo(string: str) -> str:
    return string + "foo"

print(addFoo("hello"))
```
<div class="result">
```title=""
hellobarfoo
```
</div>

## Context Manager (`with` statement)

A context manager is a magic method you can add to a class that allows you to perform setup/teardown logic that is scoped within a particular lexical block. For example:

```python
from pathlib import Path

path = Path("/Users/landonclipp/test.txt")
with path.open("w") as f:
    f.write("foobar\n")
```

The `#!python with` operator first calls the magic method `__enter__` on the object returned by `path.open`, which is a [`TextIOWrapper`](https://docs.python.org/3/library/io.html#io.TextIOWrapper). When the enclosing block exits, the context manager calls `__exit__`, which in this case will flush any remaining bytes to the file and close it.

We can implement our own context manager. Take this example:

```python
class Foo:
    def __init__(self): pass

    def __enter__(self):
        print("enter")
    def __exit__(self, exc_type, exc_val, exc_tb):
        print("exit")

with Foo() as foo:
    print("enclosing block")
```
<div class="result">
```title=""
enter
enclosing block
exit
```
</div>

The `__exit__` function takes some additional parameters, as you've noticed. These are explained [here](https://book.pythontips.com/en/latest/context_managers.html#handling-exceptions):

1. `exc_type`: The type of exception raised
2. `exc_value`: The value of the exception.
3. `exc_tb`: The traceback of the exception.

Let's see what happens when we raise an exception in our `#!python with` block:

```python
class Foo:
    def __init__(self): pass

    def __enter__(self):
        print("enter")
    def __exit__(self, exc_type, exc_val, exc_tb):
        print(f"{exc_type = }")
        print(f"{type(exc_type) = }")
        print(f"{exc_val = }")
        print(f"{type(exc_val) = }")
        print(f"{exc_tb = }")
        print(f"{type(exc_tb) = }")
        print("exit")

with Foo() as foo:
    raise ValueError("uh-oh")
```
<div class="result">
```title=""
enter
exc_type = <class 'ValueError'>
type(exc_type) = <class 'type'>
exc_val = ValueError('uh-oh')
type(exc_val) = <class 'ValueError'>
exc_tb = <traceback object at 0x7f363b472f40>
type(exc_tb) = <class 'traceback'>
exit
Traceback (most recent call last):
  File "main.py", line 18, in <module>
    raise ValueError("uh-oh")
ValueError: uh-oh


** Process exited - Return Code: 1 **
Press Enter to exit terminal
```
</div>

You can see that the value is simply a `type` object (or just a class). The value of the exception in this case is an instance of the `ValueError` class, and the [traceback](https://docs.python.org/3/library/traceback.html) is something we can use to determine specifically where the exception happened.

## dictionary

### Literal declaration

```python
print({"foo": "bar"})
```

### Function declaration

```python
print(dict(foo="bar"))
```

### dict merging

#### Dict unpacking 

(this is my preferred way of doing it, for no good reason in particular):

```python
a = {0: "a", 1: "a", 2: "a"}
b = {2: "b", 3: "b"}
print({**a, **b})
```
<div class="result">
```title=""
{0: 'a', 1: 'a', 2: 'b', 3: 'b'}

```
</div>

#### Using `.update()`

```python
a = {0: "a", 1: "a", 2: "a"}
b = {2: "b", 3: "b"}
a.update(b)
print(a)
```
<div class="result">
```title=""
{0: 'a', 1: 'a', 2: 'b', 3: 'b'}

```
</div>

#### Using union

This requires Python >= 3.9

```python3
a = {0: "a", 1: "a", 2: "a"}
b = {2: "b", 3: "b"}
print(a | b)
```
<div class="result">
```title=""
{0: 'a', 1: 'a', 2: 'b', 3: 'b'}

```
</div>

## async

### async functions

Defined as

```python
async def foo():
    print("foo")
```

### `async with`

This context manager calls the `__aenter__` and `__aexit__` magic methods instead of the regular `__enter__` and `__exit__`. This is useful when your enter/exit methods rely on potentially expensive external calls.

```python
async with Connection() as conn:
    conn.get_item()
```

## Typing

### [`typing.Protocol`](https://docs.python.org/3.12/library/typing.html#typing.Protocol)

The `typing.Protocol` type allows you to define what is essentially a Go interface. Instead of a function taking a specific implementation of a class like this:

```python3
class RedisCounter:
    def __init__(self): ...

    def increment(self): ...

    def decrement(self): ...

def do_stuff(counter: RedisCounter): 
    counter.increment()
    counter.decrement()

do_stuff(counter=RedisCounter())
```

We can instead define a `typing.Protocol` type that will define the shape of the class that we accept:

```python3
import typing

class Counter(typing.Protocol):
    def increment(self): ...

    def decrement(self): ...

class RedisCounter:
    def __init__(self): ...

    def increment(self): ...

    def decrement(self): ...

def do_stuff(counter: Counter): 
    counter.increment()
    counter.decrement()

do_stuff(counter=RedisCounter())
```

`mypy` understands and accepts this even though `RedisCounter` doesn't inherit `Counter`. 
