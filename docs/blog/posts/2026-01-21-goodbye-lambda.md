---
date: 2026-01-21
categories:
- Career
title: Goodbye Lambda, Hello CoreWeave
description: A proper farewell to a group I hope to one day work with again.
---

Today I did something that I didn't expect to be so hard, I'm leaving behind a company I've dedicated the last 2 years of my life to. I'm leaving Lambda and joining CoreWeave. This post will explain why.

![](https://f005.backblazeb2.com/file/landons-blog/assets/posts/2026-01-21-goodbye-lambda/coreweave.png){ style="background-color: #FFFFFF;"}

<!-- more -->

## Background

I joined Lambda Februrary of 2024. Lambda, for those of you who don't know, is a Neo-Cloud company that builds and operates supercomputing clusters built specifically for Machine Learning workloads. An additional software layer lives on top of these clusters that lets multiple customers rent access to the machine. This middle layer is what I worked on, specifically the VM Control Plane that scheduled and placed customer VMs onto the hardware. 

At the time I joined, my team operated pretty much the whole shebang from the time the customer's VM request came from our front-end systems all the way to instantiating it on the hosts, including managing all of the performance-related problems that come with virtualization. I was fortunate to lead some large technical initiatives at the company, the one I'm most proud of being the [Lambda Guest Agent](https://docs.lambda.ai/public-cloud/guest-agent/). You can see some pictures of the frontend of this project below:

![](https://f005.backblazeb2.com/file/landons-blog/assets/posts/2026-01-21-goodbye-lambda/view-metrics.png)
![](https://f005.backblazeb2.com/file/landons-blog/assets/posts/2026-01-21-goodbye-lambda/graphs-1.png)

I was able to lead this initiative because Lambda was in a unique phase in its journey. They were still early enough that there was room for engineers to step into unusually broad ownership. I was able to step into this role as an IC engineer because of these circumstances, but it also helped that everybody loves pretty graphs and rallying people behind the idea that our customers deserve pretty graphs is something we should do. This is not something that would be possible nowadays (at least in the way I did it) for a cowboy engineer like myself because Lambda is much more operationally mature, but it worked out at the time.

The Guest Agent was the most visible project I worked on, but I'm also proud of other initiatives I tackled both small and large that improved Lambda's reliability and performance. I reworked how Lambda's Jupyter Notebooks were exposed to the public internet which insulated us from a number of Cloudflare outages. I contributed to Lambda's [1-Click Cluster](https://lambda.ai/1-click-clusters) that gave customers access to virtual machines networked together through high performance Infiniband fabrics. I improved our internal visibility into GPU health so we could better detect when hardware went bad (here's some homework: how do you do this without access to NVIDIA Kernel Drivers or well-supported Redfish endpoints? It's not easy!). I was also instrumental in representing my team during massive datacenter buildouts in Dallas and Austin where we delivered these new clusters to customers.

## A Change in Teams

Around early 2025, I moved to a newly minted team called Special Projects whose charter was to build high-risk and possibly nebulous AI-related projects that the core business did not feel comfortable dedicating normal engineering time on. This is where I ended up researching how to do GPU-based Containers as a Service (CaaS), specifically using Kata containers. You can see the results of my research [here](2025-10-21-gpu-containers-as-a-service.md) and [here](2025-11-19-gpu-passthrough-boot.md). I built a proof-of-concept cluster on two Supermicro HGX H100 servers and presented a working example during a company show-and-tell to much excitement. This was my proudest achievement on the team because this excitement eventually spilled outside the company and into the LinkedIn blogosphere where my work was featured in [Kubernetes-specific publications](https://www.linkedin.com/posts/landonclipp_this-article-explains-how-to-build-a-multi-tenant-activity-7414706340327239680-iV0y).

<div class="center">
<iframe src="https://www.linkedin.com/embed/feed/update/urn:li:share:7414706339819618304" height="712" width="504" frameborder="0" allowfullscreen="" title="Embedded post"></iframe>
</div>

I was neither a Kubernetes expert nor a virtualization expert, but I learned enough to make this work and meet the criteria I set out for myself. I was able to get a VM with 8 H100 GPUs attached to it to boot from 30 minutes down to almost 1 through much collaboration with the contributors to Kata Containers. This was proof to me that I don't need to be an expert on something to create value, I just need dogged determination, a willingness to learn, and (most importantly) relationships with the right people. Nothing is impossible without a little bit (or even a lot) of research.

## A Change in Priorities

With such a successful career at Lambda, people must be asking why would I want to leave? Was I upset? Was I angry? Was I unhappy? The answers are no, no, and kind of. I was unhappy not because anything was _wrong_ per se, but because it ended up being the case that Lambda could not absorb my ambitions anymore. Just because I build something cool and interesting does not mean the business needs or wants it right now, and sometimes even if they do need and want it, it may not be the right time. This is where I found myself; I had proven something like this could be built, I was getting an overwhelmingly positive signal from the external market, and yet the company I worked at could not yet absorb it. 

This presented to me a real fork in the road. I could either spend a lot of political capital trying to convince the business to let me build this, which wouldn't have been in their best interests anyway, or I could go to somewhere that has explicit needs for the kinds of skills and research I acquired in the course of building out this PoC. This is where CoreWeave came into play. It's actually not an opportunity I seeked out, but rather a match that landed serendipitously into my lap.

## Strategically Clear, Emotionally Hard

My decision to leave Lambda was simultaneously one of the easiest and most difficult ones to make. It was the easiest decision because the alignment between CoreWeave and I could not have been better matched. It was the hardest decision because I honestly, although perhaps a bit naively, thought that Lambda would be a long-term home for me. I certainly wanted it as a home because I loved the people, I loved the cool shit we were able to build together, and riding on the crest of this AI wave in the boat of this scrappy startup with wicked smart people all rowing together was an experience I'll always miss. Having to break the news to my colleagues made my stomach twist. I don't say that as a corporate shill, I say it genuinely because I felt a real camaraderie with the people here and I have to fight back these feelings that I somehow failed them. I know I didn't fail them, nobody failed anybody, and yet that's the feeling anyway.

Some of the folks that are top of mind for me: Jeff Nadler, who shares my love of observability, trusted me enough to let me take charge on the guest-agent project back in early 2025. We have a shared understanding of what makes engineering teams great and what doesn’t. Paul Sebexen saw my potential early and pulled me into high-stakes, long-form, and sometimes obscure projects because he trusted me to do the right thing. I also want to shout out Michael Balaban who took time out of his late night to talk to me one on one after a project I had been deeply invested in was cut last minute. This was an act of sympathy I’ll never forget, one that turned a bad situation into a story of resilience.

I can't express how excited I am to turn this new chapter with CoreWeave. Anyone who knows anything about AI infrastructure has heard of them, and I have a lot to learn from the incredible work they've done. I can't share any details on plans, but all I can say is that you'll see lots of open source work from me on Kata Containers and hopefully a number of public talks at KubeCon on the subject. I'll continue to collaborate with Zvonko Kaiser and team from NVIDIA on this cool project to build out new capabilities for AI researchers.

As I've already shared in my [AI Infra Thesis](2026-01-01-ai-infra-thesis.md), I'm willing to bet the next few years of my career that the platform in which AI Researchers use to train and serve their models will fundamentally change. **Compute will become democratized amongst the people and barriers to entry will fall.** I hope that one day AI Researchers never have to touch a VM again in their life. That's the goal, at least.
