---
date: 2025-05-08
authors:
  - LandonTClipp
categories:
  - Math
title: Introductions to Lambda Calculus
links:
  - brilliant.org: https://brilliant.org/wiki/lambda-calculus/
draft: true
---

Lambda calculus is a Turing-complete mathematical theory of computation. This post explores the basics of lambda calculus and how it relates to the ideas of functional programming. Most of the content herein is shamelessly copied from various educational sources on the internet, although some exercises at the end of this post will be my own work.(1)
{ .annotate }

1. You may be thinking, why do I just copy stuff? What value does that provide? Well, you have to remember that I write these blog posts for myself mainly, so this is just a learning exercise! I don't really care if no one else gets any value out of it :D

<!-- more -->

## Introduction

Lambda calculus, also represented as λ-calculus, forms the theoretical foundation of functional programming. Functional programming is a paradigm supported in many modern languages like Python, Go, Rust, PHP etc. There are two broad categories of Lambda calculus: typed and untyped. Most programming languages use typed lambda calculus with some notable exceptions being Python and Haskell. This blog will conflate the two systems a bit where we'll describe untyped lambda calculus against analogous typed Go code. 

Let's take for example a common math function:

$$
f(x)=x^2
$$

We can represent this in Go and pure lambda calculus:

=== "lambda"

    $$
    \lambda x.x^2
    $$

=== "go"

    ```go title=""
    func(x int) { return x * x }
    ```


If we wanted to apply this function to an expression (which can be a variable, a literal, or another function), we could do something like this:

=== "lambda"

    $$
    (\lambda x.x^2)7
    $$

=== "go"

    ```go title=""
    func(x int) { return x * x }(7)
    ```

Of course the answer being 49. The simplest form of a lambda calculus function is the identity function:

$$
\lambda x.x
$$

## Expressions

An expression can be thought of as programs in lambda calculus. Given a set of variables like $x,y,z,...,$, we can define expressions through a series of abstractions (anonymous functions) and applications as follows:

!!! info "Definition"

    Let $\Lambda$ be the set of expressions.

    1. **Variables**: If $x$ is a variable, then $x \in \Lambda$
    2. **Abstractions**: If $x$ is a variable and $\mathscr{M} \in \Lambda$, then $(\Lambda x.\mathscr{M}) \in \Lambda$.[^1] 
    3. **Applications**: If $\mathscr{M} \in \Lambda$ and $\mathscr{N} \in \Lambda$, then $(\mathscr{M}\mathscr{N}) \in \Lambda$.[^2]


[^1]: This is essentially saying that if you have another lambda expression $\mathscr{M}$, it's valid to apply that expression as the body of another lambda function.
[^2]: In English, this says that if you have two valid lambda expressions, you can apply those two expressions together as another valid lambda expression.

There are a few important conventions to note:

1. Function application is left-associative, unless otherwise stated by parenthesis:

    $$
    \mathscr{E}_1\mathscr{E}_2\mathscr{E}_3 \equiv ((\mathscr{E}_1\mathscr{E}_2)\mathscr{E}_3).
    $$

2. Consecutive abstractions can be uncurried:

    $$
    \lambda x y z . \mathscr{M} \equiv \lambda x . \lambda y . \lambda z . \mathscr{M}.
    $$

3. The body of the abstraction extends to the right as far as possible:

    $$
    \lambda x . \mathscr{M} \mathscr{N} \equiv \lambda x . (\mathscr{M} \mathscr{N}).
    $$

## Free and Bound Variables

In an abstraction like $\lambda x . x$, the variable $x$ is something that has no original meaning but is a placeholder. We say that $x$ is a variable _bound_ to the $\lambda$. On the other hand, in $\lambda x . y$, i.e. a function which always returns $y$ whatever it takes, $y$ is a free variable since it has an independent meaning by itself. Because a variable is bound in some sub-expression (the scope) does not mean it is bound everywhere. For example, this is a perfectly valid expression (an example of an application, by the way):

$$
(\lambda x . x)(\lambda y . yx)
$$

Here the $x$ in the second parentheses has nothing to do with the one in the first.

Let us define these concepts conceptually:

!!! info "Free"

    1. $x$ is free in the expression $x$.
    2. $x$ is free in the expression $\lambda y . \mathscr{M}$ if $x \ne y$ and $x$ is free in $\mathscr{M}$.
    3. $x$ is free in $\mathscr{M} \mathscr{N}$ if $x$ is free in $\mathscr{M}$ or if it is free in $\mathscr{n}$.

!!! info "Bound"

    1. $x$ is bound in the expression $\lambda x . \mathscr{M}$.
    2. $x$ is bound in $\mathscr{M} \mathscr{n}$ if $x$ is bound in $\mathscr{M}$ or if it is bound in $\mathscr{N}$.

Notice that a variable can be both bound and free but they represent differentthings, as we discussed in the example above.(1)
{ .annotate }

1. This is indeed something I immediately wondered about because point 3 in the `Free` definition directly contradicts the `Bound` definition point 2. How can both be true? Well, it seems in such a case, $x$ simply refers to two different things.

An expression with no free variables is called a **closed** expression.

## Reductions

TODO