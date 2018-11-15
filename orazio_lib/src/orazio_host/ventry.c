#include <string.h>
#include <stdio.h>
#include "ventry.h"
#include "orazio_print_packet.h"

int VEntry_read(VEntry* v, const char* s) {
  if (!(v->access_flags&VarWrite)){
    printf("access error Owned: %d Required: %d\n", v->access_flags, VarRead);
    return 0;
  }
  switch(v->type){
  case TypeInt8:
    return sscanf(s, "%hhd",(int8_t*)v->ptr);
  case TypeUInt8:
    return sscanf(s, "%hhu",(uint8_t*)v->ptr);
  case TypeInt16:
    return sscanf(s, "%hd",(int16_t*)v->ptr);
  case TypeUInt16:
    return sscanf(s, "%hu",(uint16_t*)v->ptr);
  case TypeInt32:
    return sscanf(s, "%d",(int32_t*)v->ptr);
  case TypeUInt32:
    return sscanf(s, "%u",(uint32_t*)v->ptr);
  case TypeInt32Hex:
    return sscanf(s, "%x",(int32_t*)v->ptr);
  case TypeFloat:
    return sscanf(s, "%f",(float*)v->ptr);
  case TypeString:
    return sscanf(s, "%s",(char*)v->ptr);
  default:
    return 0;
  }
}

int VEntry_write(char* s, VEntry* v) {
  if (!(v->access_flags&VarRead)){
    printf("access error Owned: %d Required: %d\n", v->access_flags, VarRead);
    return 0;
  }
  switch(v->type){
  case TypeInt8:
    return sprintf(s, "%hhd", *(int8_t*)(v->ptr));
  case TypeUInt8:
    return sprintf(s, "%hhu",*(uint8_t*)(v->ptr));
  case TypeInt16:
    return sprintf(s, "%hd",*(int16_t*)(v->ptr));
  case TypeUInt16:
    return sprintf(s, "%hu",*(uint16_t*)(v->ptr));
  case TypeInt32:
    return sprintf(s, "%d",*(int32_t*)(v->ptr));
  case TypeUInt32:
    return sprintf(s, "%u",*(int32_t*)(v->ptr));
  case TypeInt32Hex:
    return sprintf(s, "%08x",*(uint32_t*)(v->ptr));
  case TypeFloat:
    return sprintf(s, "%.5f",*(float*)(v->ptr));
  case TypeString:
    return sprintf(s, "%s",(char*)(v->ptr));
  default: {
    printf("printing packet %d\n", ((PacketHeader*)v->ptr)->type);
    return Orazio_printPacket(s, (PacketHeader*)v->ptr);
  }
  }
}

