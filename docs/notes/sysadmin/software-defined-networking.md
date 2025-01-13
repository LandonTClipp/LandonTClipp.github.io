---
icon: material/table-network
---

# Software Defined Networking

## NDFC

Cisco's Nexus Dashboard Fabric Controller. Don't use it, it's closed source, has little in the way of tooling and monitoring, and is very mysterious.

## OVN

![](https://hustcat.github.io/assets/ovs/ovs_architecture_01.png)

Open Virtual Networking is an abstraction layer built on top of OVS. It provides logical network components such as virtual networks, routers, and security groups. OVN simplifies the definition and management of network connectivity and policies within virtualized environments. By leveraging switching capabilities of OVS, OVN implements overlay networking to enable advanced features like routing, access control, and distributed load balancing.

OVN is what's called a Cloud Management System (CMS).

## OVS

Open vSwitch provides a virtual switch implementation. It's a software-based switch that allows for the automatic creation and management of virtual ports, bridges, and tunnels.

### Useful Links

- https://blog.vsq.cz/blog/2023-07-11-ovs-datapath-overview/
- https://fairbanks.nl/migrating-to-ovn/