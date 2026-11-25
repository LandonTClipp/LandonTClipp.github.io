---
date: 2026-11-24
categories:
- Programming
- System Design
title: "Architecting the Blackwall"
description: On the frontlines of AI cyber warfare, and the uncomfortable parallels between my job and the dystopian world of Cyberpunk 2077.
---

![](https://f005.backblazeb2.com/file/landons-blog/assets/posts/2026-11-24-architecting-the-blackwall/blackwall.jpeg){ style="width: 100%; height: 300px; object-fit: cover; object-position: 0 30%" }

There is a certain irony in spending years fascinated by dystopian science fiction, only to wake up one day and realize your profession has started to resemble it. Of all the fictional universes I have immersed myself in, none feels quite as disturbingly relevant to my work as Cyberpunk 2077.

<!-- more -->

I lead CoreWeave's secure container runtime efforts for untrusted workloads. In its simplest form, my job is to build walls around computation we cannot trust. More and more, the intelligences probing those walls are not human.

One of the most consequential battles of our technological civilization is now unfolding, and I find myself standing in the middle of it. A video game released in 2020 offers an eerily prescient lens into the architectural and philosophical problems that occupy my work.

## The Blackwall

In the universe of Cyberpunk 2077, humanity once relied on a vast, interconnected cyberspace much like our internet. Then came the DataKrash, a catastrophic attack that shattered the world's digital infrastructure. What remained of the Old Net became a digital wilderness haunted by rogue artificial intelligences, some immensely powerful and entirely alien to human thought.

Humanity had created intelligences it could no longer control. Unable to reclaim the Old Net, an organization called NetWatch raised an enormous digital barrier called the [Blackwall](https://cyberpunk.fandom.com/wiki/Blackwall) to wall off the habitable fragments of cyberspace from the feral AIs roaming the ruins.

Dozens of NetWatch's best netrunners built the wall to keep what remained of the Net usable. Later raids raised doubts about how much protection it really offered.

![](https://f005.backblazeb2.com/file/landons-blog/assets/posts/2026-11-24-architecting-the-blackwall/The_Blackwall_Database_CP2077.jpeg)

In the game, the Blackwall is hauntingly beautiful: a vast plane of red light woven from countless points suspended in the dark of cyberspace. Beyond it lurks something humanity created, fears, and no longer knows how to contain or destroy.

NetWatch's netrunners are tasked with holding that line. They know civilization's safety rests on the integrity of a barrier most people will never see, let alone comprehend, and they wage an invisible war against entities whose reach threatens to outstrip their own.

For reasons I never could have anticipated when I first played Cyberpunk, I have come to identify closely with them.

## The war has already begun

Most people are unaware of what is happening in cybersecurity right now. Even within the tech industry, few appreciate the scale of the shift underway.

For decades, computer security has been a contest of human ingenuity: builders designing ever more complex systems on one side, researchers hunting for flaws on the other. The pace of discovery and exploitation was bounded by the finite attention and availability of human minds. That constraint is evaporating.

I have worked directly with some of the most capable frontier cybersecurity models in existence. You can point them at intricate codebases, give them an objective, and watch them reason through systems that would take a seasoned engineer weeks to map. They form hypotheses, test them, uncover subtle architectural assumptions, and surface vulnerabilities that have lain dormant for years. They are frighteningly good.

What unsettles me most is not simply that AI can find bugs. It is that the intellectual labor required to find them is becoming a commodity. Expertise that once took a human decades to build can now run in parallel, at machine speed, with a persistence no person could sustain.

Picture thousands of elite security researchers working around the clock, never tiring, never losing focus, systematically interrogating every assumption your infrastructure rests on. Now picture the cost of summoning that workforce dropping toward zero.

What happens when every motivated threat actor on Earth can command that army? That is the question that keeps me awake at night.

## The gates of hell

Today, the strongest frontier security models sit behind access controls. A handful of institutions still govern who can use them and under what terms. Whatever one thinks of those gatekeepers, they create friction between a dangerous capability and its global proliferation. That friction will not last.

Mark my words: open-weight models will eventually match or surpass today's most powerful restricted systems at offensive security. They will become downloadable artifacts that can run on thousands of machines and be made available to anyone whose intentions we have no way of knowing.

Once released, there will be no recalling them. No institution to petition to shut them down, no central authority to revoke access, no reliable way to halt their replication. On that day, the floodgates open.

The accumulated technical debt of our digital civilization will face unprecedented scrutiny. Decades of latent bugs, architectural shortcuts, poorly understood legacy code, and assumptions left behind by long-retired engineers will become the hunting grounds of relentless machines.

Our power grids, financial systems, telecom networks, transit infrastructure, hospitals, and governments all run on software built for an era when the adversary was human.

We have spent decades building a civilization that depends on the integrity of immensely complex software, and we are now creating tools to interrogate that software at a scale it was never built to withstand. We are about to discover just how fragile many of our foundations really are.

This is not a prophecy of inevitable collapse (although it very well could be if we don't prepare). It is a warning about what happens when offensive capability outpaces our ability to defend the systems we rely on. Right now, we are not ready.

## Architecting the Blackwall

![](https://f005.backblazeb2.com/file/landons-blog/assets/posts/2026-11-24-architecting-the-blackwall/NetWatch_Database_CP2077.jpeg)

My corner of this battlefield sits at the intersection of operating systems, virtualization, hardware architecture, and AI. The core problem occupying my work is simple to state and massive in its architectural implications: how do you let immensely powerful computation run inside an environment while preventing it from exerting unauthorized influence over the world outside?

A modern GPU server is a remarkably intricate machine. CPUs, GPUs, hypervisors, operating systems, device firmware, memory controllers, NICs, PCIe and NVLink fabrics, NUMA topologies, and physical interconnects all cooperate to sustain the illusion of a single coherent computer. Every seam between those layers is another place where our security assumptions must hold.

My work is to map those seams, probe their weaknesses, and build containment boundaries around potentially adversarial workloads. Those boundaries have to hold regardless of the intent or intelligence of whatever runs inside them.

You can teach a model that an action is forbidden. You can train it to behave ethically and layer behavioral oversight on top. Those efforts matter, yet as a systems engineer, I have to assume they will eventually fail. I cannot design a security architecture whose integrity relies on the cooperation of the very entity it is meant to contain.

That is why hardware-enforced isolation is non-negotiable. We need execution environments where prohibited actions are not merely against policy, but architecturally unreachable. An intelligence should not be able to talk its way past a physical memory boundary, nor should containment depend on our ability to outthink the model on every possible edge case.

Hardware can fail, of course. Anything we build can. The craft of security architecture lies in layering independent defenses so that a breach in one never collapses the whole. The Blackwall I envision is not a glowing red hologram suspended in cyberspace. It is built from silicon, firmware, microVMs, IOMMU isolation, memory-protection hardware, and tightly audited kernel code. Its foundations sit in the physical architecture of the machines these models run on, yet its purpose mirrors its fictional namesake: separating computation we cannot trust from a world we cannot afford to lose.

## Fighting the demons with demons

One of the most unsettling details about Cyberpunk's Blackwall is that the wall is itself an AI. Humanity enlisted one intelligence to hold back the others, and that dynamic is starting to mirror modern security engineering.

I use AI to defend against AI. I point frontier models at the systems I secure and task them with finding the weaknesses another model might one day exploit. The same machinery threatening our defenses is quickly becoming our sharpest instrument for hardening them. Some days, the recursion feels almost absurd. I sit at my desk directing artificial intelligences to audit the isolation boundaries of machines built to host artificial intelligences, trying to anticipate the moves of yet other intelligences that will one day try to break out.

Ten years ago, that sentence would have sounded like sci-fi delirium. Today, it is routine. Much of cybersecurity's future will be fought between models, with humans acting as the architects, orchestrators, and custodians of the arenas where those battles unfold. Our job will increasingly mean steering intellectual forces that, in narrow domains, surpass our own. We will need to understand them well enough to wield them safely while building environments that never surrender final authority.

The irony is not lost on me. To defend ourselves from the demons we have summoned, we are leaning ever more heavily on other demons.

## The exhaustion of the netrunner

![](https://f005.backblazeb2.com/file/landons-blog/assets/posts/2026-11-24-architecting-the-blackwall/Cyberspace_Database_CP2077.jpeg)

It is hard to capture what it feels like to work inside this shift. Carrying responsibility for a security boundary while watching adversarial capabilities accelerate at breakneck speed takes a quiet psychological toll. You spend your days thinking through how your systems might fail, which assumptions might break, and what happens if you missed something subtle.

Every vulnerability found is both a small victory and a reminder of how fragile our systems are. You are grateful you caught it before an attacker did, yet you cannot stop wondering how many others still hide past the edge of your visibility. Tomorrow, you wake up and do it again.

The models fighting on this front do not get tired. They do not need to step away from the terminal, retreat into the mountains, or navigate the messy realities of being human. Their stamina is bounded by FLOPs and power budgets rather than biology. There is a strange asymmetry to being a human participant in an automated battle of intellect.

To be honest, I am exhausted. I am also completely captivated. I cannot think of another moment in computing when systems engineering has collided so directly with fundamental questions about humanity's relationship with its own creations.

I have spent much of my adult life learning the invisible machinery that keeps our digital world running. I never imagined that craft would land me on this frontline, helping decide where the containment boundaries get drawn. Some days, the weight of that responsibility is humbling.

## The hubris of humanity

At its core, Cyberpunk is a story about hubris. A civilization acquired godlike tools long before it had the wisdom or institutional maturity to wield them. Night City's megacorporations keep poking holes in the Blackwall because the power waiting on the other side is too tempting to leave alone, even when nobody fully grasps the fallout of crossing over. Sound familiar?

We are locked in a technological race of unprecedented ambition. The economic incentives to push frontier AI forward are massive. Neither companies nor nations can easily afford to slow down when falling behind looks existential, turning relentless acceleration into the default.

I am not against this technology. Quite the opposite. I am fascinated by it, and I believe it can dramatically improve the human condition. I have dedicated my career to building the infrastructure that makes it possible. Yet the ability to create something does not guarantee the ability to control it, and history is full of civilizations learning that difference the hard way.

The tragedy of the Blackwall is that it was raised only after humanity had already lost the Old Net. The wall stands as a monument to underestimating the fallout of our own ingenuity. We still have a window to learn from that fiction before we face an architectural crisis of our own.

## Before the DataKrash

I do not know what the world will look like in ten years. I do not know whether AI will ever resemble the rogue entities of Cyberpunk, nor do I think consciousness or malice is required for these systems to become profoundly dangerous.

What I do know is that we are building stronger models while giving them deeper access to the computational infrastructure our civilization runs on. Those two trends colliding should command the attention of every serious systems engineer alive.

I want to spend this chapter of my career designing the architectures that let humanity benefit from these systems without surrendering control of the hardware beneath them. I want to build containment boundaries that do not rely on the adversary being less intelligent than we are. I want to build them while we still have the luxury of foresight, rather than amid the wreckage of a crisis that proved them necessary.

Perhaps there is something slightly grandiose about identifying with the netrunners of a fictional dystopia. I am, after all, a systems engineer. Most of my day goes to reading source code, sitting in meetings, and staring at Linux kernel logs. There are no neon-lit streets outside my window, no cybernetic implants in my skull, and no towering red hologram separating me from the Old Net.

Yet the architectural problem is real, the forces driving it are real, and the cost of getting it wrong may one day fall on people who will never know the names of the engineers who built the defenses. I cannot imagine a more fascinating time to be alive, or a more consequential moment to work in this corner of computer science.

The hounds of hell are coming. I intend to spend the foreseeable future helping build the wall between them and us. Let us hope we finish it before the DataKrash.
