# Programmable Logic Controller

Handles the final robot permission and stop layer.

# Specifications

Written in the PLC's native safety language because the PLC must enforce the final fail-safe stop independently from the PC software and remain reliable during communication or system failures.

# Behaviour

- **Orchestrator State Intake**  
  Receives robot-level safety states from the Orchestrator.

- **Orchestrator's Signal Health Check**  
  Checks that Orchestrator messages are valid, and arriving within the expected time.

- **Run Permission Logic**  
  Allows robot to move only while the Orchestrator state is `RUN` and all safety conditions are healthy.

- **Fail-Safe**  
  Removes robot run permission if the Orchestrator state is `STOP`, messages become stale or communication fails.

- **Robot Safety Output**  
  Controls the wired robot safety input, safety relay, or STO channel.

- **Built-In Fault Detection**
  PLC diagnostics to detect internal faults, communication timeouts, I/O errors, or output inconsistencies. On fault, robot stops.

# PLC Code

Structured Text that must be implemented on the PLC. Folders follow the TwinCAT project layout.

| File | Type | Used by |
| --- | --- | --- |
| `DUTs/ST_RobotPose.st` | `ST_RobotPose`: robot TCP pose and joints, `nCycle` incremented on every write | Manager `RobotPosePoller` reads it over ADS (e.g. `GVL.stRobotPose`). Packed (100 bytes) and 8-byte aligned (104 bytes) layouts are both accepted. Pose counts as stale when `nCycle` stops advancing. |
| `DUTs/ST_RobotDH.st` | `ST_RobotDH`: DH table (a, alpha, d, theta offset) and joint limits for A1..A6 | Instance in the GVL (e.g. `GVL.stRobotDH`), readable over ADS in one read. Mirrors `modules/manager/config/robots/kuka_kr10_r1100.json`; keep both in sync. |

# Inputs

[[Manager#Outputs]]

# Outputs

- *DEBUG: Robot 1 (Run Allowed)*

		{
		  "robot_serial": "22344555",
		  "robot_id": "robot_1",
		  "timestamp": "2026-06-23T15:42:10.160Z",
		  "plc_state": "RUNNING",
		  "input_state": "RUN",
		  "orchestrator_status": "OK",
		}
