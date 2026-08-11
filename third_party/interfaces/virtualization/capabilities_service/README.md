The IVmCapabilitiesService HAL is used in a flow to grant a pVM a capability to
issue vendor-specific SMCs. For more information see: TODO(ioffe): link the docs

Here is a brief overview of the subdirectories structure:

* default/ - a reference implementation of the HAL that partners can integrate
    in their products.
* noop/ - a no-op implementation is used in cuttlefish for mixed build testing.
* vts/ - VTS tests for this HAL.
