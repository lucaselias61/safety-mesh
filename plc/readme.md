Programmable Logic Controller. Handles the final robot permission and stop layer.

[[plc.svg]]
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

# Inputs

[[Safety Manager#Outputs]]

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
