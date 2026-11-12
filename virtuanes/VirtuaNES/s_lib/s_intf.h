// simple interface for player
// by fanoble

#ifdef __cplusplus
extern "C" {
#endif

int player_init(int sample_rate);
int player_done(void);

int player_play(const char* url, void* cookie);
int player_stop(void);

int player_pause(int on);

int player_get_duration(void);
int player_get_pos(void);
int player_seek(int ms);

// 0 : playing
// 1 : paused
// 2 : stopped
int player_get_status(void);

// pcm format: 16bit 2ch
// should be implemented by user
int player_on_data(short* pcm, int samples, void* cookie);

#ifdef __cplusplus
}
#endif
