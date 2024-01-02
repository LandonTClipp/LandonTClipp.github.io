---
title: System Design Interview – An insider's guide
---

https://books.google.com/books/about/System_Design_Interview_An_Insider_s_Gui.html?id=b_mUzQEACAAJ&source=kp_book_description

## Chapter 1: Scale from Zero To Millions of Users

**Database Sharding**

Sharing is a horizontal scaling technique used in databases whereby multiple databases share the same schema, but will store different sets of data. Where a specific piece of data goes depends on the sharding key.

Sharding has many problems:

1. Resharding data: one shard might become too saturated due to uneven data distribution. This requires changing the sharding function and moving data around.
2. Celebrity problem: what if Lady Gaga gets put on a single shard? Now that shard will be overloaded with reads.
3. Joins: corss-shard joins become difficult (you need to think about how to optimize your query by considering where the data lives).

## Chapter 2: Back-of-the-Envelope Estimation

Back-of-the-envelope estimations are typically asked for in system design interviews.

Tips:

- Write down your assumptions
- Label your units
- Write down QPS (queries per second), peak QPS, storage, cache (if applicable), number of servers etc.

### Power of Two

| power | approximate value | full name | short name |
|-------|-------------------|-----------|------------|
| 10 | 1 Thousand | 1 Kilobyte | 1 KB |
| 20 | 1 Million | 1 Megabyte | 1 MB |
| 30 | 1 Billion | 1 Gigabyte | 1 GB |
| 40 | 1 Trillion | 1 Terabyte | 1 TB |
| 50 | 1 Quadrillion | 1 Petabyte | 1 PB |

### Latency numbers

Here are some typical latency numbers every programmer should know:

| Operation Name | Time |
|----------------|------|
| L1 cache reference | 0.5 ns |
| Branch mispredict | 5 ns |
| L2 cache reference | 7 ns |
| Mutex lock/unlock | 100 ns |
| Main memory reference | 100ns |
| Compress 1K bytes with Zippy | 10,000 ns = 10 μs |
| Send 2K bytes over 1Gbps network | 20,000 ns = 20 μs |
| Read 1 MB sequentially from memory | 250,000 ns = 250 μs |
| Round trip within the same datacenter | 500,000 ns = 500 μs |
| Disk seek | 10,000,000 ns = 10 ms | 
| Read 1 MB sequentially from network | 10,000,000 ns = 10 ms |
| Read 1 MB sequentially from disk | 30,000,000 ns = 30ms |
| Send packet (California -> Netherlands -> California) | 150,000,000 ns = 150ms |

### Availability numbers

Usually measured in "nines", or number of 9's digits. Example: 3 nines is 99.9% availability.

## Chapter 3: A Framework for System Design

#### 4-step process for interview

1. Understand the problem and establish design scope
    - Do not rush into starting a solution. Slow down, ask questions, and think deeply about the requirements and assumptions. This is extremely important.
    - When you ask a question, the interviewer will either answer or tell you to make an assumption. In either case, write down the answer, or the new assumption you have to make.
    - Ask questions to understand the requirements. Examples:
        - What specific features are we going to build?
        - How many users does the product have?
        - How fast does the company anticipate to scale up?
        - What is the company's technology stack? What existing services you might leverage to simplify the design?
2. Propose high-level design and get buy-in
    - Come up with an initial blueprint. Ask for feedback. Treat your interviewer as a teammate and work together.
    - Draw box diagrams with key components on a whiteboard or paper.
    - Do back-of-the-envelope calculations to evaluate if your blueprint fits the scale constraints. Think out loud. Communicate with interviewer if back-of-the-envelope is necessary before diving into it.
    - Go through a few concrete use-cases
    - Should we include API endpoints and database schema? What level of detail are we looking for?
3. Design deep dive
    - You and the interviewer should have already agreed on the following objectives:
        1. Agreed on overall goals
        2. Sketched out high-level blueprint for overall design
        3. Obtained feedback from interviewer on high-level design
        4. Had some intial ideas about areas to focus on based on interviewer's feedback
4. Wrap up
    - Interviewer might ask a few follow up questions:
        1. Identify system bottlenecks and potential improvements
        2. Might be useful to give the interviewer a recap of the design. Refreshing the interviewer's memory is helpful.
        3. Error cases?
        4. Operation issues. How do support this system in on-call? Observability/monitoring/logging?
        5. How to scale this up? If your design can handle 1 million users, what changes have to be made to scale to 10 million?
        6. Propose other refinements

##### Do's and Don'ts

**Dos**:

1. Ask for clarification. Do not assume assumption is correct.
2. Understand the requirements
3. There is no right answer nor the best answer. 
4. Let the interviewer know what you are thinking.
5. Suggest multiple approaches if possible.
6. Once you agree on blueprint, go into detail on each component. Design most critical components first.
7. Bounce ideas off interviewer
8. Never give up

**Don'ts**:

1. Don't be unprepared for typical interview questions
2. Don't jump into a solution without clarifying requirements
3. Don't go into too much detail on a single component. Start at high-level, then drill down where appropriate.
4. If you get stuck, don't hesitate to ask for hints.
5. Don't think in silence.
6. Don't think the interviewer is done when you give the design. Ask for feedback early and often.

#### Time allocation

Allocated 45 minutes or an hour is typical, but not enough to entirely flesh out a full system.

Step 1: understanding problem and design scope (3-10 minutes)

Step 2: Propose high-level design (10-15 minutes)

Step 3: Design deep dive (10-25 minutes)

Step 4: Wrap up (3-5 minutes)

## Chapter 4: Design a Rate Limiter

Foo