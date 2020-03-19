#ifndef FRM_PAR
#define FRM_PAR

#include "header_info.h"
#include "conn_manager.h"


int parse_frame(char* buf, size_t len,uint32_t& conn_id,addr_port& srcaddr);


#endif

