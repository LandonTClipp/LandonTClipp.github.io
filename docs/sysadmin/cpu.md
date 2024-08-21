# CPU

## [CPU Branch Prediction](https://en.wikipedia.org/wiki/Branch_predictor) <!-- md:optimization -->

Branch prediction is a digital circuit on most modern CPUs that attempts to guess which direction an if/else statement (a branch) will take. The accuracy of the prediction plays a major role into improving the pipelining efficiency of a CPU. The predicted instructions will be preemptively executed. If the result of the branch is different from the predicted path, the preemptive result is thrown away and execution is resumed from the true branch path.

## [IOMMU](https://en.wikipedia.org/wiki/Input%E2%80%93output_memory_management_unit)

![IOMMU diagram](https://upload.wikimedia.org/wikipedia/commons/d/d6/MMU_and_IOMMU.svg){ align=left width=400px }

IOMMU is a memory-management unit that connects a direct-memory-access-capable (DMA-capable) IO bus to the main memory. IOMMUs are similar to CPU MMUs in that it translates device-visible virtual addresses to phyiscal addresses.

An example IOMMU is the graphics address remapping table used by AGP and PCIe graphics cards on Intel and AMD computers.

## [Green Thread](https://en.wikipedia.org/wiki/Green_thread)

A green thread is a thread whose execution is controlled by a runtime or virtual machine instead of by the OS kernel.

The Python asyncio event loop is an example of a runtime executing multiple coroutines on a single green thread.