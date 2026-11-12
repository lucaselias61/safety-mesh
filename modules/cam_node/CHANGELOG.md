# Changelog
All notable changes to this project will be documented in this file. See [conventional commits](https://www.conventionalcommits.org/) for commit guidelines.

- - -
## cam_node-0.3.0 - 2026-11-12
#### Features
- (**cam_node**) add Logger with LOG_LEVEL and Rerun ViewerSink - (9392b66) - lucaselias61
- (**cam_node**) add geometry module with RealSense calibration - (f5e1ab6) - lucaselias61
- (**cam_node**) add Docker host installer and pin shared build deps - (8e99695) - lucaselias61
- (**cam_node**) build and cache TensorRT engines from ONNX on first run - (a5ed856) - lucaselias61
#### Refactoring
- (**cam_node**) extract load_config in docker entrypoint - (7098062) - lucaselias61
- (**cam_node**) build Docker engines with cam_node, not trtexec - (e633df4) - lucaselias61
- (**cam_node**) move TensorRT engine into engines/tensorrt/ - (a45933e) - lucaselias61

- - -

## cam_node-0.2.0 - 2026-11-05
#### Features
- (**cam_node**) add Docker Compose deployment with TensorRT - (2149c37) - lucaselias61
- (**cam_node**) allow installation on Ubuntu 24.04 and newer - (2b679d0) - lucaselias61
- (**cam_node**) add --version flag - (8545a2e) - lucaselias61
#### Bug Fixes
- (**cam_node**) replace undefined check_cuda with handle_cuda_error - (bb714ee) - lucaselias61
#### Documentation
- (**cam_node**) clarify dependency installation on newer Ubuntu releases - (e0b715e) - lucaselias61
#### Refactoring
- rename src to modules - (de678a5) - lucaselias61

- - -

## cam_node-0.1.0 - 2026-11-03
#### Features
- (**cam_node**) add TensorRT inference engine - (9efb0bc) - lucaselias61
- (**cam_node**) display live detections in camera preview - (c860583) - lucaselias61
- (**cam_node**) configurable OpenCV DNN detection engine - (b7d6abf) - lucaselias61
- (**cam_node**) engine interface for model loading and detecting - (b5370ea) - lucaselias61
- (**cam_node**) expose hardware reset for camera class - (710566f) - lucaselias61
- (**cam_node**) view cli - (e63f43f) - lucaselias61
- (**cam_node**) configure camera streams from JSON - (26a2659) - lucaselias61
- (**cam_node**) feature flag for listing supported x active stream profiles - (892a08a) - lucaselias61
- (**cam_node**) list of all supported stream profiles - (b9a9714) - lucaselias61
- (**cam_node**) add cam_.is_ready condition for pipeline to start - (37c0029) - lucaselias61
- (**cam_node**) add streams getter cli - (c4cbd46) - lucaselias61
- (**cam_node**) implement vision model along with placeholders for future inference - (8630fc2) - lucaselias61
- (**cam_node**) cam worker for the detection pipeline handling - (0d3e87b) - lucaselias61
- (**cam_node**) rs2 wrapper for managing and getting frames from an active camera pipeline - (5bd57c9) - lucaselias61
- (**cam_node**) add cli app using CLI11 - (642c6c2) - lucaselias61
- (**cam_node**) the cpp script for discovery - (7618f0f) - lucaselias61
- (**cam_node**) the scripts needed to discover all connected devices and build the binary - (feabd83) - lucaselias61
- (**cam_node**) add data structure to retrive and save cam's metadata - (b3e14e5) - lucaselias61
- (**cam_node**) add data structure to retrive and save cam's metadata - (e05e436) - lucaselias61
- (**cam_node**) discover RealSense cameras and save intrinsics - (b89cc2f) - lucaselias61
- (**deprojection**) add deprojection module for cam frame - (776cf55) - lucaselias61
- (**streams**) add resolution sorting - (b476ba2) - lucaselias61
#### Bug Fixes
- (**cam_node**) correct bounding box handling and address clang-tidy warnings - (12d3ce9) - lucaselias61
- (**cam_node**) allow SDK exc to throw from stream profile getters - (7982b07) - lucaselias61
- (**cam_node**) always list supported profiles in streams command - (fe97c86) - lucaselias61
- (**cam_node**) missing early exit - (4c06697) - lucaselias61
- (**cam_node**) dangling reference - (77f0b64) - lucaselias61
- (**cam_node**) handle frame timeouts with try_wait_for_frames - (13493fa) - lucaselias61
- (**cam_node**) missing import - (c45a0fa) - lucaselias61
- (**cam_node**) define missing constructor - (b60dfe0) - lucaselias61
- (**cam_node**) correct exec path - (e44060b) - lucaselias61
#### Performance Improvements
- (**cam_node**) reuse buffers for color and depth preview - (e2a8ba8) - lucaselias61
#### Documentation
- (**cam_node**) GPU support for TensorRT backend - (096dee3) - lucaselias61
- (**cam_node**) describe build and dependency scripts - (78f24c4) - lucaselias61
- (**cam_node**) reorganize cam_node documentation and explain current behavior - (4a0e3eb) - lucaselias61
- (**cam_node**) note MIG for GPU acceleration - (d8a5239) - lucaselias61
- (**cam_node**) TensorRT backend build and usage - (28b2758) - lucaselias61
- (**cam_node**) inference engine docs - (6af9fa4) - lucaselias61
- (**cam_node**) update inference and realsense docs - (8b8377e) - lucaselias61
- (**cam_node**) update readme - (b4f97d7) - lucaselias61
- (**cam_node**) build logic - (2308494) - lucaselias61
- add READMEs - (c90c732) - lucaselias61
#### Build system
- (**cam_node**) add TensorRT setup and engine generation scripts - (d8f4399) - lucaselias61
- drop conan completly, pin Ubuntu 24.04 apt dependencies - (bf3e1e1) - lucaselias61
#### Refactoring
- (**cam_node**) group streams options into a struct - (d6c9cfa) - lucaselias61
- (**cam_node**) one file per cli command - (afa5d50) - lucaselias61
- (**cam_node**) move CLI subcommand implementations into cli.cpp - (1fa0524) - lucaselias61
- (**cam_node**) delegate error handling to main - (0c6f218) - lucaselias61
- (**cam_node**) delegate error handling to main - (dd2e661) - lucaselias61
- (**cam_node**) removed conan for simplicity and adde bootstraping scripts - (c7ad35e) - lucaselias61
- (**cam_node**) termination handling (docker or cmd line) for clean exit cleanup - (551fd18) - lucaselias61
- (**cam_node**) increased responsability delegated to class Camera - (756ae77) - lucaselias61
- (**cam_node**) subcommand renaming - (62f3145) - lucaselias61
- (**cam_node**) add CLI subcommand specific config files - (04d77f2) - lucaselias61
- (**cam_node**) discovery is not a seperate executable but a CLI. responsabilities have been separated for all usecases - (1992343) - lucaselias61
- (**config**) example file rename - (060eccc) - lucaselias61
- monorepo style SemVer - (5d7a1b4) - lucaselias61
#### Miscellaneous Chores
- (**cam_node**) drop macOS support and rename installer - (4dbc1b9) - lucaselias61
- (**cam_node**) remove view from run pipeline - (302ca32) - lucaselias61
- (**cam_node**) add clang-tidy configuration - (b3a10ac) - lucaselias61
- (**cam_node**) decresed fps on view cli - (cefaa21) - lucaselias61
- (**cam_node**) heatmap display with view cli - (2e7881a) - lucaselias61
- (**cam_node**) implement stream list filtering and sorting - (e57d8c6) - lucaselias61
- (**cam_node**) filter profiles by var 'key' - (6a7f5fd) - lucaselias61
- (**cam_node**) enabled streams logging - (971d9c7) - lucaselias61
- (**cam_node**) streams list keyed by unique id - (ef90416) - lucaselias61
- (**cam_node**) rework the list structure - (1ce25c0) - lucaselias61
- (**cam_node**) rework the list structure - (fab07d9) - lucaselias61
- (**cam_node**) built artifacts are separated between bin/ and build/ - (f39e2de) - lucaselias61
- (**cam_node**) cmake list file contains the new discover exec logic - (78c846e) - lucaselias61
- (**cam_node**) add conan for package managment and build presets - (1362f64) - lucaselias61
- (**cam_node**) add conan lock file, cmake list, main script, update gitignore - (b60d5b6) - lucaselias61
#### Style
- (**cam_node**) formatting - (33d43b2) - lucaselias61
- (**cam_node**) applied 'clang-format' formating - (bdbe8f7) - lucaselias61

- - -

Changelog generated by [cocogitto](https://github.com/cocogitto/cocogitto).