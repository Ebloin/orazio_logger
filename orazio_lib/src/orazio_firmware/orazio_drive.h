#pragma once

// initiaizes the drive controller
PacketStatus Orazio_driveInit(void);

// updates odometry in the status packet
void Orazio_driveUpdate(void);

// flushes to the joints velocities computed to move the base
// with translational and rotational velocity
void Orazio_driveControl(void);

