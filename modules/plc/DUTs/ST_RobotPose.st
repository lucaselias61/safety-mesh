TYPE ST_RobotPose :
STRUCT
    nCycle   : UDINT;            // incremented every write → detects stale data
    nJointCount : USINT;         // number of valid entries in aJoints
    fTcpX, fTcpY, fTcpZ : LREAL; // cartesian coordinates of TCP in robot frame 
    fTcpA, fTcpB, fTcpC : LREAL; // orientation
    aJoints  : ARRAY[0..5] OF LREAL;
END_STRUCT
END_TYPE
