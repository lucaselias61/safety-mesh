Distributed vision-based safety system for monitoring human-robot workspaces.

[[docs/sysdesign_lucas.svg]]
# Specifications

The system distributes human pose detection across multiple 3D Camera Nodes. A central Safety Manager combines their detections with robot state, makes robot-level safety decisions, and sends those decisions to a PLC for fail-safe enforcement.

# Architecture

- **Camera Nodes**  
  Each [[Camera Node]] processes one 3D camera stream, detects human poses, and transforms them into a shared world coordinate frame.

- **Safety Manager**  
  The [[Safety Manager]] combines Camera Node detections, monitors system health, compares people with robot safety areas, and determines whether each robot may run or must stop.

- **PLC**  
  The [[PLC]] validates the Safety Manager signal and controls the robot safety output. Missing, stale, or unsafe input removes robot run permission.

# Data Flow

3D Cameras -> Camera Nodes -> Safety Manager -> PLC -> Robot Safety Inputs
