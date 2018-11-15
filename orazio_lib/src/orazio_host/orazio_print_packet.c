#include "orazio_print_packet.h"


int SystemStatusPacket_print(char* f, SystemStatusPacket* p){
  return sprintf(f, "{seq:%05d, rxb:%d, rxp:%d, rxe:%d, rxseq:%05d, txb:%d, txp:%d, txe:%d, batt: %d, wd: %d idle:%d}",
	  p->header.seq,
	  p->rx_buffer_size,
	  p->rx_packets,
	  p->rx_packet_errors,
	  p->rx_seq,
	  p->tx_buffer_size,
	  p->tx_packets,
	  p->tx_packet_errors,
	  p->battery_level,
	  p->watchdog_count,
	  p->idle_cycles);
}

int SystemParamPacket_print(char* f, SystemParamPacket* p){
  return sprintf(f, "{seq:%05d, protocol:%x, firmware:%x, period:%d, baud:%d, com_c:%d, com_f: %02x wd:%d, mot:%d }",
	  p->header.seq,
	  p->protocol_version,
	  p->firmware_version,
	  p->timer_period_ms,
	  p->comm_speed,
	  p->comm_cycles,
	  p->periodic_packet_mask,
	  p->watchdog_cycles,
	  p->num_joints);
}

int JointInfo_print(char* f, JointInfo* j){
  return sprintf(f, "{m:%d, pos: %d, ms: %d, ds: %d, pwm: %d, curr:%d}",
	  j->mode,
	  j->encoder_position,
	  j->encoder_speed,
	  j->desired_speed,
	  j->pwm,
	  j->sensed_current);
}

int JointParams_print(char* f, JointParams* j){
  return sprintf(f, "{kp:%d, ki:%d, kd:%d, maxI:%d, max_pwm:%d, min_pwm:%d, max_speed:%d, slope:%d, ht:%d, hp0:%d, hp1:%d, hp2:%d}",
                 j->kp,
                 j->ki,
                 j->kd,
                 j->max_i,
                 j->max_pwm,
                 j->min_pwm,
                 j->max_speed,
                 j->slope,
                 j->h_bridge_type,
                 j->h_bridge_pins[0],
                 j->h_bridge_pins[1],
                 j->h_bridge_pins[2]);
}

int JointStatusPacket_print(char* f, JointStatusPacket* p){
  char* f_end=f;
  f_end+=sprintf(f_end, "{seq:%05d, j%d:", p->header.header.seq, p->header.index);
  f_end+=JointInfo_print(f_end, &p->info);
  f_end+=sprintf(f_end,"}");
  return f_end-f;
}

int JointParamPacket_print(char* f, JointParamPacket* p){
  char* f_end=f;
  f_end+=sprintf(f_end, "{seq:%05d, j%d:", p->header.header.seq, p->header.index);
  f_end+=JointParams_print(f_end, &p->param);
  f_end+=sprintf(f_end,"}");
  return f_end-f;
}

int DifferentialDriveStatusPacket_print(char* f, DifferentialDriveStatusPacket* p){
  return sprintf(f,
	  "{seq:%05d, x:%.3f, y:%.3f, t:%.3f, tvm:%.3f, tvd:%.3f, tva:%.3f, rvm:%.3f, rvd:%.3f, rva:%.3f}",
	  p->header.seq,
	  p->odom_x,
	  p->odom_y,
	  p->odom_theta,
	  p->translational_velocity_measured,
	  p->translational_velocity_desired,
	  p->translational_velocity_adjusted,
	  p->rotational_velocity_measured,
	  p->rotational_velocity_desired,
	  p->rotational_velocity_adjusted);
  
}

int DifferentialDriveParamPacket_print(char* f, DifferentialDriveParamPacket* p){
  return sprintf(f,
	  "{seq:%05d, ikl:%.3f, ikr:%.3f, b:%.3f, lji:%d, rji:%d, tvmax:%.3f, tamax:%.3f, tdmax:%.3f, rvmax:%.3f, ramax:%.3f}",
	  p->header.seq,
	  p->ikl,
	  p->ikr,
	  p->baseline,
	  p->left_joint_index,
	  p->right_joint_index,
	  p->max_translational_velocity,
	  p->max_translational_acceleration,
	  p->max_translational_brake,
	  p->max_rotational_velocity,
	  p->max_rotational_acceleration);
}

int StringMessagePacket_print(char* f, StringMessagePacket* p){
  return sprintf(f,
	  "{seq:%05d, message:[%s]}",
		 p->header.seq,
		 p->message);
}

int SonarStatusPacket_print(char* f, SonarStatusPacket* p){
  return sprintf(f, "{seq:%05d, [%u %u %u %u %u %u %u %u]}",
                 p->header.seq,
                 p->ranges[0],
                 p->ranges[1],
                 p->ranges[2],
                 p->ranges[3],
                 p->ranges[4],
                 p->ranges[5],
                 p->ranges[6],
                 p->ranges[7]);
}

int SonarParamPacket_print(char* f, SonarParamPacket* p){
  return sprintf(f, "{seq:%05d, [%u %u %u %u %u %u %u %u]}",
                 p->header.seq,
                 (uint32_t)p->pattern[0],
                 (uint32_t)p->pattern[1],
                 (uint32_t)p->pattern[2],
                 (uint32_t)p->pattern[3],
                 (uint32_t)p->pattern[4],
                 (uint32_t)p->pattern[5],
                 (uint32_t)p->pattern[6],
                 (uint32_t)p->pattern[7]);
}

typedef int (*PrintPacketFn)(char* f, PacketHeader*h);
static PrintPacketFn _packet_print_ops[PACKET_TYPE_MAX];
void Orazio_printPacketInit(void){
  for (int i=0; i<PACKET_TYPE_MAX; ++i)
    _packet_print_ops[i]=0;

  _packet_print_ops[SYSTEM_STATUS_PACKET_ID]=(PrintPacketFn)SystemStatusPacket_print;
  _packet_print_ops[SYSTEM_PARAM_PACKET_ID]=(PrintPacketFn)SystemParamPacket_print;
  _packet_print_ops[DIFFERENTIAL_DRIVE_STATUS_PACKET_ID]=(PrintPacketFn)DifferentialDriveStatusPacket_print;
  _packet_print_ops[DIFFERENTIAL_DRIVE_PARAM_PACKET_ID]=(PrintPacketFn)DifferentialDriveParamPacket_print;
  _packet_print_ops[MESSAGE_PACKET_ID]=(PrintPacketFn)StringMessagePacket_print;
  _packet_print_ops[SONAR_STATUS_PACKET_ID]=(PrintPacketFn)SonarStatusPacket_print;
  _packet_print_ops[SONAR_PARAM_PACKET_ID]=(PrintPacketFn)SonarParamPacket_print;
  _packet_print_ops[JOINT_STATUS_PACKET_ID]=(PrintPacketFn)JointStatusPacket_print;
  _packet_print_ops[JOINT_PARAM_PACKET_ID]=(PrintPacketFn)JointParamPacket_print;
}

int Orazio_printPacket(char* f, PacketHeader* h){
  PacketType type=h->type;
  if (type>=PACKET_TYPE_MAX) {
    return 0;
  }
  PrintPacketFn print_fn=_packet_print_ops[type];
  if(!print_fn)
    return 0;
  return (*print_fn)(f,h);
}
