// Denavit-Hartenberg parameters
// Index 0..5 = joints A1..A6, same order as ST_RobotPose.aJoints.
TYPE ST_RobotDH :
STRUCT
    nJointCount   : USINT;                // number of valid joints
    aA_m          : ARRAY[0..5] OF LREAL; // link length a [m]
    aAlpha_deg    : ARRAY[0..5] OF LREAL; // link twist alpha [deg]
    aD_m          : ARRAY[0..5] OF LREAL; // link offset d [m]
    aThetaOff_deg : ARRAY[0..5] OF LREAL; // joint angle offset theta [deg]
    aMin_deg      : ARRAY[0..5] OF LREAL; // lower joint limit [deg]
    aMax_deg      : ARRAY[0..5] OF LREAL; // upper joint limit [deg]
END_STRUCT
END_TYPE
