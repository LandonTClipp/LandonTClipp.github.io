---
date: 2026-11-26
categories:
  - Programming
  - System Design
title: "Architecting the Blackwall"
description: On the frontlines of AI cyber warfare, and the increasingly uncomfortable parallels between my job and the dystopian world of Cyberpunk 2077.
---

![](https://f005.backblazeb2.com/file/landons-blog/assets/posts/2026-11-24-architecting-the-blackwall/blackwall.jpeg){ style="width: 100%; height: 300px; object-fit: cover; object-position: 0 30%" }

There is a certain irony in having spent years fascinated by dystopian science fiction, only to wake up one day and realize that your profession is beginning to resemble it. Of all the fictional universes I have immersed myself in throughout my life, none has become quite as disturbingly relevant to my work as Cyberpunk 2077.

<!-- more -->

I lead CoreWeave's secure container runtime efforts for untrusted workloads. My job, in its simplest conceptualization, is to build walls around computation that we cannot trust. Increasingly, the intelligences capable of attacking those walls are not human.

I want to explain why I believe one of the most consequential battles of our technological civilization is beginning to unfold, why I find myself in the middle of it, and why a video game released in 2020 provides such an eerily prescient depiction of the architectural and philosophical problems that now occupy my professional life.

## The Blackwall

In the universe of Cyberpunk 2077, humanity once enjoyed a vast, interconnected cyberspace not entirely unlike the internet we know today. That civilization suffered a catastrophic event known as the DataKrash, which devastated the digital infrastructure upon which humanity had become dependent. What remained of the Old Net became a wilderness inhabited by rogue artificial intelligences, some of which were extraordinarily powerful and fundamentally alien to human understanding.

Humanity had created intelligences that it could no longer reliably control. Unable to reclaim the entirety of cyberspace, an organization known as NetWatch constructed an enormous digital barrier called the [Blackwall](https://cyberpunk.fandom.com/wiki/Blackwall), separating the remnants of humanity's digital civilization from the hostile intelligences beyond it.

> When attempts to rebuild the Net after Rache Bartmoss' attack turned out to be futile and feral post-Krash AIs began to threaten corporations (often the same ones who created them), NetWatch decided to cut its losses. Dozens of the agency's finest netrunners created what came to be known as the Blackwall — a protective barrier keeping the fragmented, unstable (but still usable) Net safe from the rogue AIs roaming the ruins of cyberspace.

> The Blackwall came without warning, out of nowhere, which meant that any netrunners left stranded on the other side had no way to safely jack out nor survive indefinitely in cyberspace. As a result, NetWatch managed to kill two birds with one stone — neutralizing the threat of AIs and getting rid of troublesome hackers. The price of the Blackwall meant abandoning major technological advancements that came with self-aware AIs. Nevertheless, NetWatch made up for it by creating new tools that would grant them even greater access over the Net.

> For years, corporate propaganda glorified NetWatch agents as defenders of the people and, through the Blackwall, humanity's last line of defense. But mysterious raids in cyberspace (that agents are not always able to cover-up) prove that the "wall" maybe isn't as strong as we think it is...[6]

![](https://f005.backblazeb2.com/file/landons-blog/assets/posts/2026-11-24-architecting-the-blackwall/The_Blackwall_Database_CP2077.jpeg)

The visual depiction of the Blackwall is hauntingly beautiful. An incomprehensibly vast plane of red light, constructed from innumerable points suspended in the darkness of cyberspace. Beyond it exists something that humanity fears, something it created but no longer possesses the means to adequately understand or destroy.

The netrunners of NetWatch are tasked with maintaining this boundary. They understand that the safety of civilization depends, in part, on the integrity of a structure that most of humanity will never see, much less understand. They wage a largely invisible battle against entities whose capabilities threaten to exceed their own.

For reasons that I never could have anticipated when I first played Cyberpunk, I have come to identify quite strongly with these fictional people.

## The war has already begun

I think most people are fundamentally unaware of what is beginning to happen in cybersecurity. Even among those who work in the technology industry, I suspect relatively few appreciate the magnitude of the transformation that is taking place.

For decades, computer security has been a battle of human ingenuity. On one side are the people who construct increasingly sophisticated computational systems, and on the other are the people who dedicate their intellect to discovering the flaws in those systems. The rate at which vulnerabilities could be discovered and exploited was constrained, at least in part, by the finite intellectual capacity and availability of human beings. That constraint is beginning to disappear.

I have had the opportunity to work with some of the most capable frontier cybersecurity models in existence. I can point these models at incredibly sophisticated pieces of software, give them an objective, and watch them reason about systems that would take an experienced engineer considerable time to understand. They can formulate hypotheses, investigate their validity, uncover subtle architectural assumptions, and sometimes discover vulnerabilities that have lain dormant for years. They are unbelievably, frighteningly good.

What is particularly terrifying to me is not that artificial intelligence can discover vulnerabilities. It is the prospect that the collective intellectual effort historically required to discover those vulnerabilities is becoming a commodity. Expertise that once took a human being decades to cultivate can increasingly be deployed in parallel, at machine speed, and with a persistence that no human being could physically sustain.

Imagine the collective efforts of thousands of exceptionally talented security researchers, working continuously, never becoming tired, never becoming bored, and systematically interrogating every conceivable assumption upon which your infrastructure depends. Now imagine that the cost of deploying that capability continues to plummet.

What happens when every sufficiently motivated threat actor on Earth can summon that army? This is the question that keeps me awake at night.

## The gates of hell

Today, some of the most capable cybersecurity models are subject to access restrictions. There are institutions that retain meaningful control over who can employ certain capabilities and under what circumstances. Whatever one thinks of those institutions, there is at least some friction between the existence of a dangerous capability and its proliferation throughout the world. I do not believe that this condition will persist indefinitely.

Mark my words: there will come a day when open-weight AI models possess cybersecurity capabilities equal to or greater than those of today's most powerful restricted models. They will become downloadable artifacts, reproducible across thousands of machines, accessible to people whose intentions we have absolutely no means of discerning.

There will be no meaningful way to recall them. There will be no institution to petition for the termination of their operation, no centralized authority capable of revoking access, and no reliable means of preventing their replication. On that day, the floodgates will have opened.

The accumulated technical debt of our digital civilization will be subjected to a degree of scrutiny that it has never before experienced. Decades of latent vulnerabilities, architectural compromises, poorly understood legacy systems, and assumptions made by long-retired engineers will become the hunting grounds of utterly relentless artificial intelligences.

Our electrical grids, financial institutions, telecommunications networks, transportation systems, hospitals, and governments all depend upon software constructed in an era when the adversary was predominantly human.

We have spent decades constructing a civilization whose continued existence in its present form depends upon the integrity of extraordinarily complicated software. We are now creating the means to interrogate that software at a scale for which it was never designed to withstand. I fear that we are going to discover just how fragile many of the foundations of modern civilization really are.

This is not a prediction of an inevitable apocalypse. It is a warning about what happens when the capabilities of the adversary grow substantially faster than our capacity to secure the systems upon which we depend. I do not think we are adequately prepared for that possibility.

## Architecting the Blackwall

![](https://f005.backblazeb2.com/file/landons-blog/assets/posts/2026-11-24-architecting-the-blackwall/NetWatch_Database_CP2077.jpeg)

My particular corner of this battlefield exists at the intersection of operating systems, virtualization, hardware architecture, and artificial intelligence. The fundamental question that occupies my professional life is how to permit extraordinarily powerful computation to occur within an environment while preventing that computation from exercising unauthorized influence over the world outside of it. It is a deceptively simple problem statement. The architectural implications are enormous.

A modern GPU server is a remarkably complicated machine. CPUs, GPUs, hypervisors, operating systems, device firmware, memory controllers, networking hardware, PCIe and NVLink fabrics, NUMA topologies, and physical interconnects all cooperate to produce the illusion of a coherent computational environment. Every interface between these components represents another place where the assumptions underpinning our security architecture must hold.

My work involves understanding those interfaces, investigating their weaknesses, and constructing boundaries around potentially adversarial computation. Those boundaries must remain meaningful irrespective of the intentions or intellectual capabilities of the entity operating within them.

An intelligence can be made to understand that it is forbidden from performing a particular action. It can be instructed to behave ethically, constrained by its training, and subjected to various forms of behavioral oversight. These are all worthwhile endeavors, but from the perspective of a systems engineer, I must contemplate the possibility that such measures will fail. I cannot construct a security architecture whose integrity depends upon the cooperation of the very entity it is designed to contain.

This is why I believe so strongly in the importance of hardware-enforced isolation. We must strive to construct environments in which certain actions are not merely prohibited by policy, but are architecturally inaccessible. An intelligence should not be able to negotiate its way around a physical security boundary. Nor should the integrity of that boundary depend upon our ability to intellectually outmaneuver the intelligence on every conceivable occasion.

Of course, hardware itself is not infallible. Nothing we construct is. The art of security architecture lies in establishing sufficiently independent layers of protection that the failure of one does not precipitate the failure of the entire structure.

The Blackwall that I envision is not a luminous red hologram suspended in cyberspace. It is constructed from silicon, firmware, hypervisors, memory protection mechanisms, and carefully engineered software. Its foundations are laid in the physical architecture of the machines upon which these intelligences depend. But its philosophical purpose bears an extraordinary resemblance to that of its fictional counterpart. We are constructing a boundary between computation that we cannot necessarily trust and the world that we cannot afford to have compromised.

## Fighting the demons with demons

Perhaps the most fascinating aspect of the Blackwall in Cyberpunk is that the Blackwall is itself an artificial intelligence. Humanity enlisted the aid of one intelligence to protect itself from the others. There is something profoundly unsettling about this idea, and yet it is beginning to resemble the manner in which cybersecurity is evolving today.

I employ artificial intelligence to help defend against the capabilities of artificial intelligence. I direct these models toward the systems that I am responsible for securing and ask them to discover the weaknesses that another intelligence might one day attempt to exploit. The intellectual machinery that threatens to undermine our defenses is simultaneously becoming one of the most powerful instruments available to us for strengthening them.

There are days when I find myself contemplating the absurdity of what I am doing. I sit at my computer, directing artificial intelligences to interrogate the security boundaries of machines that will themselves host artificial intelligences, anticipating the actions of other artificial intelligences that may one day attempt to compromise them.

This would have sounded like the ramblings of a science fiction madman a decade ago. It is now an increasingly ordinary part of my professional existence. I suspect that much of the future of cybersecurity will consist of battles between artificial intelligences, with human beings serving as the architects, orchestrators, and custodians of the systems through which those battles are conducted.

Our responsibilities will increasingly involve directing intellectual forces that, in certain specialized domains, may eventually exceed our own capabilities. We will have to understand enough about these forces to employ them effectively while simultaneously constructing environments that do not surrender our authority to them.

The irony is not lost on me. In order to defend ourselves against the demons we have created, we find ourselves increasingly dependent upon the assistance of other demons.

## The exhaustion of the netrunner

![](https://f005.backblazeb2.com/file/landons-blog/assets/posts/2026-11-24-architecting-the-blackwall/Cyberspace_Database_CP2077.jpeg)

I wish I could adequately communicate what it feels like to work in this environment. There is a certain psychological burden associated with being responsible for the integrity of a security boundary while watching the capabilities of the adversary advance at an extraordinary pace. You spend your days contemplating how the systems you have built might fail, how your assumptions might be violated, and what consequences might unfold if you have overlooked something important.

Every vulnerability discovered represents both a victory and a reminder of the fragility of the systems we construct. You are grateful that it was discovered before a malicious actor could exploit it, but you cannot help wondering how many others exist beyond the limits of your present understanding. And tomorrow, you will wake up and do it again.

The artificial intelligences participating in this battle do not experience exhaustion. They do not need to step away from their computers, retreat into the mountains, or contend with the innumerable complications of being a human being. Their capacity for intellectual labor is constrained by computational resources rather than by the biological limitations that govern my own existence. I am beginning to understand the peculiar asymmetry of being a human participant in an increasingly automated battle of intellect.

To be honest, I'm exhausted, but I am also extraordinarily fascinated by what is happening. I cannot think of another period in the history of computer science in which the work of a systems engineer has intersected so profoundly with questions about the future relationship between humanity and its own creations.

I have spent much of my adult life cultivating an understanding of the invisible machinery upon which our digital world depends. Never did I imagine that this knowledge would become relevant to a problem of this nature, nor that I would find myself in a position to help determine how some of these boundaries are constructed. There are moments when the significance of that responsibility is almost overwhelming.

## The hubris of humanity

Cyberpunk is fundamentally a story about human hubris. It depicts a civilization that acquired extraordinary technological capabilities before it acquired the wisdom or institutional maturity necessary to wield them responsibly. The corporations of Night City continue to seek access to the intelligences beyond the Blackwall because the power they represent is simply too enticing to ignore. The potential rewards of crossing that boundary are enormous, even when the consequences of doing so are poorly understood. Does any of this sound familiar?

We are engaged in a technological race of unprecedented ambition. The economic incentives associated with creating increasingly capable artificial intelligence are enormous. The companies and nations participating in this race cannot easily afford to relinquish their competitive position, and the consequences of falling behind are perceived to be sufficiently grave that continued acceleration becomes an almost inescapable imperative.

I am not opposed to this technology. Quite the contrary, I am fascinated by it, and I believe it possesses the potential to bring about extraordinary improvements in the human condition. I have dedicated my professional life to building infrastructure that enables its existence. But I also believe that our capacity to create something does not imply a capacity to control it. History is replete with examples of humanity discovering this distinction only after suffering the consequences of its ignorance.

The tragedy of the Blackwall is that it was constructed after humanity had already lost control of the Old Net. It was a monument to the failures of a civilization that had underestimated the consequences of its own technological ingenuity. We have the opportunity to learn from that fictional tragedy before we are confronted with an equivalent architectural crisis of our own.

## Before the DataKrash

I do not know what the world will look like in ten years. I do not know whether artificial intelligence will develop into something resembling the rogue intelligences depicted in Cyberpunk, nor do I believe that consciousness or independent malicious intent is necessary for this technology to become profoundly dangerous.

What I do know is that we are constructing increasingly capable intelligences and granting them increasingly sophisticated means of interacting with the computational infrastructure upon which our civilization depends. The implications of these two developments occurring simultaneously ought to command the attention of every serious systems engineer on the planet.

I want to spend this chapter of my life constructing the architectures that will allow humanity to benefit from these intelligences without surrendering control of the machinery upon which they operate. I want to build security boundaries whose integrity does not depend upon the intellectual inferiority of the adversary. And I want to do so while we still possess the luxury of designing these systems proactively, rather than amid the wreckage of a catastrophe that has demonstrated their necessity.

Perhaps there is something admittedly grandiose about identifying with the netrunners of a fictional dystopia. I am, after all, a systems engineer. Much of my day consists of reading source code, attending meetings, and staring at Linux kernel logs. There are no neon-lit streets outside my window, no cybernetic implants in my skull, and no enormous red hologram separating me from the horrors of the Old Net.

But the underlying architectural problem is real. The technological forces that give rise to it are real. And the consequences of failing to adequately address it may one day be felt by people who will never know the names of the engineers responsible for constructing these defenses. I cannot imagine a more fascinating time to be alive, nor a more consequential time to be working in my particular corner of computer science.

Somewhere in the vast computational machinery that humanity is constructing, the boundaries between intelligence, autonomy, and control are beginning to be tested in ways that few of us could have imagined when we entered this profession.

The hounds of hell are coming. I intend to spend the foreseeable future helping to build the wall that stands between them and us. Let us hope that we build it before the DataKrash.
