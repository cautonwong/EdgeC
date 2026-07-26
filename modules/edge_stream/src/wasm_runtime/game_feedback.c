#define IMPORT(name) __attribute__((import_module("env"), import_name(name)))

IMPORT("host_get_tag") int host_get_tag(unsigned int tag_id, double *val_ptr);
IMPORT("host_set_tag") int host_set_tag(unsigned int tag_id, double val);
IMPORT("ext_log") void ext_log(unsigned int level, const char *msg);

static int g_last_completion = 0;

__attribute__((export_name("on_tick")))
void on_tick() {
    double completion = 0.0;
    if (host_get_tag(12007, &completion) == 0) { // TAG_REHAB_COMPLETION_RATE
        int cur = (int)completion;
        if (cur >= 100 && g_last_completion < 100) {
            ext_log(3, "WASM GAME REWARD: 训练目标已达成！播放正向激励语音: 今天又进步了哦，加油！");
            host_set_tag(12008, 1.0); // TAG_VOICE_PROMPT_REWARD = 1
        }
        g_last_completion = cur;
    }
}
