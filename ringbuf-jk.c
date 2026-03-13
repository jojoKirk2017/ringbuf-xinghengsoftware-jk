/*
 * ringbuf-jk.c
 *
 * 工业级环形缓冲区（无锁 / 支持32位计数溢出 / 帧校验）
 *
 * Author: J.K
 * From: 星珩软件工作室
 *
 * 本项目为自由软件 (Free Software)
 * 可自由使用、复制、修改、分发
 * 致敬自由软件精神！
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define QUEUE_LEN    1024
#define MINDATA      10
#define FRAME_HEAD   0x55
#define FRAME_TAIL   0xAA

uint32_t WW = 0;
uint32_t RW = 0;
uint8_t  data_buf[QUEUE_LEN];

static uint32_t get_write_offset(void) {
    return WW % QUEUE_LEN;
}

static uint32_t get_read_offset(void) {
    return RW % QUEUE_LEN;
}

static uint32_t ringbuf_used(void) {
    uint32_t w_off = get_write_offset();
    uint32_t r_off = get_read_offset();
    if (w_off >= r_off)
        return w_off - r_off;
    else
        return (QUEUE_LEN - r_off) + w_off;
}

static bool ringbuf_is_full(void) {
    return ringbuf_used() >= QUEUE_LEN;
}

void buffer_write(uint8_t *data, uint32_t len) {
    if (data == NULL || len == 0) return;
    for (uint32_t i = 0; i < len; i++) {
        if (ringbuf_is_full()) {
            RW++;
        }
        uint32_t w_off = get_write_offset();
        data_buf[w_off] = data[i];
        WW++;
    }
}

uint32_t buffer_read(uint8_t *read_out) {
    uint32_t read_cnt = 0;
    if (ringbuf_used() < MINDATA) return 0;

    while (1) {
        uint32_t used = ringbuf_used();
        if (used < MINDATA) break;

        uint32_t r_off = get_read_offset();
        if (data_buf[r_off] == FRAME_HEAD) {
            uint32_t tail_rw = RW + MINDATA - 1;
            uint32_t tail_off = tail_rw % QUEUE_LEN;
            if (data_buf[tail_off] == FRAME_TAIL) {
                for (uint32_t i = 0; i < MINDATA; i++) {
                    uint32_t off = (RW + i) % QUEUE_LEN;
                    read_out[read_cnt++] = data_buf[off];
                }
                RW += MINDATA;
                break;
            }
        }
        RW++;
    }
    return read_cnt;
}