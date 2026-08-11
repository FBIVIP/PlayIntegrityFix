# Rust Skeleton EVS HAL implementation.

WARNING: This is not a reference EVS HAL implementation and therefore does not
provide any actual functionality.

This folder contains a skeleton EVS HAL implementation in Rust to demonstrate
how vendors could implement their EVS HAL in Rust. To compile and run this
implementation, please include below package to the device build script:

* `android.hardware.automotive.evs-aidl-rust-service`

Please note that this service will attempt to register the service as
`IEvsEnumerator/rust/0` and therefore is also required to be declared in the
service context by adding below line to a proper `service_contexts` file:

> android.hardware.automotive.evs.IEvsEnumerator/rust/0 u:object_r:hal_evs_service:s0

This implementation intentionally returns `binder::StatusCode::UNKNOWN_ERROR`
for any API call except deprecated API for ultrasonics; the process will be
panicked on these methods instead. Hence, this implementation does not comply
with VTS tests and vendors must replace each method with actual implementation.
