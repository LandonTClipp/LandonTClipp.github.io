---
date: 2024-10-07
categories:
  - RV
  - Intech Sol Horizon
title: Starlink for a Mobile World
description: Installing Starlink on an RV.
draft: true
links:
  - blog/posts/2023-06-21-intech-sol-horizon-cellular.md
---

In a [previous blog post of mine](2023-06-21-intech-sol-horizon-cellular.md), I retrofitted my camper trailer with a cellular internet solution for remote work. After having over a year to gather data on the practical, real-world performance of the Peplink router, I've come to the realization that I am a huge nerd and want even more technology! In this post, I'll show you my journey with Starlink and my impressions with its usefulness as a remote-work solution.

<!-- more -->

## Background

You may be asking yourself whether Starlink is really _necessary_ for a remote working solution, to which I'm happy to proclaim a resounding _no_. I suppose I shouldn't be so dogmatic with that determination because it certainly depends on where exactly you plan on travelling. If you want to stay near civilization in any capacity, cellular internet at this point in 2024 is in almost all respects superior to anything else. It's not affected by weather (1) or trees, and it's available in nearly all locations that contain a small critical mass of people.
{ .annotate }

1. For the most part, obviously some cell towers rely on microwave relays to locations with real underground cables.

It didn't use to be this way. I remember the days when cell technology was atrocious, and getting access to a tower with decent throughput was a bit like winning a $2 lottery (and equally as satisfying. Kind of cool until you realize that you have to be glued to that one cell tower. Travel anywhere else and you're SOL). With advances in signal processing and increased bandwidth sales from the FCC, cell has come a long way.

Over the last year with my Peplink solution, I found that if I was ever near any small town, or any interstate at all, I almost always had access to decent cell coverage. In the instances where I didn't have good cell coverage, 90% of those situations wouldn't have been saved by Elon's solution due to tree coverage. So you may ask, why get Starlink? I already told you, I'm a technology nerd and I like to brag to people about how cool my remote work solution is.

Jokes aside, the real reason comes from the recent product improvements Starlink has made, namely:

1. Reduction in price of their antenna.
2. Full support for internet-in-motion.
3. Data plans tailored specifically to RVers. Their mobile plans offer unlimited data and the ability to pause and restart your plan at will. This is the killer feature that I was awaiting because being tied to inflexible billing schemes or plans that require stationary antennas were all non-starters.
4. Increased reliability with the ongoing additions to their constellation.

## What's Included

When you buy the Starlink antenna, it comes with three components:

1. The antenna itself
2. A WiFi router
3. An AC-DC adapter

<div class="grid cards" markdown>

- ![Antenna](https://f005.backblazeb2.com/file/landons-blog/assets/images/blog/2024-10-07-starlink-mobile/IMG_6149_down.jpg){data-gallery="all"}
- ![Router and ACDC](https://f005.backblazeb2.com/file/landons-blog/assets/images/blog/2024-10-07-starlink-mobile/IMG_6150_down.jpg){data-gallery="all"}

</div>

Any semi-experienced RVer will immediately balk at the idea of running an ACDC adapter due to the fact that the DC battery from the RV has to be inverted to 60Hz AC, then converted back to DC by the adapter. You might also balk at the idea of running a separate router when we already have our Peplink Cellular modem/router! Not to fear, as a quick search on the internet shows that technically speaking, the Starlink antenna does not _need_ these two components to operate. 

There exist third-party Starlink PoE conversion kits that are powered by 12V DC and will step up to 50V DC required by the antenna. The "ethernet" cable that the antenna uses is a proprietary form factor despite having the same number of wiring pins as ethernet. These conversion kits will bridge the gap between the Starlink ethernet, which requires the 50V PoE line, and the standard ethernet that you can send directly to your router. The other benefit is that it cuts out the Starlink-specific router which further aids in power efficiency.

An example: https://www.amazon.com/XLTTYWL-Starlink-Protection-Conversion-Converter/dp/B0D4DH9TDN/ref=sr_1_1_sspa?crid=3MSWDXF0OT12S&dib=eyJ2IjoiMSJ9.qP1_Af53E-CvdtPv7Z8Ofgdi-WmGsrNUh_pUqCU7mqM0kg8eZHiREHnkRnKASukvw53-zE8QEdZO6suABv0KcHQfiospY-KimIqMMPnpQbNk5K_vh80IUghh1iwSzbiQcKmUpalRPqlGA16_wXySqkvt3eF_s-pXVME5gYX7Z61T9Rgdj0WD-E69bUHeRGb-Nd1BDfqmMsqpqLw6JJXGCyLvUgLKZRXsSs1KrRBXsITz7TCcLh7Mjng6T_IEG8nM4wyL9_NQabBKBn7Mygp1wjrQd02OEE3Kvdvs1Af6LkE.vc0XT6Ve0TMX39b01i6X0JvnvOz-mPwDYB2KpDLSJfA&dib_tag=se&keywords=starlink+gen+3+dc+conversion+kit&qid=1728328253&s=electronics&sprefix=starlink+gen+3+dc+conversion+ki%2Celectronics%2C171&sr=1-1-spons&sp_csd=d2lkZ2V0TmFtZT1zcF9hdGY&psc=1

For my proof-of-concept build, I will just use all the standard Starlink equipment and optimize it with a conversion kit at a later date.

## Setup

The setup was surprisingly simple. I downloaded the Starlink app and it took me through a series of setup tasks. After having plugged in the ACDC converter, the router, and the antenna together, the system for the most part set itself up. The antenna determines what cardinal direction it's pointing and will inform you that it prefers to point north. I found this wasn't strictly necessary and was in fact ill-advised because to my north was a large oak tree. Keeping it pointed south-west was totally fine for my environment.

<div class="grid cards" markdown>

- ![Obstruction visualization](https://f005.backblazeb2.com/file/landons-blog/assets/images/blog/2024-10-07-starlink-mobile/IMG_6152_down.jpg){data-gallery="all"}
- ![Obstruction wide-angle](https://f005.backblazeb2.com/file/landons-blog/assets/images/blog/2024-10-07-starlink-mobile/IMG_6154_down.jpg){data-gallery="all"}

</div>

The obstruction visualization had me scan the sky with my phone's camera. I'm assuming it does some rudimentary "blue pixels good, anything else bad" algorithm where it determined the tree to be disruptive. The app also gives fairly detailed metrics into things like power draw, ping success rate, latency, throughput, and outage timelines.

## Performance

<div class="grid cards" markdown>

- ![Speed test night](https://f005.backblazeb2.com/file/landons-blog/assets/images/blog/2024-10-07-starlink-mobile/IMG_6148_down.jpg){data-gallery="all"}
- ![Speed test day](https://f005.backblazeb2.com/file/landons-blog/assets/images/blog/2024-10-07-starlink-mobile/IMG_6155_down.jpg){data-gallery="all"}
- ![Statistics](https://f005.backblazeb2.com/file/landons-blog/assets/images/blog/2024-10-07-starlink-mobile/IMG_6156_down.jpg){data-gallery="all"}

</div>

I spent a night in the Middle of Nowhere Nebraska and ran a few tests. The first was a simple speed test in the middle of the night, where I was able to pull 147 Mbps down. The next speed test I took was at around 2:30PM the following day where I got 41Mbps down/28Mbps up. While that's over three times slower than the middle of the night, it's still fast enough for most activities.

Ping success rate, for my purposes, is the most important metric. I was able to get around 90% success rate in the middle of the day which for me is a bit sad. 