#ifndef CH32CLOCK_OLED_H
#define CH32CLOCK_OLED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OLED_WIDTH 128U
#define OLED_PAGES 8U
#define OLED_BUFFER_SIZE (OLED_WIDTH * OLED_PAGES)
#define OLED_FONT_WIDTH 6U

typedef enum {
    OLED_OK = 0,
    OLED_NACK,
    OLED_TIMEOUT,
    OLED_BUS_ERROR,
    OLED_ARBITRATION_LOST,
    OLED_INVALID_ARGUMENT,
    OLED_BUSY,
    OLED_DMA_ERROR,
} OLED_STATUS;

typedef struct oledBuffer {
    uint8_t padding[3];
    uint8_t commandByte;
    /* 控制字紧贴像素，DMA 可从 commandByte 连续发送 1025 字节。
     * oledData 仍为 4 字节对齐；每字节表示竖向 8 像素。 */
    uint8_t oledData[OLED_BUFFER_SIZE] __attribute__((aligned(4)));
} oledBuffer;

typedef struct OLED {
    uint8_t address; /* 7 位 I2C 地址，发送层负责左移。 */
    volatile uint8_t status; /* 最近一次发送结果；绘图函数只返回结果，不覆盖异步发送状态。 */
    volatile uint16_t dataLength;

    volatile uint8_t nonstopFlush; /* 1：整帧完成后继续发送；置 0 后完成已提交的帧再停。 */
    volatile uint8_t exchangeBuffer; /* 绘图端置 1 提交；帧完成回调交换成功后清零。 */
    /* 1：后台还在写，回调保留交换请求，到下一帧再试。set/clear 自动保护单次操作。
     * 批量绘图先置 1，完成全部绘图后先置 exchangeBuffer=1，再清除此标志。 */
    volatile uint8_t isWritingBackBuffer;
    volatile uint8_t isTransmitting; /* 一次传输尚未结束，transmitData/dataLength 不得改写。 */

    volatile uint8_t isFlushing; /* 整屏发送或连续刷新活动中。 */
    uint8_t positionCommand[6]; /* 持久化的列/页窗口命令。 */

    const uint8_t *volatile transmitData; /* 数据发送期间保持有效，到完成处理才释放。 */
    OLED_STATUS (*transmit)(struct OLED *self); /* 命令同步发送，像素启动 DMA 后立即返回。 */
    OLED_STATUS (*transmitCallBack)(struct OLED *self); /* DMA + I2C 完成/出错后的回调。 */
    struct oledBuffer *volatile backBuffer; /* 绘图端写入，写入期间保持 isWritingBackBuffer=1。 */
    /* DMA 独占的前台像素：绘图端禁止修改。发送结束前不得交换或复用此缓冲区。 */
    struct oledBuffer *volatile frontBuffer;
} OLED;

extern OLED g_oled;

/* 初始化控制器，不改变已有显存内容；对象须配置两块独立的静态缓冲区。 */
OLED_STATUS oled_Init(OLED *self);
/* 命令同步发送；刷新活动中返回 OLED_BUSY。 */
OLED_STATUS oled_WriteCmd(OLED *self, const uint8_t *data, uint16_t len);
/* 启动异步像素发送，OLED_OK 表示已启动；data 须保留到 isTransmitting 清零。 */
OLED_STATUS oled_WriteData(OLED *self, const uint8_t *data, uint16_t len);
/* 水平寻址窗口：列 offset..127，页 page..7。 */
OLED_STATUS oled_SetPos(OLED *self, uint8_t page, uint8_t offset);
/* 写字从指定页第 0 列开始，保留文字之后的像素；以下绘图操作只更新后台显存。 */
OLED_STATUS oled_setString(OLED *self, uint8_t page, const char *str, uint8_t charNum);
/* 将后台显存清零，不立即刷新屏幕。 */
OLED_STATUS oled_clearBuffer(OLED *self);
/* 从独立数据复制连续 pageCount 页到后台显存，不立即刷新。
 * data 也须 4 字节对齐才能走 lw/sw；未对齐时自动使用字节复制。
 * 源数据至少包含 pageCount * OLED_WIDTH 字节，且不能与目标区域重叠。 */
OLED_STATUS oled_setPages(OLED *self, uint8_t firstPage, const uint8_t *data, uint8_t pageCount);
/* 仅在 DMA 停止、最后一个 I2C 字节和 STOP 完成后调用。
 * exchangeBuffer=1 且 isWritingBackBuffer=0 才交换并清除请求；忙时保留到下一帧。
 * 错误时停止刷新并保留请求；连续模式或有待显示的新帧时启动下一整帧。 */
OLED_STATUS oled_TransmitCallBack(OLED *self);
/* 提交后台画面并启动一次 1024 字节 DMA；活动中只登记交换请求，不中途交换。
 * 空闲启动时提交首帧，之后仅整帧完成回调有权交换。新后台继承本帧以便局部绘图。
 * OLED_OK 表示启动/提交成功，异步结果读 status；nonstopFlush=1 时自动连续刷新。
 * 只允许一个主循环绘图端；直接写 backBuffer 前先置写标志，禁止跨写入批次缓存指针。 */
OLED_STATUS oled_Flush(OLED *self);

#ifdef __cplusplus
}
#endif

#endif
