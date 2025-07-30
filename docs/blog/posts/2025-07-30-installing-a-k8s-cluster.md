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
