---
date: 2024-12-03
categories:
  - Career
title: 2024 Career Reflections
description: A year in review.
---

<!-- more -->

## Highlights

For me, 2024 has been nothing short of extraordinary for a number of reasons.
Just _some_ of the main highlights include:

1. I got engaged to the love of my life.
2. Me and my fiancee couldn't wait to get married so we went to the courthouse a couple months later and sealed the deal!
3. I changed jobs.
4. I went to Colorado 2 times. I don't know if you can tell, but I REALLY love Colorado.
5. I saw a total solar eclipse in Olney, IL with my friends.
6. I drove by RAGBRAI to support one of my friends. If you're unfamiliar with it, [here's a great blog post](https://xorvoid.com/ragbrai_l.html) from one of my Jump ex-coworkers.

## Career Change

My prior employer, Jump Trading, had graciously offered me an engineering role straight
out of college back in 2018. Throughout my 5 years at the firm, I was exposed to
a broad range of cutting edge technology and environments that propelled me to
a competency that I feel college could never give. Moreso, I benefited from
a number of close mentors who coached me out of bad habits and showed me grace
when I needed it most. It also can't be denied that I regularly felt imposter
syndrome because the sheer density of brainpower I was surrounded with was humbling.

Sadly, all things end, and in late 2023 I decided my time at Jump was one of them. There was a broad (and noisy) kerfuffle that the crypto community experienced in 2022 that started with FTX and seemed to wind its way to a number of other coins and exchanges. The stock market also experienced headwinds with the post-COVID interest rate hikes and general market fear over the direction of the economy. These factors led to what I felt was a changing working environment that made it difficult for me to be successful in my role not just in terms of happiness, but also for my general career trajectory.

Those two sentiments I just described, both respect and disaffection, might sound contradictory. I credit Jump for catapulting my career to where it is today and to the relative ease with which I'm able to navigate the job market. However, it's true that business realities can change on a dime, often through no fault of our own. The HFT community can be characterized as being _really_ good when times are good, but really _really_ bad when times are tough. To be clear, I have nothing but fondness for those I had the pleasure to work with, and there are many invaluable lessons I learned through the grace of my mentors and the compassion they showed me.

Further, it's hard for me to not feel attached to my work. I think about my time at Jump a lot and of the many wonderful memories I have. I admit that I was dejected when I came to the realization that it was time for me to move on, but I'm glad I did because my new position at Lambda Labs is challenging me in the ways that I needed to be challenged. I feel a deep sense of mutual respect and trust between myself and everyone I work with, and that's a kind of environment that retains talent in the long run.

## [Lambda Labs](https://lambdalabs.com/){ .external }

![](/images/lambda/1cc-grid-purple.png)

Lambda Labs, if you are unfamiliar, is a startup cloud company that sells infrastructure tailored towards AI research. Practically speaking, this means we're building out large, GPU-dense datacenters with large, expensive Infiniband and ethernet fabrics. It's not entirely unlike what trading firms do, but the HPC environments add another significant layer of complexity that comes with multi-tenancy requirements.

Being a public cloud, we have to host customers in a virtualized environment. This means dealing with technologies like:

1. QEMU
2. SR-IOV
3. NDFC (as an SDN)
4. OVN+OVS

And many other related services and technologies that go into securely hosting a public cloud with private customer data. It's been an exciting space to live and breathe because I'm in many cases one or two degrees of separation away from rubbing shoulders with the current AI titans of the industry. That's a bit of a vain observation to make, but I bring it up to highlight the excitement in which I find myself in.

Another observation I've made is that AI has a real, tangible benefit to society. Many of our customers are generating models that can predict protein folds, examine CAT scans, generate videos and images, provide possible diagnoses to health issues, and lots of other incredible uses. In fact I have been increasingly using chatbots like ChatGPT to distill complex technical topics and ask it for inspiration on troubleshooting esoteric problems.(1)
{ .annotate }

1. This deserves a whole blog post on its own, but I made the journey from extreme skepticism to whole-hearted believer when it comes to chatbots in the workplace. I don't view things like ChatGPT as a replacement for the human brain, but rather a more powerful alternative to Google and Stack Overflow. It still comes with the usual caveat of "don't believe everything you see on the internet" because it can be wrong in big ways!

<div class="annotate" markdown>
The cool thing about startups, especially ones with such meteoric growth, is that it's relatively easy to make a big impact. This is contrasted to larger companies (like Jump) that have a somewhat entrenched technical culture and already have "the way" of doing things.(1) In just a few short months, I was able to identify a huge business need that Lambda had around customer VM observability. Specifically, the need for us to ship customers metrics about their own VMs. Every public cloud deals with the same question and the solution usually looks something like:

1. Install a metrics collection service on the VM.
2. Ship the collected metrics to a hosted data store.
3. Expose the data store through some kind of API gateway so customers can access it directly (either through a UI or API).
4. Sell the metrics for profit!
</div>

1. An entrenched technology culture by itself is not a bad thing because it often means that an organization has found a solution that works well enough. However it does mean finding ways to make company-wide impacts is sometimes a fruitless effort. In the worst cases, it means that the company has become so ossified that making dramatic business pivots is exceedingly difficult

![lambda-guest-agent](/images/lambda/lambda-guest-agent-1.jpeg){ align=right width="35%"}

I was able to identify this need both from comments that executive leadership would make, but also from customers lamenting the lack of this fairly basic product. This led to me leading a project that we call the [lambda-guest-agent](https://docs.lambdalabs.com/public-cloud/guest-agent/). It's simple in theory but in practice it's a quagmire that deals with topics like data privacy laws, security, SOC compliance, cross-team collaboration, priority management, and of course the fun technical aspects like metrics collection, Prometheus, public APIs, frontend graphing technologies, API gateways... you get the picture. Building a public cloud is HARD and even conceptually simple things tend to take enormous effort. This is very much contrasted to HFTs where concepts can be turned into production with relatively minimal fuss.

## Building a Cloud From Scratch

Lambda's cloud is new. Originally, Lambda focused on building and selling desktops tailored to AI research. Within the last few years, they decided to pivot to creating a fully-fledged public cloud as the demand for AI hardware only increased. Researchers needed enterprise-grade hardware that could not fit inside a typical desktop chassis, and the leadership rightfully noticed the dearth of performant, affordable HPC solutions.

What's been striking to me is two things:

<div class="annotate" markdown>
1. There aren't many opportunities to work at a company that is building a big-boy cloud from scratch
2. Investors LOVE the idea of being able to get in on the ground floor of what appears to be the largest technological revolution in the last 20 years.(1)
</div>

1. This point is something I don't say lightly. Crypto was supposed to be the Next Big Thing^TM^ but it turned out to be one big massive fraud. My time at Jump only solidified my view that crypto does provide much benefit to society beyond finding novel ways of scamming people. I felt the impact of it by what it did to the people I worked with, the jobs it ended, and the turmoil it caused in my life. But that's enough fretting about, let's not dwell on the negative!

The confluence of those two points lends to again finding myself in a situation where I am surrounded by people way smarter than me. I am in an environment that appreciates engineers who take the initiative in leading massive projects. I feel that everyone I work with _believes_ in the promise of AI and nearly everyone has an intrinsic motivation to push this company forward.

## Thoughts on Competition

People often ask me what the competitive landscape is for the AI public cloud space, especially when considering the cloud juggernauts like AWS, Azure, GCP etc. The points can be distilled down a few ways:

1. The infrastructure demands of AI workloads are totally different from traditional web services. This means that the requisite high-performance IB networks are a fairly different skillset to operate efficiently in a multi-tenant cloud environment.
2. As it stands today, the AI cloud market is still significantly smaller than the traditional web services market, so the juggernauts don't appear to be super interested in this space _yet_.
3. The juggernauts demand high price points, because they can. Lambda Labs is currently amongst the cheapest compared to other cloud vendors. Obviously there is still a risk that the juggernauts could play anti-competitive pricing practices and take intentional losses to drive away our market share, but this has yet to materialize in any meaningful way.
4. AI-specific HPC systems are heavy, expensive, complex beasts by themselves. They are orders of magnitude more complex when you ask those systems to become multi-tenant. It takes a lot of support and a lot of time to get these systems operating efficiently, which means it's very human-capital intensive. The marginal costs of standing up more compute is higher than traditional web-focused environments. While the HPC community continues to iron out how to run AI-tailored, multi-tenant HPC systems, the ongoing costs will remain elevated as well. Lambda is time-advantaged in this case because we are learning these lessons now, while the big cloud appear to be sleeping at the wheel.

Currently, Lambda's focus is on tailoring our experience for the small AI developers. Our strategy is to court small AI startups through solid support experiences, reliable infrastructure, reasonable price points, and good experiences so that if/when the startup becomes larger, they continue to do business with us.

Our main competitor in this space, Coreweave, only focuses on landing a small number of mega-sized contracts. This presents a large business risk with the possibility that some of their customers decide not to renew. Lambda's approach is to spread our customer base amongst a large number of smaller customers and to wage an effective marketing campaign to prove to AI researchers that our product is better than the competition. It also incentivizes us to solve the multi-tenant AI-HPC infrastructure problem better than anyone else so that we are better positioned than anyone else to support AI research into the future.

## Looking Forward

Where do I go from here? Well, the idea is just to keep doing more of the same. I hope that in 2025, lambda-guest-agent will be a fully fledged, monetized product that will make billions and billions of dollars :money_mouth:. In all seriousness, I'll continue to search for high-impact projects that drive further revenue growth and make Lambda's development experience even smoother than it already is. I'm grateful to be in an environment that encourages this ambition and to be in the company of many wonderfully talented engineers.

On a personal level, I'm going to be spending as much time as I reasonably can in Colorado. If you haven't seen my other blog posts [here](2024-10-07-starlink-mobile.md) and [here](2023-06-21-intech-sol-horizon-cellular.md), I've been building out the ultimate remote work RV over the last couple of years, and it's finally done! My [last trip](2024-10-14-colorado-camping.md) was somewhat of a mixed bag in terms of what I set out to accomplish, but I've worked through those bugs and hope to have a less eventful trip next year.
