#include "user_info.h"

void user_info(MsgStruct *msg)
{
#ifdef DEBUG
    printf("Msg version:0x%x\n", msg->version);
    printf("Msg header_len:0x%x\n", msg->header_len);
    printf("Msg encrypt_type:0x%x\n", msg->encrypt_type);
    printf("Msg protocol_type:0x%x\n", msg->protocol_type);
    printf("Msg total_len:0x%x\n", msg->total_len);
    printf("Msg date_type:0x%x\n", msg->data_type);
    printf("Msg seq_num:0x%x\n", msg->seq_num);
    printf("Msg frag_flag:0x%x\n", msg->frag_flag);
    printf("Msg frag_offset:0x%x\n", msg->frag_offset);
    printf("Msg custom1:0x%x\n", msg->custom1);
    printf("Msg custom2:0x%x\n", msg->custom2);
    printf("Msg header_chk:0x%x\n", msg->header_chk);
    printf("Msg source_addr:0x%x\n", msg->source_addr);
    printf("Msg target_addr:0x%x\n", msg->target_addr);
    for(int i = 0; i < msg->total_len-msg->header_len*4; i++)
    {
        printf("Msg data[%d]:0x%x\n", i, msg->data[i]);
    }
#endif
}
