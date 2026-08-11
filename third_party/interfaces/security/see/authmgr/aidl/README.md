# AuthMgr

The AuthMgr protocol authenticates and authorizes clients before they can
access trusted HALs, AIDL-defined services in trusted execution environments.
Version 1 was designed to allow applications running in a protected virtual
machine (pVM) to access services running in a TEE in ARM TrustZone. An
implementation of `IAuthMgrAuthorization` is referred to as an AuthMgr Backend.
An implementation of a client of the AuthMgr Backend is referred to as an
AuthMgr Frontend.


## Additional Requirements by Android Version

The comments on `IAuthMgrAuthorization` describe the requirements for implementing
an AuthMgr Backend (implementor of the interface) itself. There are some additional
requirements that are specific to Android release versions.

### Android 16
- If implementing `IAuthMgrAuthorization` in Android 16 only one AuthMgr Backend is
supported and dynamic service discovery is not supported. The AuthMgr Backend
service must be exposed on secure partition ID 0x8001 over VSOCK port 1.

- AuthMgr Front Ends must implement the "android.16" profile as described in the
[Android Profile for DICE](https://pigweed.googlesource.com/open-dice/+/HEAD/docs/android.md#versions)