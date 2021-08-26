#ifndef SOUND_H
#define SOUND_H
extern void setvolume(int involume);
extern int readvolume();
extern void pcm_close();
extern void pcm_init();
#endif
