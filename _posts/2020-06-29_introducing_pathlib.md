[`chigopher/pathlib`](https://github.com/chigopher/pathlib/)
===============================================================

Most programing languages include some kind of "OS" package that allows you to manipulate files and filesystems. The issue with a lot of these packages, including Golang's own `os` and `io/ioutil`, is that they don't provide an easy way to represent objects within your filesystem in an object-oriented way. For instance, let's say that you want to read the root directory of your filesystem. Using vanilla Golang packages, you would do something like this:

[Go Playground](https://play.golang.org/p/xQuu60BntrM)
```go
package main

import (
	"fmt"
	"io/ioutil"
)

func main() {
	files, _ := ioutil.ReadDir("/")

	for _, f := range files {
		fmt.Println(f.Name())
	}
}
```


Simple enough! Now let's say you wanted to read the root dir and any directory below that. Here's what you might do:

[Go Playground](https://play.golang.org/p/ztwDITye8AE)
```go
package main

import (
	"fmt"
	"io/ioutil"
	"path/filepath"
)

func listDir(path string, indentLevel int) {
 	subPathFiles, _ := ioutil.ReadDir(path)
			
	indent := ""
	for i := 0; i < indentLevel; i++ {
		indent += "	"
	}
	
	for _, subPathFile := range subPathFiles {
		fmt.Println(indent, subPathFile.Name())
	}				
}

func main() {
	files, _ := ioutil.ReadDir("/")

	for _, f := range files {
		if f.IsDir() {
			subPath := filepath.Join("/", f.Name())
			listDir(subPath, 1)
		}
		fmt.Println(f.Name())
	}
}

```

Whoa! That's really ugly! Let's break down what's going on here:

1. For every directory inside `/`, we construct the absolute path by using `filepath.Join()`.
2. We then list all of the contents of that directory by passing the path to `listDir()`
3. `listDir()` calls `ioutil.ReadDir()` with the given path and iterates over the retuend list.

Why is this really ugly? Well for one, any time we iterate over a directory, we don't have an object-oriented way to relate one path to its parent, or to its children. We have to construct these subpaths by continuously joining paths together using `filepath`, and then iterating over the children with `ioutil`. This method is very disjointed and incoherent.

Let me show you a better way. Using `pathlib`, this turns into:

```go

```
