* use CONFIG_HIGH_RES_TIMERS for deferred work
    * waiting the result of the  tests
* find a way to re-protect the vmem after page_mkwrite instead of creating a dirty pages list 
    * DONE!!! very proud.
* use infiniband protocol https://en.wikipedia.org/wiki/RDMA_over_Converged_Ethernet
    * rdma is particularly hostile in SUSE 42.x and must be enabled and configured properly, increased sysadmin complexity and risk of user frustration
* use cuda RDMA https://docs.nvidia.com/cuda/gpudirect-rdma/index.html https://docs.nvidia.com/cuda/gpudirect-rdma/index.html#linking-kernel-module-against-nvidia-ko
    * same as above
* ALREADY DONE???? https://docs.nvidia.com/cuda/gpudirect-rdma/index.html#nvidia-peermem >>>>>> https://github.com/NVIDIA/open-gpu-kernel-modules/blob/main/kernel-open/nvidia-peermem/nvidia-peermem.c
    * makes no sense, it would need an nvidia card anyway (and probably Mellanox OFED card too)
* try to use network card dma directly
* uso in docker: https://docs.docker.com/storage/volumes/#share-data-between-machines
