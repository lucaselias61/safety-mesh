# Beckhoff's open-source ADS client (MIT), standalone variant as beckhoff_adslib target.

# Talks to TwinCAT PLC over TCP without a local TwinCAT install. 
# Only the sources are downloaded and compiled. Offline builds can point
# FETCHCONTENT_SOURCE_DIR_BECKHOFF_ADS at a local checkout.

include(FetchContent)
FetchContent_Declare(beckhoff_ads
    URL https://github.com/Beckhoff/ADS/archive/24e598a113edfdc0d34414bfefb847930d94cb87.tar.gz
    URL_HASH SHA256=9230ffe55d77346ee6a921902bb1b1f1a14da81c2f20d9321f8d8defdb823cf3
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR do-not-add-subdirectory
)
FetchContent_MakeAvailable(beckhoff_ads)
set(ADSLIB_DIR "${beckhoff_ads_SOURCE_DIR}/AdsLib")

find_package(Threads REQUIRED)
add_library(beckhoff_adslib STATIC
    ${ADSLIB_DIR}/AdsDef.cpp
    ${ADSLIB_DIR}/AdsDevice.cpp
    ${ADSLIB_DIR}/AdsFile.cpp
    ${ADSLIB_DIR}/AdsLib.cpp
    ${ADSLIB_DIR}/ECatAccess.cpp
    ${ADSLIB_DIR}/Frame.cpp
    ${ADSLIB_DIR}/LicenseAccess.cpp
    ${ADSLIB_DIR}/Log.cpp
    ${ADSLIB_DIR}/MasterDcStatAccess.cpp
    ${ADSLIB_DIR}/RTimeAccess.cpp
    ${ADSLIB_DIR}/RegistryAccess.cpp
    ${ADSLIB_DIR}/RouterAccess.cpp
    ${ADSLIB_DIR}/Sockets.cpp
    ${ADSLIB_DIR}/SymbolAccess.cpp
    ${ADSLIB_DIR}/SysServAccess.cpp
    ${ADSLIB_DIR}/bhf/ParameterList.cpp
    ${ADSLIB_DIR}/standalone/AdsLib.cpp
    ${ADSLIB_DIR}/standalone/AmsConnection.cpp
    ${ADSLIB_DIR}/standalone/AmsNetId.cpp
    ${ADSLIB_DIR}/standalone/AmsPort.cpp
    ${ADSLIB_DIR}/standalone/AmsRouter.cpp
    ${ADSLIB_DIR}/standalone/NotificationDispatcher.cpp
)
set_target_properties(beckhoff_adslib PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_features(beckhoff_adslib PUBLIC cxx_std_14)
target_compile_definitions(beckhoff_adslib PRIVATE
    _GNU_SOURCE CONFIG_DEFAULT_LOGLEVEL=1)
# Third-party code: don't surface its warnings in our builds.
target_include_directories(beckhoff_adslib SYSTEM PUBLIC "${ADSLIB_DIR}")
target_compile_options(beckhoff_adslib PRIVATE -w)
target_link_libraries(beckhoff_adslib PUBLIC Threads::Threads)
