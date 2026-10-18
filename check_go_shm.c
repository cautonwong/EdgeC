#include <uv.h>
#include "datahub.h"
#include <unistd.h>
#include <stdio.h>

int main() {
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    
    // We pass name "demo" and capacity 1024
    if (datahub_init(&hub, &loop, "demo", 128, 1024, NULL, 0, NULL, NULL, NULL) != 0) {
        printf("Failed to init datahub\n");
        return 1;
    }
    
    pipe_msg_t msg = {0};
    msg.tag_id = 1001;
    msg.type = PIPEMSG_TYPE_SCALAR;
    
    printf("C DataHub running, writing stream to tag 1001...\n");
    fflush(stdout);
    
    for (int i=0; i<10; i++) {
        msg.payload.val.f = 220.0 + i*0.1;
        msg.ts = i; // Mock TS
        datahub_write_shadow(&hub, &msg);
        usleep(500000); // 500ms
    }
    
    datahub_destroy(&hub);
    return 0;
}
