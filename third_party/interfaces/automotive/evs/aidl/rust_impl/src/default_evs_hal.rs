//
// Copyright (C) 2024 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

use android_hardware_automotive_evs::aidl::android::hardware::automotive::evs::{
    CameraDesc::CameraDesc, DisplayState::DisplayState, IEvsCamera::IEvsCamera,
    IEvsDisplay::IEvsDisplay, IEvsEnumerator::IEvsEnumerator,
    IEvsEnumeratorStatusCallback::IEvsEnumeratorStatusCallback,
    IEvsUltrasonicsArray::IEvsUltrasonicsArray, Stream::Stream,
    UltrasonicsArrayDesc::UltrasonicsArrayDesc,
};

pub struct DefaultEvsHal {}

impl binder::Interface for DefaultEvsHal {}

impl IEvsEnumerator for DefaultEvsHal {
    fn closeCamera(
        &self,
        _: &binder::Strong<(dyn IEvsCamera + 'static)>,
    ) -> std::result::Result<(), binder::Status> {
        Err(binder::StatusCode::UNKNOWN_ERROR.into())
    }

    fn closeDisplay(
        &self,
        _: &binder::Strong<(dyn IEvsDisplay + 'static)>,
    ) -> std::result::Result<(), binder::Status> {
        Err(binder::StatusCode::UNKNOWN_ERROR.into())
    }

    fn closeUltrasonicsArray(
        &self,
        _: &binder::Strong<(dyn IEvsUltrasonicsArray + 'static)>,
    ) -> std::result::Result<(), binder::Status> {
        unimplemented!()
    }

    fn getCameraList(&self) -> std::result::Result<std::vec::Vec<CameraDesc>, binder::Status> {
        Err(binder::StatusCode::UNKNOWN_ERROR.into())
    }

    fn getDisplayIdList(&self) -> std::result::Result<std::vec::Vec<u8>, binder::Status> {
        Err(binder::StatusCode::UNKNOWN_ERROR.into())
    }

    fn getDisplayState(&self) -> std::result::Result<DisplayState, binder::Status> {
        Err(binder::StatusCode::UNKNOWN_ERROR.into())
    }

    fn getStreamList(
        &self,
        _: &CameraDesc,
    ) -> std::result::Result<std::vec::Vec<Stream>, binder::Status> {
        Err(binder::StatusCode::UNKNOWN_ERROR.into())
    }

    fn getUltrasonicsArrayList(
        &self,
    ) -> std::result::Result<std::vec::Vec<UltrasonicsArrayDesc>, binder::Status> {
        unimplemented!()
    }

    fn isHardware(&self) -> std::result::Result<bool, binder::Status> {
        Err(binder::StatusCode::UNKNOWN_ERROR.into())
    }

    fn openCamera(
        &self,
        _: &str,
        _: &Stream,
    ) -> std::result::Result<binder::Strong<(dyn IEvsCamera + 'static)>, binder::Status> {
        Err(binder::StatusCode::UNKNOWN_ERROR.into())
    }

    fn openDisplay(
        &self,
        _: i32,
    ) -> std::result::Result<binder::Strong<(dyn IEvsDisplay + 'static)>, binder::Status> {
        Err(binder::StatusCode::UNKNOWN_ERROR.into())
    }

    fn openUltrasonicsArray(
        &self,
        _: &str,
    ) -> std::result::Result<binder::Strong<(dyn IEvsUltrasonicsArray + 'static)>, binder::Status>
    {
        unimplemented!()
    }

    fn registerStatusCallback(
        &self,
        _: &binder::Strong<(dyn IEvsEnumeratorStatusCallback + 'static)>,
    ) -> std::result::Result<(), binder::Status> {
        Err(binder::StatusCode::UNKNOWN_ERROR.into())
    }

    fn getDisplayStateById(&self, _: i32) -> std::result::Result<DisplayState, binder::Status> {
        Err(binder::StatusCode::UNKNOWN_ERROR.into())
    }
}
