#pragma once


#ifdef __cplusplus
extern "C" {
#endif

  typedef enum {
    TypePacket=0,
    TypeUInt8=1,
    TypeInt8=2,
    TypeUInt16=3,
    TypeInt16=4,
    TypeUInt32=5,
    TypeInt32=6,
    TypeInt32Hex=7,
    TypeFloat=8,
    TypeString=9
  } VariableType;

  typedef enum {
    VarRead=0x1,
    VarWrite=0x2
  } VarAccessFlags;
  
  typedef struct {
    const char* name;
    VariableType type;
    void* ptr;
    int access_flags;
  } VEntry;

  int VEntry_read(VEntry* v, const char* s);

  int VEntry_write(char* s, VEntry* v);

#define RV(var_name, var_type, var_flags)       \
  {						\
    .name = #var_name,				\
      .type = var_type,				\
      .ptr  = &var_name,			\
      .access_flags = var_flags			\
      } 

#ifdef __cplusplus
}
#endif
