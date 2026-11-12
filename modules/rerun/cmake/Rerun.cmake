# Rerun C++ SDK (MIT/Apache-2.0) as target rerun_sdk.
# Prebuilt for Linux x64/arm64 and macOS arm64. First configure builds Arrow
# from source (a few minutes). Offline: set FETCHCONTENT_SOURCE_DIR_RERUN_SDK
# to an extracted rerun_cpp_sdk.zip.

# Must match RERUN_VERSION in ../Dockerfile.
set(RERUN_SDK_VERSION 0.43.1)

include(FetchContent)
FetchContent_Declare(rerun_sdk
    URL https://github.com/rerun-io/rerun/releases/download/${RERUN_SDK_VERSION}/rerun_cpp_sdk.zip
    URL_HASH SHA256=77274889e8b26a51b5946a819d37d51a4ae9df389be51dc46c2a8839a02eebc4
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(rerun_sdk)
