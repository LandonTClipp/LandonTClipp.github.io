---
date: 2024-12-03
categories:
  - Career
title: 2024 Career Reflections
draft: true
---

The year is coming to a close and I felt it was time for me to reflect on the changes
that I went through both personally and professionally.

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
a competency that I feel college could never give. But moreso, I benefited from
a number of close mentors who coached me out of bad habits and showed me grace
when I needed it most. It also can't be denied that I regularly felt imposter
syndrome because the sheer density of brainpower I was surrounded with was humbling.

Sadly, all things end, and in late 2023 I decided my time at Jump was one of them. There was a broad (and noisy) kerfuffle that the crypto community experienced in 2022 that started with FTX and seemed to wind its way to a number of other coins and exchanges. The stock market also experienced headwinds with the post-COVID interest rate hikes and general market fear over the direction of the economy. These factors led to what I felt was a changing working environment that made it difficult for me to be successful in my role not just in terms of happiness, but also for my general career trajectory. 

Those two sentiments I just described, both respect and disaffection, might sound contradictory. I credit Jump for catapulting my career to where it is today and to the relative ease with which I'm able to navigate the job market. However, it's true that business realities can change on a dime, often through no fault of our own. The HFT community can be characterized as being _really_ good when times are good, but really _really_ bad when times are tough. To be clear, I have nothing but fondness for those I had the pleasure to work with, and there are many invaluable lessons I learned through the grace of my mentors and the compassion they showed me, even when I messed up badly.

## [Lambda Labs](https://lambdalabs.com/){ .external }

Lambda Labs, if you are unfamiliar, is a startup cloud company that sells infrastructure tailored towards AI research. Practically speaking, this means we're building out large, GPU-dense datacenters with large, expensive Infiniband and ethernet fabrics. It's not entirely unlike what trading firms do, but the HPC environments add another significant layer of complexity that comes with multi-tenancy requirements. Being a public cloud, we have to host customers in a virtualized environment. This means dealing with technologies like:

1. QEMU
2. SR-IOV
3. NDFC (as an SDN)
4. OVN+OVS

And many other related services and technologies that go into securely hosting a public cloud with private customer data. It's been an exciting space to live and breathe because I'm in many cases one or two degrees of separation away from rubbing shoulders with the current AI titans of the industry. That's a bit of a vain observation to make, but I bring it up to highlight the excitement in which I find myself in.

Another observation I've made is that AI has a real, tangible benefit to society. Many of our customers are generating models that can predict protein folds, examine CAT scans, generate videos and images, provide possible diagnoses to health issues, and lots of other incredible uses. In fact I have been increasingly using chatbots like ChatGPT to distill complex technical topics and ask it for inspiration on troubleshooting esoteric problems. (1)
{ .annotate }

1. This deserves a whole blog post on its own, but I made the journey from extreme skepticism to whole-hearted believer when it comes to chatbots in the workplace. I don't view things like ChatGPT as a replacement for the human brain, but rather a more powerful alternative to Google and Stack Overflow. It still comes with the usual caveat of "don't believe everything you see on the internet" because it can be wrong in big ways!

<div class="annotate" markdown>
The cool thing about startups, especially ones with such meteoric growth, is that it's relatively easy to make a big impact. This is contrasted to larger companies (like Jump) that have a somewhat entrenched technical culture and already have "the way" of doing things. (1) In just a few short months, I was able to identify a huge business need that Lambda had around customer VM observability. Specifically, the need for us to ship customers metrics about their own VMs. Every public cloud deals with the same question and the solution usually looks something like:

1. Install a metrics collection service on the VM.
2. Ship the collected metrics to a hosted data store.
3. Expose the data store through some kind of API gateway so customers can access it directly (either through a UI or API).
4. Sell the metrics for profit!
</div>

1. An entrenched technology culture by itself is not a bad thing because it often means that an organization has found a solution that works well enough. However it does mean finding ways to make company-wide impacts is sometimes a fruitless effort. In the worst cases, it means that the company has become so ossified that making dramatic business pivots is exceedingly difficult

