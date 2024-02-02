/************************************************************
 *   Author : German Mundinger
 *   Date   : 2020
 ************************************************************/

#include "board.h"


int main ()
{
    board_init();
    board_launch();

    return 0;
}


// Silence mode:    {"src_id":1,"dst_id":[0,2],"cmd_id":1,"data":{"mode_id":0}}
// Intrusion mode:  {"src_id":1,"dst_id":[0,2],"cmd_id":1,"data":{"mode_id":1}}
// Alarm mode:      {"src_id":1,"dst_id":[0,2],"cmd_id":1,"data":{"mode_id":2}}
// Light on:        {"src_id":1,"dst_id":[0,2],"cmd_id":2,"data":{"mode_id":1}}
// Light off:       {"src_id":0,"dst_id":[2],"cmd_id":2,"data":{"mode_id":0}}