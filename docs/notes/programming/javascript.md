---
title: Javascript
icon: simple/javascript
---

Javascript
==========

Note: Javascript and Typescript will be used somewhat interchangeably in this page. Although, preference will be given to Typescript because why tf would you not type your languages?

Keyword Arguments
-------------------------------

Typescript does not have keyword arguments like Python, but you can emulate it with Object Destructuring:

```typescript
function greet({ firstName, lastName }) {
    console.log(`Hello, ${firstName} ${lastName}!`);
}

// Calling the function with an object
greet({ firstName: "John", lastName: "Doe" }); // Output: Hello, John Doe!
```

You can provide default values:

```typescript
function greet({ firstName = "John", lastName = "Doe" } = {}) {
    console.log(`Hello, ${firstName} ${lastName}!`);
}

// Calling the function with missing properties
greet({ firstName: "Jane" }); // Output: Hello, Jane Doe!
greet({}); // Output: Hello, John Doe!
greet(); // Output: Hello, John Doe!
```