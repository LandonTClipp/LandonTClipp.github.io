---
date: 2023-12-01
categories:
  - Programming
title: Worse is Better
---

Worse is Better
===============

When I design software systems, my _modus operandi_ is generally to design the minimally acceptable set of features required by the business needs. This methodology allows me to be efficient with my time as I am focusing my energy on the known business problems. It also reduces the expectations your users have which is strangely a huge benfit: lower expectations usually translates to lower maintenance burdens. It allows you to focus your effort on the biggest problems your users face.

<!-- more -->

I discovered that this methodology has a name: [Worse is Better](https://en.wikipedia.org/wiki/Worse_is_better).[^1] Coined by Richard P. Gabriel in a 1989 essay, it makes the assertion that at a certain scale, having less functionality in your software is preferrable to having more. In my own words, the reason for this is as follows:

1. Simple software is easier to write. When it's easier to write, that means you are faster to market. It's a fact of life that the _first_ implementation is in many situations the one that grabs the largest market share.
2. Simple software is easier to test, which by extension makes your functionality more reliable.
3. Simple software is easier to maintain. Less features means less work!

The core tenants of Worse is Better emphasize the simplicity of the implementation:

!!! quote "Core Tenants"
    ## Core Tenants
    ### Simplicity

    The design must be simple, both in implementation and interface. It is more important for the implementation to be simpler than the interface. Simplicity is the most important consideration in a design.

    ### Correctness

    The design should be correct in all observable aspects. It is slightly better to be simple than correct.

    ### Consistency

    The design must not be overly inconsistent. Consistency can be sacrificed for simplicity in some cases, but it is better to drop those parts of the design that deal with less common circumstances than to introduce either complexity or inconsistency in the implementation.

    ### Completeness

    The design must cover as many important situations as is practical. All reasonably expected cases should be covered. Completeness can be sacrificed in favor of any other quality. In fact, completeness must be sacrificed whenever implementation simplicity is jeopardized. Consistency can be sacrificed to achieve completeness if simplicity is retained; especially worthless is consistency of interface.

## mockery
I relate this philosphy to my stewardship of the [mockery](https://vektra.github.io/mockery/latest/) project. For those unfamiliar with mockery, this project is a code generation tool that generates mock implementations of Go interfaces. The mocks can be configured in your test code to return certain values when certain arguments are passed to it, as well as asserting various things about how the mock was called (which obviously is a controversial thing to assert, but I digress :cowboy:).

All of this sounds well and dandy until you start getting into the edge cases. For example, how do you handle argument types? If you decide to make expectations use the same type as the interface method, then how do you handle cases where you want to specify `mock.Anything` for one of the arguments (hint: you can't). Instead what mockery [makes the `.EXPECT()` method arguments use `interface{}`](https://github.com/vektra/mockery/blob/v2.38.0/mocks/github.com/vektra/mockery/v2/pkg/fixtures/Requester.go#L55) so that you can specify [`mock.Anything`](https://pkg.go.dev/github.com/stretchr/testify@v1.8.4/mock) (which is of type `string`), _or_ a value of the type in your interface definition. These two cases have to be differentiated during runtime, which means your compiler can't help you if you made an uh-oh and provided a totally erroneous type. Yuck! 

```go title="mockery mocks"
// An example you might run across in mockery.
type Getter interface{
    Get(id int) string
}

// The function below is what you'd use to define expectations. Note `id` is `interface{}`, not `int`!
func (_e *Requester_Expecter) Get(id interface{}) *Requester_Get_Call {
	return &Requester_Get_Call{Call: _e.mock.On("Get", path)}
}

// The test might look like:
func TestGetter(t *testing.T) {
    mockGetter := NewMockGetter(t)
    mockGetter.EXPECT(0).Return("foo")
    mockGetter.EXPECT(1).Return("bar")
    if getFromDB(mockGetter) != 1 {
        t.Errorf("you twat")
    }
}

func getFromDB(getter Getter) string {
    return getter.Get(1)
}
```

Armed with hindsight, I realize that mockery is not _simple_ in its implementation. Its interface is indeed quite simple and intuitive, but the fact that its implementation is so complex means that we are sometimes plagued by edge cases and bugs (which I've diligently worked to address, and successfully might I add). A more simple code-generation implementation I've found is Mat Ryer's own [moq](https://github.com/matryer/moq) repo. Instead of matching arguments to return values, users of Mat's moq only have to assign an anonymous function to the appropriate attribute of the mock struct.

## moq

```go title="moq mocks"
type Getter inferface {
    Get(id int) string
}

// We assume the mock object has already been made
func TestGet(t *testing.T) {
    mockGetter := &GetterMock{
        GetFunc: func(id int) string {
            if id == 0 {
                return "foo"
            }
            return "bar"
        }
    }
    if getFromDB(mockGetter) != "bar" {
        t.Errorf("you muppet")
    }
}
```

With `moq`, there is no fancy argument matching, no crazy semantics, no blackbox magic happening. By all accounts it's "worse" than mockery in the sense that it doesn't have as many features, but the benefit is that its maintainers don't have to do as much work as I do (I'm hoping) to keep the project running.

I didn't intend for this to turn into a diatribe on the subtlties of Go mocks, but it's a real world application I've found where there is concrete evidence that Worse _is indeed_ (sometimes... many times) Better.

## postscriptum

"Worse is Better" is obviously a click-baity name for the idea, which is a nice reminder that clickbait long predates the cesspool of modern engagement-based monitziation schemes. On that note, Gabriel wrote a rebuttal to his own essay, which he called ["Worse is Better is Worse"](https://dreamsongs.com/Files/worse-is-worse.pdf).[^2] No joke!

[^1]: https://en.wikipedia.org/wiki/Worse_is_better
[^2]: https://dreamsongs.com/WorseIsBetter.html

