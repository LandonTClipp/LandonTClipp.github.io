---
date: 2023-06-21
categories:
  - Golang
  - Programming
title: Introducing Mockery's New packages Feature
description: What the packages feature is, how it works, and why you should use it.
---

Introducing Mockery's New `packages` Feature
============================================

[vektra/mockery](https://github.com/vektra/mockery) is a Go package that auto-generates mock implementations of interfaces in your project. For those not familiar with what mockery is, please take a look at its [documentation page](https://vektra.github.io/mockery/). I took over the project in 2020 and have overseen a number of significant feature updates to the code. The most recent of these is a feature I call `packages`, and it's one I'm most proud of due to the enormous benefits it grants you.

<!-- more -->

Previous State of the Art
----------------

Previously, the way one might tell mockery to generate mock implementations would be to use `go:generate` directives in your code. Example:

```go title="string.go"
package pkg

//go:generate mockery --name Stringer
type Stringer interface {
	String() string
}
```

You would then call `go generate` at the top of your project. Go would recursively iterate through your `.go` files to find all instances of `go:generate` and execute the command listed. This would induce mockery to generate the mock specified in the `--name` parameter.

The problem with this approach is multi-faceted:

1. It's [_incredibly_ slow](https://github.com/vektra/mockery/discussions/520) for large projects. Mockery would call [`packages.Load`](https://pkg.go.dev/golang.org/x/tools/go/packages#Load) once per file, which means it's parsing the entire syntax tree for a package multiple times with little to no caching.
2. It's inflexible. Many Github issues were created throughout the years requesting new features for esoteric and specific use-cases that existed purely because mockery didn't give people the tools they need to generate the mocks how they wanted. The config model itself was wrong.
3. It's verbose. You have to do quite a lot of writing if your configuration is complex or varies based on each config.
4. It's incohesive. You have many go generate statements spread across your repo, which makes it hard to quickly determine the extent of your mock generation. There is also no strict requirement that `go:generate` statements are next to your interface, they can be placed literally anywhere in your project.


My experience as maintainer of this project over the years gave me the insight I needed to fix the problems people were having. The most painful issue was almost always the first (performance), followed closely by the second (inflexibility).

What is `packages`?
-------------------

`packages` is simply a feature that changes the configuration model to be more cohesive with how abstract syntax tree parsing works in Go, and provides more flexibility and control over how mocks are generated. As mentioned before, the primary concern was with calling `packages.Load` multiple times per project, when it really only should be called once for the entire project (irrespective of how many packages you're actually parsing).

Let's go back to our example of `Stringer`:

```go title="string.go"
package pkg

type Stringer interface {
	String() string
}
```

Using `packages`, you could tell mockery to generate a mock for this in config:

```yaml
packages:
  myproject/pkg:
    interfaces:
      Stringer:
```

![Initial project layout](/images/mockery_packages_feature/0000_directory_layout_initial.png)

We can then call mockery with no arguments[^1]:

![mockery terminal output](/images/mockery_packages_feature/0001_mockery_call.png)

You will see that mockery has now generated a separate `mocks` folder with the `mock_Stringer.go` file:

![project layout after mockery run](/images/mockery_packages_feature/0002_directory_layout_post.png)

The end result is largely the same, however the configuration file gives you an incredibly powerful way to specify the configs.

Why do you want `packages`?
---------------------------

1. It's blazing fast. The mockery project [dogfoods](https://en.wikipedia.org/wiki/Eating_your_own_dog_food) itself and has found that usage of `packages` results in a 5x increase in speed. Your performance increase could be even higher. You can see first-hand testimonials of people trying out the alpha and beta releases of `packages`: https://github.com/vektra/mockery/discussions/549

2. Its configuration options allow huge control over what, where, and how your mocks are generated.
3. The config model is hierarchical! The body of config options can be specified at the top of the yaml to specify default values, at the package level, and at the interface level. These config options are merged hierarchically, which means it's more DRY-compliant.
4. Many of the config options use the Go templating engine, which allows you to reference dynamic runtime parameters.

The performance benefit comes from the fact that mockery now gathers all of the packages to load (that are either statically defined or dynamically discovered through `recursive`) and passes that list to a single `packages.Load` invocation. Most other mock generator packages, even the officially supported `gomock` project, do not do this, which means mockery should now be orders of magnitude faster than any other solution.

Useful links:

- [feature documentation](https://vektra.github.io/mockery/features/#packages-configuration)
- [configuration options available](https://vektra.github.io/mockery/configuration/#packages-config) for more details.
- [migrating to `packages`](https://vektra.github.io/mockery/migrating_to_packages/)

Advanced Examples
------------------

### Using `#!yaml all: true`

Take for example the [mockery project itself](https://github.com/vektra/mockery/tree/master/pkg/fixtures), which contains a huge number of interfaces in `github.com/vektra/mockery/v2/fixtures` that are used for integration testing. Instead of manually specifying each individual interface we want to generate mocks for, we can simply do something like:

```yaml
packages:
  github.com/vektra/mockery/v2/pkg/fixtures:
    config:
      all: True
```

This tells mockery we want all the interfaces to be included in mock generation. You can see the output of this run [here](https://github.com/vektra/mockery/tree/v2.30.1/mocks/github.com/vektra/mockery/v2/pkg/fixtures).

### Using `#!yaml recursive: true`

There may also be cases where you have a large number of packages, and you don't want to specify each individual package in config. If you use `#!yaml recursive: true`, you can specify the top-level package and let mockery auto-discover all of the subpackages beneath. Again, the mockery project has an example of this, using [these interfaces](https://github.com/vektra/mockery/tree/v2.30.1/pkg/fixtures/recursive_generation).

```yaml
packages:
  github.com/vektra/mockery/v2/pkg/fixtures/recursive_generation:
    config:
      recursive: True
      all: True
```

### Usage of templated variables to specify alternate locations

By default, mockery places all mocks into a separate `mocks/` folder. However, you can use the template variables to specify alternate locations for your mocks, for example adjacjent to where the interface is defined. This can be useful to resolve cyclic import issues. The docs show us the config you need to use to achieve this:

```yaml
dir: "{{.InterfaceDir}}" # (1)!
outpkg: "{{.PackageName}}" # (2)!
inpackage: True # (3)!
```

1. The `InterfaceDir` template variable specifies that the directory of the mock file should be the same as the original interface.
2. The `PackageName` template variable is, unsurprisingly, the package name of the original interface. Because we are generating the mock in the same directory as the interface, it has to also have the same package name.
3. We have to explicitly tell mockery that the mock is generated "in-package" because the types and variables it references don't need to be imported; they're already in the same package as the mock itself.

Going back to our toy example, here is the config we'd want:

```yaml
dir: "{{.InterfaceDir}}"
outpkg: "{{.PackageName}}"
inpackage: true
packages:
  myproject/pkg:
    config:
      all: true
```

After running mockery, you see that the mock is generated adjacent to the `string.go` file:

![in-package mock directory structure](/images/mockery_packages_feature/0003_directory_layout_in_package.png)

### Multiple mocks generated from the same interface

There may also be cases where you want multiple mocks with differing config from the same interface. This is also supported. The mockery project [provides an example of this](https://github.com/vektra/mockery/blob/v2.30.1/.mockery.yaml#L17-L24):

```yaml
packages:
  github.com/vektra/mockery/v2/pkg/fixtures:
    config:
      all: True
    interfaces:
      RequesterVariadic: # (1)!
        config:
          with-expecter: False
        configs:
          - mockname: RequesterVariadicOneArgument # (2)!
            unroll-variadic: False
          - mockname: RequesterVariadic
            unroll-variadic: True
```

1. Even though we're specifying `#!yaml all: true`, we can still override specific interfaces.
2. Each list element here must contain a `mockname` to differentiate it from the parent interface. We also specify a different `unroll-variadic` argument to each one.

External Packages
-----------------

Of course, you can still specify external interfaces to be mocked. If you can `go get` it, you can mock it!

```yaml
packages:
  io:
    interfaces:
      Reader: 
```

Feedback
---------

Please try this out and let me know your thoughts in the comments below. I worked hard on this feature and it turned out really well, so I hope you enjoy it.

I also want to thank everyone who has contributed to this project over the years. Most of the features in mockery have been implemented by the open source community, and I've only served as an advisor and button pusher for most of it. So thanks for making this project what it is, open source works well when done correctly!

[^1]: You may note the mockery logs are saying the packages feature in a beta state. At the time of this writing, this is true, however it will be fully released a short time after this is published.
