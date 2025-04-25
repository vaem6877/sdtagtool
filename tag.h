#ifndef TAG_H   
#define TAG_H

typedef struct {
    const char *category;
    const char *tag;         // 포인터로 변경
    const char *description;  
} Tag;

extern Tag tags[];
extern int tagCount;

#endif
