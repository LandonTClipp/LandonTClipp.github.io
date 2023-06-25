---
date: 2023-06-21
categories:
  - RV
title: Intech Sol Horizon Cellular Setup
draft: true
---

Intech Sol Horizon Cellular Setup
==================================

In 2022, I bought myself an [Intech Sol Horizon Rover](https://intech.com/rv/models/sol/horizon/). I had been looking at RVs for a long time and decided on this particular manufacturer because of the excellent build quality and reputation of the manufacturer. The Horizon was a good fit for my needs because it was spacious enough to be comfortable for two adults to live in for long periods of time, and small enough that I could tow it with my Jeep Grand Cherokee. My goal for this RV is to be something that I could work remotely from, which means I needed to get a reliable internet setup. This post will show you my journey with finding the right internet solution, the lessons I learned, and the resources I used to make this a successful build.

<!-- more -->

The Rig
--------

<div class="grid cards" markdown>

- ![Intech Sol Horizon Rover](/images/intech_sol_horizon_cellular/20220810_154222.jpg){data-gallery="rig"}
- ![RV from the Inside](/images/intech_sol_horizon_cellular/20220811_075849.jpg){data-gallery="rig"}
- ![RV from the side](/images/intech_sol_horizon_cellular/IMG_1386.jpeg){data-gallery="rig"}
- ![RV from the side 2](/images/intech_sol_horizon_cellular/IMG_1769.jpeg){data-gallery="rig"}

</div>


Categories of Cellular Installations
-------------------------------------

After a lot of researching online, I found that there are two main categories of cellular internet installations:

### Mobile Hotspots

Mobile hotspots work by receiving a cellular signal and exposing a WiFi access point inside your trailer. Most modern cellphones have a hotspot feature, but you can also buy dedicated hotspot modems like the Verizon Jetpack or the AT&T Nighthawk. 

#### Pros

- Easy to set up. You purchase a pre-packaged plan from the provider and they ship you a device that will work out of the box
- Easy to use. There is minimal wiring that needs to be setup. Usually you'll only need to plug the device into an AC outlet and any external antennas that may be provided.
- Portable. You don't have to confine the device to just your RV. You can take it anywhere you want.
- Heavy duty cellular boosters like the [Weboost Destination RV](https://www.weboost.com/products/destination-rv) are very easy to setup and can greatly increase the range of the booster with minimal effort.

#### Cons

- They are not meant for long-term heavy use. There have been reports of the devices overheating due to excessive use if you are full-timing.
- Poor cellular reception. In areas of low-quality cell service, the internal antennas will often be insufficient. Most of the window-mounted antennas are also inadequate. Usage of an external cellular booster like the Weboost Destination RV can help mitigate this.
- You're often tied to one carrier. Some devices can only be used with one carrier (not all of them are like this, but most are in my experience).

### Dedicated Cellular Routers

A cellular router is a more robust and permanent platform. A router like the [Peplink MAX Transit Duo](https://mobilemusthave.com/collections/cellular-wifi-routers/products/pepwave-max-transit-pro-dual-modem-cat-12-cat-7-lte-a-router) are powerful devices that are capable of bonding multiple sources of internet into a single WiFi access point. The MAX Transit Duo, for example, is capable of bonding two separate cellular SIM cards and external WiFi signals (like what campgrounds offer) into a single access point. Some of the more advanced Peplink models even allow bonding to a Starlink signal, providing you with quadruple internet redundancy.

#### Pros

- Carrier-agnostic. You won't be tied to any particular carrier and can independently swap out SIM cards any time you want.
- WAN Bonding. You can bond with any kind of internet signal, like from starlink, to provide multiple redundancy paths.
- Redundancy. Because of the WAN bonding, the multi-SIM Peplink devices will automatically and transparently failover to other internet sources. You also get cellular coverage redundancy, meaning if one carrier doesn't have coverage in a particular area, another carrier might. You would not have to configure anything for the failover to happen, the Peplink device would do it automatically.
- Energy efficient. The peplink devices can be wired directly to your 12V system which means you won't need an inverter to convert the battery DC to AC, and then another wall AC adapter to convert the AC back to DC. This DC->AC->DC conversion wastes lots of energy as heat.
- Options for high quality external antenna mounts. The devices can be plugged directly into a permanently mounted external omnidirectional antenna on your roof that provides large dB gains to your signal. You could also theoretically connect the directional antenna from the Weboost Destination RV into a Peplink router for situations where the omnidirectional antenna doesn't provide enough dB gain.

#### Cons

- More expensive. Most of the multi-WAN bonding devices _start_ at around $1000 and quickly increase depending on the options you want. 
- More complex. Careful consideration has to be taken on the physical installation with antenna placement, wire routing, and drilling. The devices themselves also tend to require manual configuration in order to work with the carriers and bands you need. For the Intech Sol Horizon specifically (as I'll elaborate on later), the antenna itself could not be drilled anywhere near the AC unit which means I had to figure out how to use a magnetic mount on a fiberglass roof.

Weboost Destination RV
-----------------------

My goal for my internet setup was ultimate redundancy. I wanted a setup where I could use my phone's internal hotspot for internet, as well as access to a directional antenna that will be powerful enough to punch through areas with incredibly weak signal. The Weboost Destination RV was a great choice because it required minimal setup and works seamlessly with all cellular devices that have their own internal antennas. 

Here is a general diagram of how it works:

![Weboost Destination RV Diagram](/images/intech_sol_horizon_cellular/0001_weboost_diagram.jpeg)

An external antenna is mounted on a telescoping pole that you attach to the side of your RV using 3M tape. The antenna is connected to a signal repeater on the inside of the RV that boosts the signal to a higher dB. The repeater is then connected to a low-gain internal cellular antenna that your devices will interact with.

The Destination RV works great with the Intech Sol Horizon because you can re-use the existing external coaxial port on the left side of the RV for running your cellular signal. This port runs behind the TV over the bed and is meant for routing a satellite antenna to the TV, but works perfectly well for a cellular signal as well.

![external coaxial port on RV](/images/intech_sol_horizon_cellular/IMG_1809.jpeg)

You can then use simple 3M velcro tape to adhere the internal repeater box behind the TV and connect it to the coax port labelled `Satellite`. I strongly recommend using velcro 3M tape instead of permanently mounting with nails because you might find you need to readjust the positioning of your equipment. Connect the repeater to the AC plug using the provided AC adapter.

<!-- insert picture of repeater box -->

Once the repeater has been  mounted and connected, you need to find a place to put the internal antenna. Initially, I decided to place the antenna above the bed as seen here:

<!-- picture of original antenna location -->

But I found this didn't work because the repeater box was flashing green and red which indicated that the internal and external antenna were getting to a runaway feedback loop condition, which caused the repeater to reduce the output power of the external antenna. This is a bit like putting a microphone up to its speaker. The feedback causes runaway noise and can only be fixed by reducing the volume (which is what happens when the repeater indicates a flashing red/green) or by moving the microphone away.

I found that placing the antenna on the floor next to the bed was an easy option that didn't require any major cable routing. The feedback condition was removed and the signal as reported by my phone did indeed improve.

The bundled [weBoost app](https://apps.apple.com/us/app/weboost/id1611974453) is great for guiding you through the setup of pointing the antenna in the right direction and measuring the signal dB as reported by your phone. You can access the internal cellular metrics of your phone by dialing `*3001#12345#*`. 

<div class="grid cards" markdown>

- ![internal phone metric menu](/images/intech_sol_horizon_cellular/IMG_1801.jpg){data-gallery="weboost setup"}
- ![internal phone metric rach attempt](/images/intech_sol_horizon_cellular/IMG_1802.jpg){data-gallery="weboost setup"}

</div>

Peplink Cellular Router
------------------------

The next project I tackled was adding a dedicated cellular router. I chose to use a prepackaged bundle from MobileMustHave.com. MobileMustHave is a reseller of cellular components and offers videos, articles, and ticket support to help you install one of their pre-packaged solutions.

### To 5G or not to 5G?

5G is a new standard that operates in both sub-6GHz (called Frequency Range 1, or FR1) and 24.25 GHz to 71.0 GHz (FR2). FR2 is a microwave band that provides significantly faster download speeds, but in my experience the upload speed gains have been unimpressive. I've found through my own un-scientific tests that upload speeds for 5G in general are lackluster, which is an issue for video conferencing. 

Because of the somewhat untested and experimental nature of 5G, I decided to go with a 4G solution as it provides plenty of bandwidth for my needs.

### Cellular providers

The feature I'm optimising for in my choice of cellular providers is the coverage. The FCC publishes a coverage map for each provider:

https://fcc.maps.arcgis.com/apps/webappviewer/index.html?id=6c1b2e73d9d749cdb7bc88a0d1bdd25b

Here's an image of each coverage map, overlayed with various combinations of providers:

| T-Mobile | Verizon | AT&T | Image |
|----------|---------|------|-------|
| :octicons-check-16: | :octicons-x-12: | :octicons-x-12: | ![T-Mobile only](/images/intech_sol_horizon_cellular/Screenshot 2023-06-24 at 6.03.33 PM.jpeg){data-gallery="coverage-map"} |
| :octicons-x-12: | :octicons-x-12: | :octicons-check-16: | ![AT&T only](/images/intech_sol_horizon_cellular/Screenshot 2023-06-24 at 6.09.56 PM.jpeg){data-gallery="coverage-map"} |
| :octicons-x-12: | :octicons-check-16: | :octicons-x-12: | ![Verizon only](/images/intech_sol_horizon_cellular/Screenshot 2023-06-24 at 6.10.05 PM.jpeg){data-gallery="coverage-map"} |
| :octicons-x-12: | :octicons-check-16: | :octicons-check-16: | ![Verizon and AT&T coverage map](/images/intech_sol_horizon_cellular/Screenshot 2023-06-24 at 6.10.13 PM.jpeg){data-gallery="coverage-map"} |

You can see that T-Mobile by far offers the worst overall coverage: their towers are concentrated along interstates and highly populated regions, but beyond those areas it becomes pretty sparse. Verizon and AT&T offer fairly similar coverage out west, but there are many areas where only one of them provides coverage. 

Using this knowledge, it suggests that the best solution is to acquire data plans for both AT&T and Verizon simultaneously. Using T-Mobile provides no advantage beyond a few select regions in the west.

### MobileMustHave bundles

I ended up choosing the [Ultimate Road Warrior VR2](https://mobilemusthave.com/collections/mobile-internet-bundles-v2/products/ultimate-road-warrior-vr2) bundle. It's a 4G-only solution that allows dual SIM card usage so you can multiplex two carriers at once. MMH also sells their own data plans that come in various flavors. I had to ask them which plan was carried by which provider, as their documentation didn't seem to say.

### MobileMustHave Data Plans

| Plan name | Data cap | Speed Cap | 2023 Cost | Carrier |
|-----------|----------|-----------|-----------|---------|
| P1000 | 1000GB | None | $175 | T-Mobile |
| R300 | 300GB | 25Mbps | $129 | Verizon |
| R300+ | 300GB | 50Mbps | $175 | Verizon |

!!! tip "But I want AT&T :cry:"
    MMH discontinued their AT&T offerings for reasons unknown, but you can still purchase AT&T sim cards from various online vendors. Surprisingly, many folks have had success buying SIM card plans from companies on eBay that have business agreements with AT&T that allow them to distribute SIM cards using their own business data plan. I've been able to find resellers on eBay that offer truly unlimited AT&T plans. 
    
    If you own an LLC, you can also start your own data-only SIM card plan. There may also be ways to get a personal data-only SIM card AT&T plan but I've found it difficult to navigate their website on how to do this.

I ended up purchasing their R300 plan to have Verizon coverage, and got an unlimited AT&T plan from a reputable seller on eBay. I will be updating my experience here when I have more data to share.

### Installation

#### Cable routing plan

The Intech Sol Horizon roof only provides one area where you can drill an electrical hole without wires being exposed on the inside ceiling: the microwave. I previously paid someone to install solar panels and you can see they had the same idea. The junction box that was installed drills into the corner of the microwave cavity and runs down behind the microwave and fridge.

<div class="grid cards" markdown>

- ![Microwave](/images/intech_sol_horizon_cellular/IMG_1810.jpeg){data-gallery="microwave-removal"}
- ![Microwave removed](/images/intech_sol_horizon_cellular/IMG_1813.jpeg){data-gallery="microwave-removal"}
- ![Microwave cavity with cables to roof](/images/intech_sol_horizon_cellular/IMG_1815.jpeg){data-gallery="microwave-removal"}
- ![Roof junction box](/images/intech_sol_horizon_cellular/IMG_1817_with_box.jpeg){data-gallery="microwave-removal"}
</div>

On the outside of the RV, you can pull off the plastic grates that protect the fridge and furnace compartments. I noted the direction that the solar wires were run, which required very minimal drilling. We can use this as inspiration. It comes out the top compartment, runs along the backside of the fridge and runs down into the furnace compartment as shown below.

<div class="grid cards" markdown>

- ![Compartment 1 zoomed out](/images/intech_sol_horizon_cellular/IMG_1818.jpeg){data-gallery="cable-routing-1"}
- ![Compartment 1 zoomed in](/images/intech_sol_horizon_cellular/IMG_1819 traced.jpeg){data-gallery="cable-routing-1"}
- ![Compartment 2 zoomed in](/images/intech_sol_horizon_cellular/IMG_1820 traced.jpeg){data-gallery="cable-routing-1"}
</div>

The cables then run into the battery compartment through a hole drilled into the side of the wall. You can remove the shelf below the fridge by pressing on the green tabs. This reveals the cavity where the cables reside.

<div class="grid cards" markdown>

- ![Shelf upside down](/images/intech_sol_horizon_cellular/IMG_1824.jpeg){data-gallery="cable-routing-2"}
- ![Shelf removed](/images/intech_sol_horizon_cellular/IMG_1821.jpeg){data-gallery="cable-routing-2"}
- ![Cables behind shelf](/images/intech_sol_horizon_cellular/IMG_1823.jpeg){data-gallery="cable-routing-2"}
- ![Battery compartment](/images/intech_sol_horizon_cellular/IMG_1811.jpeg){data-gallery="cable-routing-2"}
</div>

My plan is to then route the antenna cable across the battery compartment, through the front underneath the cushions, and into the righthand storage compartment. The reason for this is to keep the cellular router away from the noisy electronics in the battery compartment that could cause interference and a degredation in signal.

The DC power cable for the router can also be routed in the same way. In fact, I've already crimped the cables onto the battery terminal hubs where all the DC circuits in the RV fan out from.

!!! tip
    Don't mind the toilet paper in the photo. I can promise you I was _not_ in fact taking a dump. :poop:

![Cable routing across front of trailer](/images/intech_sol_horizon_cellular/IMG_1812 traced.jpeg)
