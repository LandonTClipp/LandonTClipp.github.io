---
date: 2025-07-30
authors:
  - LandonTClipp
categories:
  - System Administration
  - Cloud
title: Installing and Managing a Multi-Tenant K8s Cluster
draft: true
---

This post explores my experiences with installing a k8s cluster, managing it, and setting up a multi-tenant containers-as-a-service environment.

<!-- more -->

## Installing k8s

For this MVP setup, we will use a single GPU node. Fortunately I work at a company where getting access to large GPU servers isn't too difficult.

We follow the instructions at https://kubernetes.io/docs/setup/production-environment/tools/kubeadm/install-kubeadm/.

We see the kubelet is crashlooping as expected, as it hasn't received instructions from kubeadm yet:

```
# systemctl status kubelet
● kubelet.service - kubelet: The Kubernetes Node Agent
     Loaded: loaded (/lib/systemd/system/kubelet.service; enabled; vendor preset: enabled)
    Drop-In: /usr/lib/systemd/system/kubelet.service.d
             └─10-kubeadm.conf
     Active: activating (auto-restart) (Result: exit-code) since Wed 2025-07-30 18:21:41 UTC; 1s ago
       Docs: https://kubernetes.io/docs/
    Process: 2249230 ExecStart=/usr/bin/kubelet $KUBELET_KUBECONFIG_ARGS $KUBELET_CONFIG_ARGS $KUBELET_KUBEADM_ARGS $KUBELET_EXTRA_ARGS (code=exited, status=1/FAILURE)
   Main PID: 2249230 (code=exited, status=1/FAILURE)
```


## Configuring the container runtime

My hypervisor already has docker configured. You can see I have a leftover nats cluster:

```
# docker container ls
CONTAINER ID   IMAGE     COMMAND                  CREATED        STATUS        PORTS                                                                    NAMES
3d5c452fa47b   nats      "/nats-server --clus…"   2 months ago   Up 2 months   4222/tcp, 6222/tcp, 8222/tcp                                             docker-nats-2-1
6f5b78b90ff8   nats      "/nats-server --clus…"   2 months ago   Up 2 months   4222/tcp, 6222/tcp, 8222/tcp                                             docker-nats-1-1
ba97e9fa833b   nats      "/nats-server --clus…"   2 months ago   Up 2 months   0.0.0.0:4222->4222/tcp, 0.0.0.0:6222->6222/tcp, 0.0.0.0:8222->8222/tcp   docker-nats-1
```

### Enabling IPv4 packet forwarding

We need to allow the kernel to forward IPv4 packets between interfaces. Technically speaking, this is only needed in multi-node environments, but it's worth mentioning anyway:

```
# sysctl params required by setup, params persist across reboots
cat <<EOF | sudo tee /etc/sysctl.d/k8s.conf
net.ipv4.ip_forward = 1
EOF

# Apply sysctl params without reboot
sudo sysctl --system
```

The setting has been enabled:

```
# sysctl net.ipv4.ip_forward
net.ipv4.ip_forward = 1
```

### Configuring cgroup drivers

https://kubernetes.io/docs/setup/production-environment/container-runtimes/

Control groups are a Linux kernel feature that allow you to segment and isolate many parts of a system's resources. There are two main cgroup drivers k8s can use: the `cgroupfs` driver (enabled by default) and the recommended systemd cgroup driver. The `cgroupfs` driver, as the name suggests, interacts directly with the cgroup filesyste at `/sys/fs/cgroup`:

```
# ls /sys/fs/cgroup
blkio  cpu  cpu,cpuacct  cpuacct  cpuset  devices  freezer  hugetlb  memory  net_cls  net_cls,net_prio  net_prio  perf_event  pids  rdma  systemd  unified
```

My system has both cgroup v1 and v2 installed:

```
# mount | grep cgroup
tmpfs on /sys/fs/cgroup type tmpfs (ro,nosuid,nodev,noexec,mode=755)
cgroup2 on /sys/fs/cgroup/unified type cgroup2 (rw,nosuid,nodev,noexec,relatime,nsdelegate)
cgroup on /sys/fs/cgroup/systemd type cgroup (rw,nosuid,nodev,noexec,relatime,xattr,name=systemd)
cgroup on /sys/fs/cgroup/net_cls,net_prio type cgroup (rw,nosuid,nodev,noexec,relatime,net_cls,net_prio)
cgroup on /sys/fs/cgroup/cpu,cpuacct type cgroup (rw,nosuid,nodev,noexec,relatime,cpu,cpuacct)
cgroup on /sys/fs/cgroup/pids type cgroup (rw,nosuid,nodev,noexec,relatime,pids)
cgroup on /sys/fs/cgroup/perf_event type cgroup (rw,nosuid,nodev,noexec,relatime,perf_event)
cgroup on /sys/fs/cgroup/blkio type cgroup (rw,nosuid,nodev,noexec,relatime,blkio)
cgroup on /sys/fs/cgroup/freezer type cgroup (rw,nosuid,nodev,noexec,relatime,freezer)
cgroup on /sys/fs/cgroup/memory type cgroup (rw,nosuid,nodev,noexec,relatime,memory)
cgroup on /sys/fs/cgroup/cpuset type cgroup (rw,nosuid,nodev,noexec,relatime,cpuset)
cgroup on /sys/fs/cgroup/rdma type cgroup (rw,nosuid,nodev,noexec,relatime,rdma)
cgroup on /sys/fs/cgroup/devices type cgroup (rw,nosuid,nodev,noexec,relatime,devices)
cgroup on /sys/fs/cgroup/hugetlb type cgroup (rw,nosuid,nodev,noexec,relatime,hugetlb)
```

!!! quote 

    The cgroupfs driver is not recommended when systemd is the init system because systemd expects a single cgroup manager on the system. Additionally, if you use cgroup v2, use the systemd cgroup driver instead of cgroupfs.

We use systemd as the init system so we should follow their recommendation to use the systemd cgroup driver. They recommend the following yaml:

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
...
cgroupDriver: systemd
```

We can see it has been configured to load a config file from this location:

```
# systemctl cat kubelet |& grep KUBELET_CONFIG_ARGS
Environment="KUBELET_CONFIG_ARGS=--config=/var/lib/kubelet/config.yaml"
```

We write:

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
address: "10.254.192.244"
port: 20250
serializeImagePulls: false
cgroupDriver: systemd
evictionHard:
    memory.available:  "100Mi"
    nodefs.available:  "10%"
    nodefs.inodesFree: "5%"
    imagefs.available: "15%"
    imagefs.inodesFree: "5%"
```

To this file. The IP address used is the IP of the default interface as defined by `ip r s`.

## Create the k8s cluster

```
# kubeadm init
[init] Using Kubernetes version: v1.33.3
[preflight] Running pre-flight checks
W0730 18:44:39.635425 2257977 checks.go:1065] [preflight] WARNING: Couldn't create the interface used for talking to the container runtime: failed to create new CRI runtime service: validate service connection: validate CRI v1 runtime API for endpoint "unix:///var/run/containerd/containerd.sock": rpc error: code = Unimplemented desc = unknown service runtime.v1.RuntimeService
        [WARNING SystemVerification]: cgroups v1 support is in maintenance mode, please migrate to cgroups v2
[preflight] Pulling images required for setting up a Kubernetes cluster
[preflight] This might take a minute or two, depending on the speed of your internet connection
[preflight] You can also perform this action beforehand using 'kubeadm config images pull'
error execution phase preflight: [preflight] Some fatal errors occurred:
failed to create new CRI runtime service: validate service connection: validate CRI v1 runtime API for endpoint "unix:///var/run/containerd/containerd.sock": rpc error: code = Unimplemented desc = unknown service runtime.v1.RuntimeService[preflight] If you know what you are doing, you can make a check non-fatal with `--ignore-preflight-errors=...`
To see the stack trace of this error execute with --v=5 or higher
```

