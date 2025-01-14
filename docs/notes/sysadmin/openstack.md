---
icon: simple/openstack
---

[Neutron](https://docs.redhat.com/en/documentation/red_hat_openstack_platform/16.1/html/networking_guide/networking-overview_rhosp-network#networking-overview_rhosp-network)
-------

Neutron is an API gateway that provides a control plane for the underlying SDN solution. It often sits in front of OVN. It is a networking-as-a-service (NaaS) framework.

Its primary role is to act as an API layer for tenants to request and manage networking services. OVN itself will implement and enforce the requested networking logic using OVS. Tenants can interact directly with Neutron.

[OVN](https://docs.redhat.com/en/documentation/red_hat_openstack_platform/16.1/html/networking_guide/networking-overview_rhosp-network#con_open-virtual-network_network-overview)
---

![](https://ubuntu.com/wp-content/uploads/d5c3/image.png)

Open Virtual Network (OVN), is a system to support logical network abstraction in virtual machine and container environments. Sometimes called open source virtual networking for Open vSwitch, OVN complements the existing capabilities of OVS to add native support for logical network abstractions, such as logical L2 and L3 overlays, security groups and services such as DHCP.

A physical network comprises physical wires, switches, and routers. A virtual network extends a physical network into a hypervisor or container platform, bridging VMs or containers into the physical network. An OVN logical network is a network implemented in software that is insulated from physical networks by tunnels or other encapsulations. This allows IP and other address spaces used in logical networks to overlap with those used on physical networks without causing conflicts. Logical network topologies can be arranged without regard for the topologies of the physical networks on which they run. Thus, VMs that are part of a logical network can migrate from one physical machine to another without network disruption.

OVN acts as the control plane. It orchestrates and manages OVS instances.  It ensures the network topology and policies are consistently applied across the infrastructure.

### [ovn-controller](https://docs.redhat.com/en/documentation/red_hat_openstack_platform/16.1/html/networking_guide/assembly_work-with-ovn_rhosp-network#ovn-controller-on-compute-nodes_work-ovn)

The ovn-controller service runs on each Compute node and connects to the OVN southbound (SB) database server to retrieve the logical flows. The ovn-controller translates these logical flows into physical OpenFlow flows and adds the flows to the OVS bridge (br-int). To communicate with ovs-vswitchd and install the OpenFlow flows, the ovn-controller connects to the local ovsdb-server (which hosts conf.db) using the UNIX socket path that was passed when ovn-controller was started (for example unix:/var/run/openvswitch/db.sock).


[OVS](https://docs.redhat.com/en/documentation/red_hat_openstack_platform/16.1/html/networking_guide/networking-overview_rhosp-network#open-vswitch_network-overview)
-----

![](https://hustcat.github.io/assets/ovs/ovs_architecture_01.png)

Open vSwitch (OVS) is a software-defined networking (SDN) virtual switch similar to the Linux software bridge. OVS provides switching services to virtualized networks with support for industry standard OpenFlow and sFlow. OVS can also integrate with physical switches using layer 2 features, such as STP, LACP, and 802.1Q VLAN tagging. Open vSwitch version 1.11.0-1.el6 or later also supports tunneling with VXLAN and GRE.

OVS acts as the data plane. It handles packet forwarding based on flow rules.

Northbound Database
-------------------

The NB database contains high-level configuration and policy information, such as logical network definitions and access policies.

Southbound Database
-------------------

The SB database contains low-level, system-specific configuration and runtime data for individual OVS instances.