### What is containerd?

https://www.docker.com/blog/containerd-vs-docker/

![](https://www.docker.com/app/uploads/2024/03/containerd-diagram-v1.png)

containerd is the container runtime environment. It interacts directly with your host operating system. Docker is an abstraction layer that sits on top of containerd and it provides additional concepts like volumes, image management (i.e. downloading and caching images from dockerhub), networking etc.

### containerd CRI Plugin

It appears the container runtime is not configured correctly. We look at the containerd CLI enabled plugins:

```
# ctr plugins ls
TYPE                                   ID                       PLATFORMS      STATUS    
io.containerd.snapshotter.v1           aufs                     linux/amd64    skip      
io.containerd.event.v1                 exchange                 -              ok        
io.containerd.internal.v1              opt                      -              ok        
io.containerd.warning.v1               deprecations             -              ok        
io.containerd.snapshotter.v1           blockfile                linux/amd64    skip      
io.containerd.snapshotter.v1           btrfs                    linux/amd64    skip      
...
```

If we look at the containerd config, we see something interesting:

```
# cat !$
cat /etc/containerd/config.toml
#   Copyright 2018-2022 Docker Inc.

#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at

#       http://www.apache.org/licenses/LICENSE-2.0

#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

disabled_plugins = ["cri"]
```

The CRI plugin is the Containers Runtime Interface, a gRPC API that kubernetes uses to talk to containerd. It seems obvious that we should comment out this line and restart containerd.

```
# systemctl restart containerd
# systemctl status containerd
● containerd.service - containerd container runtime
     Loaded: loaded (/lib/systemd/system/containerd.service; enabled; vendor preset: enabled)
     Active: active (running) since Wed 2025-07-30 19:01:25 UTC; 1s ago
```

We try kubeadm init again:

```
# kubeadm init --config ./kubeadm.yaml 
W0730 19:01:56.158637 2266197 common.go:101] your configuration file uses a deprecated API spec: "kubeadm.k8s.io/v1beta3" (kind: "ClusterConfiguration"). Please use 'kubeadm config migrate --old-config old-config-file --new-config new-config-file', which will write the new, similar spec using a newer API version.
W0730 19:01:56.158889 2266197 initconfiguration.go:333] error unmarshaling configuration schema.GroupVersionKind{Group:"kubeadm.k8s.io", Version:"v1beta3", Kind:"ClusterConfiguration"}: strict decoding error: unknown field "localAPIEndpoint"
[init] Using Kubernetes version: v1.32.0
[preflight] Running pre-flight checks
        [WARNING SystemVerification]: cgroups v1 support is in maintenance mode, please migrate to cgroups v2
error execution phase preflight: [preflight] Some fatal errors occurred:
        [ERROR KubeletVersion]: the kubelet version is higher than the control plane version. This is not a supported version skew and may lead to a malfunctional cluster. Kubelet version: "1.33.3" Control plane version: "1.32.0"
[preflight] If you know what you are doing, you can make a check non-fatal with `--ignore-preflight-errors=...`
To see the stack trace of this error execute with --v=5 or higher
```

### Config Schema Issue

We don't get the gRPC error this time. Great! But let's fix the config schema.

```diff
# diff kubeadm.yaml kubeadm-new.yaml 
1,4c1,30
< apiVersion: kubeadm.k8s.io/v1beta3
< kind: ClusterConfiguration
< kubernetesVersion: v1.32.0
< controlPlaneEndpoint: "10.254.192.244:6443"
---
> apiVersion: kubeadm.k8s.io/v1beta4
> bootstrapTokens:
> - groups:
>   - system:bootstrappers:kubeadm:default-node-token
>   token: hw4taj.tacc97bn9dm2rdhz
>   ttl: 24h0m0s
>   usages:
>   - signing
>   - authentication
> kind: InitConfiguration
> localAPIEndpoint:
>   advertiseAddress: 10.254.192.244
>   bindPort: 6443
> nodeRegistration:
>   criSocket: unix:///var/run/containerd/containerd.sock
>   imagePullPolicy: IfNotPresent
>   imagePullSerial: true
>   name: inst-5c3dw-san-jose-dev-a10-hypervisors-pool
>   taints:
>   - effect: NoSchedule
>     key: node-role.kubernetes.io/control-plane
> timeouts:
>   controlPlaneComponentHealthCheck: 4m0s
>   discovery: 5m0s
>   etcdAPICall: 2m0s
>   kubeletHealthCheck: 4m0s
>   kubernetesAPICall: 1m0s
>   tlsBootstrap: 5m0s
>   upgradeManifests: 5m0s
> ---
7c33,48
<     - "10.254.192.244"
---
>   - 10.254.192.244
> apiVersion: kubeadm.k8s.io/v1beta4
> caCertificateValidityPeriod: 87600h0m0s
> certificateValidityPeriod: 8760h0m0s
> certificatesDir: /etc/kubernetes/pki
> clusterName: kubernetes
> controlPlaneEndpoint: 10.254.192.244:6443
> controllerManager: {}
> dns: {}
> encryptionAlgorithm: RSA-2048
> etcd:
>   local:
>     dataDir: /var/lib/etcd
> imageRepository: registry.k8s.io
> kind: ClusterConfiguration
> kubernetesVersion: v1.32.0
9,10c50,54
<   podSubnet: "10.254.192.244/24"
<   serviceSubnet: "10.254.192.244/24"
---
>   dnsDomain: cluster.local
>   podSubnet: 10.254.192.244/24
>   serviceSubnet: 10.254.192.244/24
> proxy: {}
> scheduler: {}
# mv kubeadm-new.yaml kubeadm.yaml 
```

There are a few more bugs to work out:

```
# kubeadm init --config ./kubeadm.yaml 
[init] Using Kubernetes version: v1.32.0
[preflight] Running pre-flight checks
        [WARNING SystemVerification]: cgroups v1 support is in maintenance mode, please migrate to cgroups v2
error execution phase preflight: [preflight] Some fatal errors occurred:
        [ERROR KubeletVersion]: the kubelet version is higher than the control plane version. This is not a supported version skew and may lead to a malfunctional cluster. Kubelet version: "1.33.3" Control plane version: "1.32.0"
[preflight] If you know what you are doing, you can make a check non-fatal with `--ignore-preflight-errors=...`
To see the stack trace of this error execute with --v=5 or higher
```

### cgroupv2 issue

The kernel is indeed using cgroups v2:

```
# mount | grep cgroup2
cgroup2 on /sys/fs/cgroup/unified type cgroup2 (rw,nosuid,nodev,noexec,relatime,nsdelegate)
```

I suspect that kubeadm is looking at the old cgroups v1 directory at `/sys/fs/cgroup`, not the `unified` subpath. After a minor bit of searching, it appears we need to reconfigure the kernel to only expose the cgroup v2 interface. Kubeadm/kubelet don't appear to have a way to override the path they use when finding the cgroupfs. Enabling cgroup v2 is a kernel commandline parameter, so we must do this:

```text title="/etc/default/grub"
GRUB_CMDLINE_LINUX="systemd.unified_cgroup_hierarchy=1 cgroup_no_v1=all"
```

Then update grub:

```
# update-grub
Sourcing file `/etc/default/grub'
Sourcing file `/etc/default/grub.d/50-cloudimg-settings.cfg'
Sourcing file `/etc/default/grub.d/99-lambda.cfg'
Sourcing file `/etc/default/grub.d/init-select.cfg'
Generating grub configuration file ...
Found linux image: /boot/vmlinuz-5.14.15-custom
Found initrd image: /boot/initrd.img-5.14.15-custom
Adding boot menu entry for UEFI Firmware Settings
done
```

Then reboot the system:

```
# reboot
```

After it comes back, we see that only cgroups v2 is mounted:

```
# mount |& grep cgroup
cgroup2 on /sys/fs/cgroup type cgroup2 (rw,nosuid,nodev,noexec,relatime,nsdelegate)
```

Now that warning is gone:

```
# kubeadm init --config ./kubeadm.yaml 
[init] Using Kubernetes version: v1.32.0
[preflight] Running pre-flight checks
error execution phase preflight: [preflight] Some fatal errors occurred:
        [ERROR KubeletVersion]: the kubelet version is higher than the control plane version. This is not a supported version skew and may lead to a malfunctional cluster. Kubelet version: "1.33.3" Control plane version: "1.32.0"
[preflight] If you know what you are doing, you can make a check non-fatal with `--ignore-preflight-errors=...`
To see the stack trace of this error execute with --v=5 or higher
```

To fix the version error, I just needed to change this line:

```
# grep kubernetesVersion kubeadm.yaml 
kubernetesVersion: v1.33.3
```

We should also first ensure that containerd is configured to use systemd cgroups.

```toml title="/etc/containerd/config.toml"
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
  SystemdCgroup = true
```

We run it again and success!

```
# kubeadm init --config ./kubeadm.yaml                                                              
[init] Using Kubernetes version: v1.33.3
[preflight] Running pre-flight checks
[preflight] Pulling images required for setting up a Kubernetes cluster
[preflight] This might take a minute or two, depending on the speed of your internet connection
[preflight] You can also perform this action beforehand using 'kubeadm config images pull'
W0730 19:36:29.963617   10523 checks.go:846] detected that the sandbox image "registry.k8s.io/pause:3.8" of the container runtime is inconsistent with that used by kubeadm.It is recommended to use "registry.k8s.io/pause:3.10" as the CRI sandbox image.
[certs] Using certificateDir folder "/etc/kubernetes/pki"
[certs] Generating "ca" certificate and key
[certs] Generating "apiserver" certificate and key
[certs] apiserver serving cert is signed for DNS names [inst-5c3dw-san-jose-dev-a10-hypervisors-pool kubernetes kubernetes.default kubernetes.default.svc kubernetes.default.svc.cluster.local] and IPs [10.254.192.1 10.254.192.244]
[certs] Generating "apiserver-kubelet-client" certificate and key
[certs] Generating "front-proxy-ca" certificate and key
[certs] Generating "front-proxy-client" certificate and key
[certs] Generating "etcd/ca" certificate and key
[certs] Generating "etcd/server" certificate and key
[certs] etcd/server serving cert is signed for DNS names [inst-5c3dw-san-jose-dev-a10-hypervisors-pool localhost] and IPs [10.254.192.244 127.0.0.1 ::1]
[certs] Generating "etcd/peer" certificate and key
[certs] etcd/peer serving cert is signed for DNS names [inst-5c3dw-san-jose-dev-a10-hypervisors-pool localhost] and IPs [10.254.192.244 127.0.0.1 ::1]
[certs] Generating "etcd/healthcheck-client" certificate and key
[certs] Generating "apiserver-etcd-client" certificate and key
[certs] Generating "sa" key and public key
[kubeconfig] Using kubeconfig folder "/etc/kubernetes"
[kubeconfig] Writing "admin.conf" kubeconfig file
[kubeconfig] Writing "super-admin.conf" kubeconfig file
[kubeconfig] Writing "kubelet.conf" kubeconfig file
[kubeconfig] Writing "controller-manager.conf" kubeconfig file
[kubeconfig] Writing "scheduler.conf" kubeconfig file
[etcd] Creating static Pod manifest for local etcd in "/etc/kubernetes/manifests"
[control-plane] Using manifest folder "/etc/kubernetes/manifests"
[control-plane] Creating static Pod manifest for "kube-apiserver"
[control-plane] Creating static Pod manifest for "kube-controller-manager"
[control-plane] Creating static Pod manifest for "kube-scheduler"
[kubelet-start] Writing kubelet environment file with flags to file "/var/lib/kubelet/kubeadm-flags.env"
[kubelet-start] Writing kubelet configuration to file "/var/lib/kubelet/config.yaml"
[kubelet-start] Starting the kubelet
[wait-control-plane] Waiting for the kubelet to boot up the control plane as static Pods from directory "/etc/kubernetes/manifests"
[kubelet-check] Waiting for a healthy kubelet at http://127.0.0.1:10248/healthz. This can take up to 4m0s
[kubelet-check] The kubelet is healthy after 501.483944ms
[control-plane-check] Waiting for healthy control plane components. This can take up to 4m0s
[control-plane-check] Checking kube-apiserver at https://10.254.192.244:6443/livez
[control-plane-check] Checking kube-controller-manager at https://127.0.0.1:10257/healthz
[control-plane-check] Checking kube-scheduler at https://127.0.0.1:10259/livez
[control-plane-check] kube-controller-manager is healthy after 1.503219398s
[control-plane-check] kube-scheduler is healthy after 2.258506374s
[control-plane-check] kube-apiserver is healthy after 3.501755041s
[upload-config] Storing the configuration used in ConfigMap "kubeadm-config" in the "kube-system" Namespace
[kubelet] Creating a ConfigMap "kubelet-config" in namespace kube-system with the configuration for the kubelets in the cluster
[upload-certs] Skipping phase. Please see --upload-certs
[mark-control-plane] Marking the node inst-5c3dw-san-jose-dev-a10-hypervisors-pool as control-plane by adding the labels: [node-role.kubernetes.io/control-plane node.kubernetes.io/exclude-from-external-load-balancers]
[mark-control-plane] Marking the node inst-5c3dw-san-jose-dev-a10-hypervisors-pool as control-plane by adding the taints [node-role.kubernetes.io/control-plane:NoSchedule]
[bootstrap-token] Using token: hw4taj.tacc97bn9dm2rdhz
[bootstrap-token] Configuring bootstrap tokens, cluster-info ConfigMap, RBAC Roles
[bootstrap-token] Configured RBAC rules to allow Node Bootstrap tokens to get nodes
[bootstrap-token] Configured RBAC rules to allow Node Bootstrap tokens to post CSRs in order for nodes to get long term certificate credentials
[bootstrap-token] Configured RBAC rules to allow the csrapprover controller automatically approve CSRs from a Node Bootstrap Token
[bootstrap-token] Configured RBAC rules to allow certificate rotation for all node client certificates in the cluster
[bootstrap-token] Creating the "cluster-info" ConfigMap in the "kube-public" namespace
[kubelet-finalize] Updating "/etc/kubernetes/kubelet.conf" to point to a rotatable kubelet client certificate and key
[addons] Applied essential addon: CoreDNS
[addons] Applied essential addon: kube-proxy

Your Kubernetes control-plane has initialized successfully!

To start using your cluster, you need to run the following as a regular user:

  mkdir -p $HOME/.kube
  sudo cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
  sudo chown $(id -u):$(id -g) $HOME/.kube/config

Alternatively, if you are the root user, you can run:

  export KUBECONFIG=/etc/kubernetes/admin.conf

You should now deploy a pod network to the cluster.
Run "kubectl apply -f [podnetwork].yaml" with one of the options listed at:
  https://kubernetes.io/docs/concepts/cluster-administration/addons/

You can now join any number of control-plane nodes by copying certificate authorities
and service account keys on each node and then running the following as root:

  kubeadm join 10.254.192.244:6443 --token hw4taj.tacc97bn9dm2rdhz \
        --discovery-token-ca-cert-hash sha256:3835bb0ac0f16745a279e91eb812a0e5dfb5fc93d7565e714821c2768bded915 \
        --control-plane 

Then you can join any number of worker nodes by running the following on each as root:

kubeadm join 10.254.192.244:6443 --token hw4taj.tacc97bn9dm2rdhz \
        --discovery-token-ca-cert-hash sha256:3835bb0ac0f16745a279e91eb812a0e5dfb5fc93d7565e714821c2768bded915 
```

### Check kubelet.service

It appears it's not able to contact the API server (which is supposed to run on the same node):

```
E0730 21:01:10.126250   11574 kubelet_node_status.go:548] "Error updating node status, will retry" err="error getting node \"inst-5c3dw-san-jose-dev-a10-hypervisors-pool\": Get \"https://10.254.192.244:6443/api/v1/nodes/inst-5c3dw-san-jose-dev-a10-hypervisors-pool?timeout=10s\": dial tcp 10.254.192.244:6443: connect: conn>
```

We can use a tool called `crictl` that is a runtime-agnostic replacement for docker ps, docker logs etc, but designed specifically for Kubernetes.(1)
{ .annotate }

1. Why does it exist? Kubernetes doesn’t require Docker anymore — it talks to the container runtime via a standardized API called CRI. Different runtimes implement this (e.g., containerd, CRI-O, Mirantis cri-dockerd). crictl provides a consistent way to inspect and debug containers and pods regardless of which runtime is in use.

What is the apiserver doing?

```
# crictl ps -a |& grep apiserver
fd00ab5bd2e19       a92b4b92a9916       2 minutes ago        Exited              kube-apiserver            19                  175c92b5831d6       kube-apiserver-inst-5c3dw-san-jose-dev-a10-hypervisors-pool            kube-system
```

It's not running, we check the logs:

```
# crictl logs fd00ab5bd2e19
WARN[0000] Config "/etc/crictl.yaml" does not exist, trying next: "/usr/bin/crictl.yaml" 
WARN[0000] runtime connect using default endpoints: [unix:///run/containerd/containerd.sock unix:///run/crio/crio.sock unix:///var/run/cri-dockerd.sock]. As the default settings are now deprecated, you should set the endpoint instead. 
I0730 21:01:10.607488       1 options.go:249] external host was not specified, using 10.254.192.244
I0730 21:01:10.609025       1 server.go:147] Version: v1.33.3
I0730 21:01:10.609042       1 server.go:149] "Golang settings" GOGC="" GOMAXPROCS="" GOTRACEBACK=""
W0730 21:01:10.999479       1 logging.go:55] [core] [Channel #2 SubChannel #3]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
W0730 21:01:10.999566       1 logging.go:55] [core] [Channel #1 SubChannel #4]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
I0730 21:01:11.000358       1 shared_informer.go:350] "Waiting for caches to sync" controller="node_authorizer"
I0730 21:01:11.006018       1 shared_informer.go:350] "Waiting for caches to sync" controller="*generic.policySource[*k8s.io/api/admissionregistration/v1.ValidatingAdmissionPolicy,*k8s.io/api/admissionregistration/v1.ValidatingAdmissionPolicyBinding,k8s.io/apiserver/pkg/admission/plugin/policy/validating.Validator]"
I0730 21:01:11.011647       1 plugins.go:157] Loaded 14 mutating admission controller(s) successfully in the following order: NamespaceLifecycle,LimitRanger,ServiceAccount,NodeRestriction,TaintNodesByCondition,Priority,DefaultTolerationSeconds,DefaultStorageClass,StorageObjectInUseProtection,RuntimeClass,DefaultIngressClass,PodTopologyLabels,MutatingAdmissionPolicy,MutatingAdmissionWebhook.
I0730 21:01:11.011668       1 plugins.go:160] Loaded 13 validating admission controller(s) successfully in the following order: LimitRanger,ServiceAccount,PodSecurity,Priority,PersistentVolumeClaimResize,RuntimeClass,CertificateApproval,CertificateSigning,ClusterTrustBundleAttest,CertificateSubjectRestriction,ValidatingAdmissionPolicy,ValidatingAdmissionWebhook,ResourceQuota.
I0730 21:01:11.011770       1 instance.go:233] Using reconciler: lease
W0730 21:01:11.012277       1 logging.go:55] [core] [Channel #5 SubChannel #6]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
W0730 21:01:12.000165       1 logging.go:55] [core] [Channel #2 SubChannel #3]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
W0730 21:01:12.000275       1 logging.go:55] [core] [Channel #1 SubChannel #4]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
W0730 21:01:12.013075       1 logging.go:55] [core] [Channel #5 SubChannel #6]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
W0730 21:01:13.490656       1 logging.go:55] [core] [Channel #2 SubChannel #3]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
W0730 21:01:13.621854       1 logging.go:55] [core] [Channel #5 SubChannel #6]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
W0730 21:01:13.827507       1 logging.go:55] [core] [Channel #1 SubChannel #4]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
W0730 21:01:15.879253       1 logging.go:55] [core] [Channel #2 SubChannel #3]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
W0730 21:01:16.391114       1 logging.go:55] [core] [Channel #1 SubChannel #4]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
W0730 21:01:16.629331       1 logging.go:55] [core] [Channel #5 SubChannel #6]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
W0730 21:01:19.947000       1 logging.go:55] [core] [Channel #2 SubChannel #3]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
W0730 21:01:20.334115       1 logging.go:55] [core] [Channel #1 SubChannel #4]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
W0730 21:01:20.969026       1 logging.go:55] [core] [Channel #5 SubChannel #6]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
W0730 21:01:26.938075       1 logging.go:55] [core] [Channel #5 SubChannel #6]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
W0730 21:01:27.059469       1 logging.go:55] [core] [Channel #1 SubChannel #4]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
W0730 21:01:27.645963       1 logging.go:55] [core] [Channel #2 SubChannel #3]grpc: addrConn.createTransport failed to connect to {Addr: "127.0.0.1:2379", ServerName: "127.0.0.1:2379", }. Err: connection error: desc = "transport: Error while dialing: dial tcp 127.0.0.1:2379: connect: connection refused"
F0730 21:01:31.013150       1 instance.go:226] Error creating leases: error creating storage factory: context deadline exceeded
```

Port 2379 is the client API port for etcd, so let's see what's wrong there.

```
# crictl ps -a |& grep etcd
8d6f6ec0c1da2       499038711c081       3 minutes ago        Exited              etcd                      16                  c9a80c0d59849       etcd-inst-5c3dw-san-jose-dev-a10-hypervisors-pool                      kube-system
# crictl logs [...]
{"level":"info","ts":"2025-07-30T21:02:07.516400Z","caller":"osutil/interrupt_unix.go:64","msg":"received signal; shutting down","signal":"terminated"}
```

#### Incorrect containerd cgroup v2 configuration

It was shutdown by something. On further investigation, I found [this Github issue](https://github.com/etcd-io/etcd/issues/13670) that suggests the containerd runtime is not using cgroups driver approrpiately. We change our config file:

```toml title="/etc/containerd/config.toml"
version = 2
[plugins]
  [plugins."io.containerd.grpc.v1.cri"]
   [plugins."io.containerd.grpc.v1.cri".containerd]
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes]
        [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
          runtime_type = "io.containerd.runc.v2"
          [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
            SystemdCgroup = true
```

We restart `containerd.service` and `kubelet.service`. Things seem to be running now!

```
# crictl ps -a
WARN[0000] Config "/etc/crictl.yaml" does not exist, trying next: "/usr/bin/crictl.yaml" 
WARN[0000] runtime connect using default endpoints: [unix:///run/containerd/containerd.sock unix:///run/crio/crio.sock unix:///var/run/cri-dockerd.sock]. As the default settings are now deprecated, you should set the endpoint instead. 
WARN[0000] Image connect using default endpoints: [unix:///run/containerd/containerd.sock unix:///run/crio/crio.sock unix:///var/run/cri-dockerd.sock]. As the default settings are now deprecated, you should set the endpoint instead. 
CONTAINER           IMAGE               CREATED             STATE               NAME                      ATTEMPT             POD ID              POD                                                                    NAMESPACE
fc06e383ddd14       af855adae7960       14 seconds ago      Running             kube-proxy                23                  33c1780f1d81a       kube-proxy-sn66m                                                       kube-system
2511a21164725       a92b4b92a9916       17 seconds ago      Running             kube-apiserver            25                  0fa0394d6af4e       kube-apiserver-inst-5c3dw-san-jose-dev-a10-hypervisors-pool            kube-system
238b3d008fe83       499038711c081       17 seconds ago      Running             etcd                      10                  d9b0fbe4ff979       etcd-inst-5c3dw-san-jose-dev-a10-hypervisors-pool                      kube-system
6579f945aa549       bf97fadcef430       17 seconds ago      Running             kube-controller-manager   28                  671c5bd069518       kube-controller-manager-inst-5c3dw-san-jose-dev-a10-hypervisors-pool   kube-system
558e9c0399021       41376797d5122       17 seconds ago      Running             kube-scheduler            25                  a312b6a729349       kube-scheduler-inst-5c3dw-san-jose-dev-a10-hypervisors-pool            kube-system
```

We can check that kubectl indeed sees the system pods:

```
# kubectl get pods -n kube-system
NAME                                                                   READY   STATUS    RESTARTS         AGE
coredns-674b8bbfcf-k5jwk                                               0/1     Pending   0                117m
coredns-674b8bbfcf-v29ns                                               0/1     Pending   0                117m
etcd-inst-5c3dw-san-jose-dev-a10-hypervisors-pool                      1/1     Running   10 (2m45s ago)   2m33s
kube-apiserver-inst-5c3dw-san-jose-dev-a10-hypervisors-pool            1/1     Running   25 (6m25s ago)   117m
kube-controller-manager-inst-5c3dw-san-jose-dev-a10-hypervisors-pool   1/1     Running   28 (4m33s ago)   117m
kube-proxy-sn66m                                                       1/1     Running   23 (3m14s ago)   117m
kube-scheduler-inst-5c3dw-san-jose-dev-a10-hypervisors-pool            1/1     Running   25 (2m44s ago)   117m
```

However, coredns is not running. Why?

```
  Warning  FailedScheduling  120m                 default-scheduler  0/1 nodes are available: 1 node(s) had untolerated taint {node.kubernetes.io/not-ready: }. preemption: 0/1 nodes are available: 1 Preemption is not helpful for scheduling.
```

This is telling us that the node itself is not ready. We can confirm that by looking at the node:

```
# kubectl get nodes -o wide
NAME                                           STATUS     ROLES           AGE    VERSION   INTERNAL-IP      EXTERNAL-IP   OS-IMAGE             KERNEL-VERSION   CONTAINER-RUNTIME
inst-5c3dw-san-jose-dev-a10-hypervisors-pool   NotReady   control-plane   125m   v1.33.3   10.254.192.244   <none>        Ubuntu 20.04.3 LTS   5.14.15-custom   containerd://1.7.20
```

The kubelet says this:

```
E0730 21:44:40.048656   69503 kubelet.go:3117] "Container runtime network not ready" networkReady="NetworkReady=false reason:NetworkPluginNotReady message:Network plugin returns error: cni plugin not initialized"
```

We haven't initialized a [network plugin](https://kubernetes.io/docs/concepts/extend-kubernetes/compute-storage-net/network-plugins/). For that, we'll use Cilium.

### Create the CNI Network Plugin

In a multi-tenancy environment, the question becomes: how do you isolate each tenancy from each other? For the network, there are two general paths we could take:

1. Use a fully-blown fabric-level SDN like OVN. This is useful if you want each tenancy to have full VRF capabilities, for example in clouds that provide VPNs. Such a system would allow customers to not only have their own isolated tenancy, but it would allow them to customize their VPN so that multiple resources can live inside of the same network overlay. OVN is incredibly complex, howevever, and often requires expert network engineers to deploy and manage.
2. Use an eBPF tool that enforces network policies on the host level. The traffic traversing such a system would rely upon a network fabric that does not by itself separate tenancies into separate VRFs and VLANs, but strong tenancy isolation can still be achieved through end-to-end traffic encryption between the pods in a k8s network. This is generally a simpler approach to take because it does not require an SDN that can manage VRFs on switches. You do not need to manage VLANs, it does not have infrastructural dependencies on switches, it has built-in encryption and observability, and such eBPF tools like the k8s-native Cilium project are by definition k8s-native and can be managed by the k8s control plane as first-class citizens.

We'll use [Cilium](https://cilium.io/get-started/) for this.

!!! quote "What is Cilium?"

    Cilium is an open source project to provide networking, security, and observability for cloud native environments such as Kubernetes clusters and other container orchestration platforms.

    At the foundation of Cilium is a new Linux kernel technology called eBPF, which enables the dynamic insertion of powerful security, visibility, and networking control logic into the Linux kernel. eBPF is used to provide high-performance networking, multi-cluster and multi-cloud capabilities, advanced load balancing, transparent encryption, extensive network security capabilities, transparent observability, and much more.

#### Install the Cilium CLI

```
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# CILIUM_CLI_VERSION=$(curl -s https://raw.githubusercontent.com/cilium/cilium-cli/main/stable.txt)
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# CLI_ARCH=amd64
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# if [ "$(uname -m)" = "aarch64" ]; then CLI_ARCH=arm64; fi
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# curl -L --fail --remote-name-all https://github.com/cilium/cilium-cli/releases/download/${CILIUM_CLI_VERSION}/cilium-linux-${CLI_ARCH}.tar.gz{,.sha256sum}
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
  0     0    0     0    0     0      0      0 --:--:-- --:--:-- --:--:--     0
100 56.6M  100 56.6M    0     0  43.5M      0  0:00:01  0:00:01 --:--:-- 54.9M
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
  0     0    0     0    0     0      0      0 --:--:-- --:--:-- --:--:--     0
100    92  100    92    0     0    412      0 --:--:-- --:--:-- --:--:--   412
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# sha256sum --check cilium-linux-${CLI_ARCH}.tar.gz.sha256sum
cilium-linux-amd64.tar.gz: OK
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# sudo tar xzvfC cilium-linux-${CLI_ARCH}.tar.gz /usr/local/bin
cilium
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# rm cilium-linux-${CLI_ARCH}.tar.gz{,.sha256sum}
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# 
```

#### Install Cilium

https://docs.cilium.io/en/stable/gettingstarted/k8s-install-default/#create-the-cluster

```
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# kubectl config get-contexts
CURRENT   NAME                          CLUSTER      AUTHINFO           NAMESPACE
*         kubernetes-admin@kubernetes   kubernetes   kubernetes-admin   
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# cilium install --version 1.18.0
ℹ️   Using Cilium version 1.18.0
🔮 Auto-detected cluster name: kubernetes
🔮 Auto-detected kube-proxy has been installed
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# cilium status
    /¯¯\
 /¯¯\__/¯¯\    Cilium:             OK
 \__/¯¯\__/    Operator:           OK
 /¯¯\__/¯¯\    Envoy DaemonSet:    OK
 \__/¯¯\__/    Hubble Relay:       disabled
    \__/       ClusterMesh:        disabled

DaemonSet              cilium                   Desired: 1, Ready: 1/1, Available: 1/1
DaemonSet              cilium-envoy             Desired: 1, Ready: 1/1, Available: 1/1
Deployment             cilium-operator          Desired: 1, Ready: 1/1, Available: 1/1
Containers:            cilium                   Running: 1
                       cilium-envoy             Running: 1
                       cilium-operator          Running: 1
                       clustermesh-apiserver    
                       hubble-relay             
Cluster Pods:          2/2 managed by Cilium
Helm chart version:    1.18.0
Image versions         cilium             quay.io/cilium/cilium:v1.18.0@sha256:dfea023972d06ec183cfa3c9e7809716f85daaff042e573ef366e9ec6a0c0ab2: 1
                       cilium-envoy       quay.io/cilium/cilium-envoy:v1.34.4-1753677767-266d5a01d1d55bd1d60148f991b98dac0390d363@sha256:231b5bd9682dfc648ae97f33dcdc5225c5a526194dda08124f5eded833bf02bf: 1
                       cilium-operator    quay.io/cilium/operator-generic:v1.18.0@sha256:398378b4507b6e9db22be2f4455d8f8e509b189470061b0f813f0fabaf944f51: 1
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# 
```

We can run the provided Cilium connectivity test:

```
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# cilium connectivity test
ℹ️   Monitor aggregation detected, will skip some flow validation steps                                                                                                                                             ✨ [kubernetes] Creating namespace cilium-test-1 for connectivity check...
✨ [kubernetes] Deploying echo-same-node service...                                                       
✨ [kubernetes] Deploying DNS test server configmap...
✨ [kubernetes] Deploying same-node deployment...                                                         
✨ [kubernetes] Deploying client deployment...
✨ [kubernetes] Deploying client2 deployment...                                                           
⌛ [kubernetes] Waiting for deployment cilium-test-1/client to become ready...
⌛ [kubernetes] Waiting for deployment cilium-test-1/client2 to become ready...
⌛ [kubernetes] Waiting for deployment cilium-test-1/echo-same-node to become ready...
⌛ [kubernetes] Waiting for pod cilium-test-1/client-645b68dcf7-xjr6c to reach DNS server on cilium-test-1/echo-same-node-5f44c8d48c-k5rqm pod...
⌛ [kubernetes] Waiting for pod cilium-test-1/client2-66475877c6-gj74x to reach DNS server on cilium-test-1/echo-same-node-5f44c8d48c-k5rqm pod...
⌛ [kubernetes] Waiting for pod cilium-test-1/client-645b68dcf7-xjr6c to reach default/kubernetes service...
⌛ [kubernetes] Waiting for pod cilium-test-1/client2-66475877c6-gj74x to reach default/kubernetes service...
⌛ [kubernetes] Waiting for Service cilium-test-1/echo-same-node to become ready...
⌛ [kubernetes] Waiting for Service cilium-test-1/echo-same-node to be synchronized by Cilium pod kube-system/cilium-mt9xp
⌛ [kubernetes] Waiting for NodePort 10.254.192.244:32475 (cilium-test-1/echo-same-node) to become ready...
⌛ [kubernetes] Waiting for NodePort 10.254.192.244:32475 (cilium-test-1/echo-same-node) to become ready...
[...]
[=] [cilium-test-1] Test [check-log-errors] [123/123]
.....
  [.] Action [check-log-errors/no-errors-in-logs:kubernetes/kube-system/cilium-mt9xp (config)]
  [-] Scenario [check-log-errors/no-errors-in-logs]
  [.] Action [check-log-errors/no-errors-in-logs:kubernetes/kube-system/cilium-envoy-jvbms (cilium-envoy)]
  [.] Action [check-log-errors/no-errors-in-logs:kubernetes/kube-system/cilium-mt9xp (mount-bpf-fs)]
  [.] Action [check-log-errors/no-errors-in-logs:kubernetes/kube-system/cilium-mt9xp (clean-cilium-state)]
  [.] Action [check-log-errors/no-errors-in-logs:kubernetes/kube-system/cilium-mt9xp (install-cni-binaries)]
  [.] Action [check-log-errors/no-errors-in-logs:kubernetes/kube-system/cilium-mt9xp (cilium-agent)]
  ❌ Found 1 logs in kubernetes/kube-system/cilium-mt9xp (cilium-agent) matching list of errors that must be investigated:
time=2025-07-31T18:11:30.753850455Z level=error msg="cannot read proc file" module=agent.controlplane.l7-proxy file-path=/proc/net/udp6 error="open /proc/net/udp6: no such file or directory" (8 occurrences)
.  [.] Action [check-log-errors/no-errors-in-logs:kubernetes/kube-system/cilium-mt9xp (mount-cgroup)]
.  [.] Action [check-log-errors/no-errors-in-logs:kubernetes/kube-system/cilium-mt9xp (apply-sysctl-overwrites)]
.  [.] Action [check-log-errors/no-errors-in-logs:kubernetes/kube-system/cilium-operator-64ddb69dfd-ndzbc (cilium-operator)]
.
📋 Test Report [cilium-test-1]
❌ 1/74 tests failed (1/293 actions), 49 tests skipped, 1 scenarios skipped:
Test [check-log-errors]:
  🟥 check-log-errors/no-errors-in-logs:kubernetes/kube-system/cilium-mt9xp (cilium-agent): Found 1 logs in kubernetes/kube-system/cilium-mt9xp (cilium-agent) matching list of errors that must be investigated:
time=2025-07-31T18:11:30.753850455Z level=error msg="cannot read proc file" module=agent.controlplane.l7-proxy file-path=/proc/net/udp6 error="open /proc/net/udp6: no such file or directory" (8 occurrences)
[cilium-test-1] 1 tests failed
```

Most of the tests passed except for one. The log message indicates that cilium was attempting to find the IPv6 UDP psuedo-file. Upon investigation, this appears to be a file that the kernel exposes that allows processes to inspect all active IPv6 UDP sockets. For example we can try reading the regular `/proc/net/udp` file:

```
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# cat /proc/net/udp
  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode ref pointer drops             
 3909: 3500007F:0035 00000000:0000 07 00000000:00000000 00:00000000 00000000   101        0 125020 2 ffff93191c9d6300 0        
 3924: F4C0FE0A:0044 00000000:0000 07 00000000:00000000 00:00000000 00000000   100        0 1089789 2 ffff9398b6d4de80 0       
 3967: 00000000:006F 00000000:0000 07 00000000:00000000 00:00000000 00000000     0        0 147480 2 ffff9398d9d78000 0        
 4179: 0100007F:0143 00000000:0000 07 00000000:00000000 00:00000000 00000000     0        0 49261 2 ffff9319799b1200 0         
12328: 00000000:2118 00000000:0000 07 00000000:00000000 00:00000000 00000000     0        0 2139786 2 ffff9398f80f7500 0       
50191: 0100007F:B4FF 00000000:0000 07 00000000:00000000 00:00000000 00000000     0        0 2172164 2 ffff9399caa11b00 0       
51407: 0100007F:B9BF 0100007F:0143 01 00000000:00000000 00:00000000 00000000     0        0 53410 2 ffff9319d6a70000 0     
```

When we look at the kernel's dmesg logs, we see it explicitly states that ipv6 has been administratively disabled:

```
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# dmesg | grep -i ipv6
[    0.000000] Command line: BOOT_IMAGE=/boot/vmlinuz-5.14.15-custom root=UUID=3603dbb9-70be-449b-b3ba-5cad4dc8c67f ro systemd.unified_cgroup_hierarchy=1 cgroup_no_v1=all rd.driver.pre=vfio-pci video=efifb:off vga=normal nofb nomodeset usbcore.nousb hugepagesz=1G default_hugepagesz=1G transparent_hugepage=never ipv6.disable=1 console=tty1 console=ttyS0 intel_iommu=on hugepages=906
[    1.880268] Kernel command line: BOOT_IMAGE=/boot/vmlinuz-5.14.15-custom root=UUID=3603dbb9-70be-449b-b3ba-5cad4dc8c67f ro systemd.unified_cgroup_hierarchy=1 cgroup_no_v1=all rd.driver.pre=vfio-pci video=efifb:off vga=normal nofb nomodeset usbcore.nousb hugepagesz=1G default_hugepagesz=1G transparent_hugepage=never ipv6.disable=1 console=tty1 console=ttyS0 intel_iommu=on hugepages=906
[  147.786693] IPv6: Loaded, but administratively disabled, reboot required to enable
[  167.393552] systemd[1]: Binding to IPv6 address not available since kernel does not support IPv6.
[  167.499789] systemd[1]: Binding to IPv6 address not available since kernel does not support IPv6.
```

We do indeed see it was disabled from the kernel commandline:

```
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# cat /proc/cmdline
BOOT_IMAGE=/boot/vmlinuz-5.14.15-custom root=UUID=3603dbb9-70be-449b-b3ba-5cad4dc8c67f ro systemd.unified_cgroup_hierarchy=1 cgroup_no_v1=all rd.driver.pre=vfio-pci video=efifb:off vga=normal nofb nomodeset usbcore.nousb hugepagesz=1G default_hugepagesz=1G transparent_hugepage=never ipv6.disable=1 console=tty1 console=ttyS0 intel_iommu=on hugepages=906
```

No matter, this is just an experiment so there is no need for IPv6.

We check the node health and see that it's marked as Ready:

```
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# kubectl get nodes -o wide
NAME                                           STATUS   ROLES           AGE   VERSION   INTERNAL-IP      EXTERNAL-IP   OS-IMAGE             KERNEL-VERSION   CONTAINER-RUNTIME
inst-5c3dw-san-jose-dev-a10-hypervisors-pool   Ready    control-plane   22h   v1.33.3   10.254.192.244   <none>        Ubuntu 20.04.3 LTS   5.14.15-custom   containerd://1.7.20
```

We also see that CoreDNS is working, in addition to the newly created cilium pods:

```
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu# kubectl get pods -n kube-system
NAME                                                                   READY   STATUS    RESTARTS       AGE
cilium-envoy-jvbms                                                     1/1     Running   0              17m
cilium-mt9xp                                                           1/1     Running   0              17m
cilium-operator-64ddb69dfd-ndzbc                                       1/1     Running   0              17m
coredns-674b8bbfcf-k5jwk                                               1/1     Running   0              22h
coredns-674b8bbfcf-v29ns                                               1/1     Running   0              22h
etcd-inst-5c3dw-san-jose-dev-a10-hypervisors-pool                      1/1     Running   10 (20h ago)   20h
kube-apiserver-inst-5c3dw-san-jose-dev-a10-hypervisors-pool            1/1     Running   25 (20h ago)   22h
kube-controller-manager-inst-5c3dw-san-jose-dev-a10-hypervisors-pool   1/1     Running   28 (20h ago)   22h
kube-proxy-sn66m                                                       1/1     Running   23 (20h ago)   22h
kube-scheduler-inst-5c3dw-san-jose-dev-a10-hypervisors-pool            1/1     Running   25 (20h ago)   22h
```

That sounds like a success!

## Test a simple ML job

<div class="annotate" markdown>
Our k8s cluster is nowhere near the state where we need it to be, it it should allow us to do some basic ML jobs.(1) Some of the critical pieces we haven't yet implemented are:

1. Virtualized running environments. The containers shouldn't use the host kernel, they should use their own isolated VM.
2. Isolated network overlay via cilium.
3. Observability platform.
4. GPU PCIe passthrough to the VMs.

</div>

1. Actually, I'm expecting the containers to not have any access to the GPU devices as we haven't explicitly configured it to do so, but we'll see what happens anyway.


We create a k8s deployment file for a convolutional neural network that identifies handwritten digits from the MNIST dataset. I've uploaded this to dockerhub at `landontclipp/serverless-test`. We just need to run a deployment of this and see what happens.

```yaml title="deployment.yaml"
apiVersion: apps/v1
kind: Deployment
metadata:
  name: serverless-test-deployment
  namespace: default
spec:
  replicas: 1
  selector:
    matchLabels:
      app: serverless-test
  template:
    metadata:
      labels:
        app: serverless-test
    spec:
      containers:
        - name: serverless-test
          image: landontclipp/serverless-test:v1.0.5
          imagePullPolicy: Always
          command: ["python3", "-u", "mnist_image_classifier.py"]
      restartPolicy: Always
```

And then apply it

```
kubectl apply -f ./deployment.yaml
```

We see k8s is deploying the pod:

```
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu/lclipp/worker-basic/k8s# kubectl get pods
NAME                                          READY   STATUS              RESTARTS   AGE
serverless-test-deployment-6c7657c859-66pn6   0/1     ContainerCreating   0          5s
```

The container eventually fails (not suprisingly):

```
  Warning  BackOff    11s                  kubelet            Back-off restarting failed container serverless-test in pod serverless-test-deployment-6c7657c859-66pn6_default(b50a7fa8-bb90-46f1-aba4-f17a2bb67161)
```

It appears there is some application-level issue:

```
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu/lclipp/worker-basic/k8s# kubectl logs serverless-test-deployment-56bcd9f69-f4crw
font_manager.py     :1639 2025-07-31 21:51:45,306 generated new fontManager
starting data loader
train_loader begin
100%|██████████| 9.91M/9.91M [00:01<00:00, 6.56MB/s]
100%|██████████| 28.9k/28.9k [00:00<00:00, 432kB/s]
100%|██████████| 1.65M/1.65M [00:00<00:00, 3.00MB/s]
100%|██████████| 4.54k/4.54k [00:00<00:00, 7.64MB/s]
train_loader end
test_loader begin
test_loader end
Using device: cpu
//mnist_image_classifier.py:49: UserWarning: Implicit dimension choice for log_softmax has been deprecated. Change the call to include dim=X as an argument.
  return F.log_softmax(x)
/opt/conda/lib/python3.11/site-packages/torch/nn/_reduction.py:51: UserWarning: size_average and reduce args will be deprecated, please use reduction='sum' instead.
  warnings.warn(warning.format(ret))

Test set: Avg. loss: 2.3316, Accuracy: 1137/10000 (11%)

starting training epoch 1
Train Epoch: 1 [0/60000 (0%)]   Loss: 2.375533
Traceback (most recent call last):
  File "//mnist_image_classifier.py", line 175, in <module>
    classifier()
  File "//mnist_image_classifier.py", line 169, in classifier
    train(epoch)
  File "//mnist_image_classifier.py", line 140, in train
    torch.save(network.state_dict(), f"{RESULTS_DIR}/model.pth")
  File "/opt/conda/lib/python3.11/site-packages/torch/serialization.py", line 964, in save
    with _open_zipfile_writer(f) as opened_zipfile:
         ^^^^^^^^^^^^^^^^^^^^^^^
  File "/opt/conda/lib/python3.11/site-packages/torch/serialization.py", line 828, in _open_zipfile_writer
    return container(name_or_buffer)
           ^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/opt/conda/lib/python3.11/site-packages/torch/serialization.py", line 792, in __init__
    torch._C.PyTorchFileWriter(
RuntimeError: Parent directory /runpod-volume/ does not exist.
```

This is my fault actually. This image was originally meant to run in the runpod.io environment. I just need to modify it a little bit to not expect this volume to exist (and I'm not sure it actually needed it anyway).

I make a [minor change](https://github.com/LandonTClipp/worker-basic/commit/b54fc02f3bdb5fed86b1e43cd12eac1ee31d2359) and also change the deployment to be able to write its model output to `/output` which we will create as a k8s persistent volume.

=== "`persistentvolumeclaim.yaml`"

    ```yaml title="persistentvolumeclaim.yaml"
    apiVersion: v1
    kind: PersistentVolumeClaim
    metadata:
    name: model-output
    namespace: default
    spec:
    accessModes:
        - ReadWriteOnce
    resources:
        requests:
        storage: 5Gi
    ```

=== "`deployment.yaml`"

    ```yaml title="deployment.yaml"
    apiVersion: apps/v1
    kind: Deployment
    metadata:
    name: serverless-test-deployment
    namespace: default
    spec:
    replicas: 1
    selector:
        matchLabels:
        app: serverless-test
    template:
        metadata:
        labels:
            app: serverless-test
        spec:
        containers:
            - name: serverless-test
            image: landontclipp/serverless-test:v1.0.6
            imagePullPolicy: Always
            command: ["python3", "-u", "mnist_image_classifier.py"]
            volumeMounts:
                - name: model-output
                mountPath: /output
        restartPolicy: Always
        volumes:
            - name: model-output
            persistentVolumeClaim:
                claimName: model-output
    ```

```
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu/lclipp/worker-basic/k8s# kubectl apply -f deployment.yaml 
deployment.apps/serverless-test-deployment configured
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu/lclipp/worker-basic/k8s# kubectl apply -f persistentvolumeclaim.yaml 
persistentvolumeclaim/model-output created
```

The pods are stuck in pending state because of this error in the persistentstorageclaim:

```
  Normal  FailedBinding  13s (x26 over 6m26s)  persistentvolume-controller  no persistent volumes available for this claim and no storage class is set
```

We didn't specify an actual persistentvolume for this claim, whoops. I make a directory on the host at `/data` and tell k8s about this:

```yaml title="persistentvolume.yaml"
apiVersion: v1
kind: PersistentVolume
metadata:
  name: model-output-pv
spec:
  capacity:
    storage: 1Gi
  accessModes:
    - ReadWriteOnce
  hostPath:
    path: /data/model-output-pv
```

!!! note

    This is an interesting segue to explore how we could attach an NFS storage provider for our CaaS service. For now we'll just use on-disk local filesystems. It would be interesting to support an S3 storage class so that containers could mount something like s3fs to their s3 bucket.



After we apply that, the ML job is finally running!

```
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu/lclipp/worker-basic/k8s# kubectl logs serverless-test-deployment-56564b8c9b-h44lx | tail -n 20
Train Epoch: 1 [49280/60000 (82%)]      Loss: 0.855628
Train Epoch: 1 [49920/60000 (83%)]      Loss: 0.460283
Train Epoch: 1 [50560/60000 (84%)]      Loss: 0.428732
Train Epoch: 1 [51200/60000 (85%)]      Loss: 0.789118
Train Epoch: 1 [51840/60000 (86%)]      Loss: 0.559741
Train Epoch: 1 [52480/60000 (87%)]      Loss: 0.579006
Train Epoch: 1 [53120/60000 (88%)]      Loss: 0.531399
Train Epoch: 1 [53760/60000 (90%)]      Loss: 0.441080
Train Epoch: 1 [54400/60000 (91%)]      Loss: 0.308792
Train Epoch: 1 [55040/60000 (92%)]      Loss: 0.508296
Train Epoch: 1 [55680/60000 (93%)]      Loss: 0.662792
Train Epoch: 1 [56320/60000 (94%)]      Loss: 0.566424
Train Epoch: 1 [56960/60000 (95%)]      Loss: 0.376733
Train Epoch: 1 [57600/60000 (96%)]      Loss: 0.437733
Train Epoch: 1 [58240/60000 (97%)]      Loss: 0.469119
Train Epoch: 1 [58880/60000 (98%)]      Loss: 0.629886
Train Epoch: 1 [59520/60000 (99%)]      Loss: 0.352319

Test set: Avg. loss: 0.2009, Accuracy: 9394/10000 (94%)
```

We can also see the model weights written to the host:

```
root@inst-5c3dw-san-jose-dev-a10-hypervisors-pool:/home/ubuntu/lclipp/worker-basic/k8s# ls -lah /data/model-output-pv/
total 192K
drwxr-xr-x 2 root root 4.0K Jul 31 22:16 .
drwxr-xr-x 3 root root 4.0K Jul 31 22:15 ..
-rw-r--r-- 1 root root  89K Jul 31 22:20 model.pth
-rw-r--r-- 1 root root  89K Jul 31 22:20 optimizer.pth
```