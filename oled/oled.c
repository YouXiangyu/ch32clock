/* 双缓冲显存：绘图只写后台，整帧 DMA 完成后按请求交换。 */
#include "oled.h"
#include "oledConst.h"
#include <stddef.h>
#include <string.h>

/* GCC may_alias 允许以 32 位访问字节显存，避免违反严格别名规则。 */
typedef uint32_t oledWord __attribute__((may_alias));

_Static_assert(_Alignof(oledBuffer) >= 4U, "OLED buffer must be word aligned");
_Static_assert(offsetof(oledBuffer, oledData) % 4U == 0U, "OLED pixels must be word aligned");
_Static_assert(offsetof(oledBuffer, oledData) == offsetof(oledBuffer, commandByte) + 1U,
               "OLED control byte and pixels must be contiguous for DMA");
_Static_assert(OLED_WIDTH % sizeof(oledWord) == 0U, "OLED page must contain whole words");

static OLED_STATUS oled_transmit(OLED *self, uint8_t control, const uint8_t *data, uint16_t len);
static void oled_copy_pixels(uint8_t *pixels, const uint8_t *data, uint16_t len);
static uint8_t oled_begin_back_write(OLED *self);
static void oled_end_back_write(OLED *self, uint8_t previous);
static OLED_STATUS oled_send_window(OLED *self, uint8_t page, uint8_t offset);
static OLED_STATUS oled_start_frame(OLED *self);
static uint8_t oled_exchange_buffers(OLED *self);

static uint8_t oled_begin_back_write(OLED *self)
{
    uint8_t previous = self->isWritingBackBuffer;
    self->isWritingBackBuffer = 1U;
    /* 单前台写入者 + 帧完成中断：禁止把指针读取/显存写入提前到保护标志之前。 */
    __asm__ volatile ("" ::: "memory");
    return previous;
}

static void oled_end_back_write(OLED *self, uint8_t previous)
{
    __asm__ volatile ("" ::: "memory");
    /* 保留调用方的批量绘图保护，不能在一次 set/clear 后提前释放外层保护。 */
    self->isWritingBackBuffer = previous;
}

static OLED_STATUS oled_transmit(OLED *self, uint8_t control, const uint8_t *data, uint16_t len)
{
    if (self->isTransmitting != 0U) {
        return OLED_BUSY;
    }
    if (len > OLED_BUFFER_SIZE || (data == NULL && len != 0U)) {
        self->status = OLED_INVALID_ARGUMENT;
        return OLED_INVALID_ARGUMENT;
    }
    if (len == 0U) {
        return OLED_OK;
    }

    self->frontBuffer->commandByte = control;
    self->transmitData = data;
    self->dataLength = len;
    self->status = OLED_OK;
    self->isTransmitting = 1U;
    OLED_STATUS result = self->transmit(self);
    if (control == 0x00U || result != OLED_OK) {
        /* 命令是同步传输；像素成功启动后，只有底层完成处理可以释放这些字段。 */
        self->transmitData = NULL;
        self->dataLength = 0U;
        self->status = (uint8_t)result;
        self->isTransmitting = 0U;
    }
    return result;
}

OLED_STATUS oled_Init(OLED *self)
{
    return oled_WriteCmd(self, &oledInitBitstream[1], sizeof(oledInitBitstream) - 1U);
}

OLED_STATUS oled_WriteCmd(OLED *self, const uint8_t *data, uint16_t len)
{
    if (self->isFlushing != 0U) {
        return OLED_BUSY;
    }
    return oled_transmit(self, 0x00U, data, len);
}

OLED_STATUS oled_WriteData(OLED *self, const uint8_t *data, uint16_t len)
{
    if (self->isFlushing != 0U) {
        return OLED_BUSY;
    }
    return oled_transmit(self, 0x40U, data, len);
}

OLED_STATUS oled_SetPos(OLED *self, uint8_t page, uint8_t offset)
{
    if (self->isFlushing != 0U || self->isTransmitting != 0U) {
        return OLED_BUSY;
    }
    if (page >= OLED_PAGES || offset >= OLED_WIDTH) {
        self->status = OLED_INVALID_ARGUMENT;
        return OLED_INVALID_ARGUMENT;
    }

    return oled_send_window(self, page, offset);
}

static OLED_STATUS oled_send_window(OLED *self, uint8_t page, uint8_t offset)
{
    /* SSD1315 水平寻址：发送完第 127 列后自动进入下一页。 */
    self->positionCommand[0] = 0x21U;
    self->positionCommand[1] = offset;
    self->positionCommand[2] = OLED_WIDTH - 1U;
    self->positionCommand[3] = 0x22U;
    self->positionCommand[4] = page;
    self->positionCommand[5] = OLED_PAGES - 1U;
    return oled_transmit(self, 0x00U, self->positionCommand, sizeof(self->positionCommand));
}

OLED_STATUS oled_setString(OLED *self, uint8_t page, const char *str, uint8_t charNum)
{
    uint16_t len = 0U;
    if (str == NULL || page >= OLED_PAGES) {
        return OLED_INVALID_ARGUMENT;
    }
    uint8_t previous = oled_begin_back_write(self);
    uint8_t *pixels = &self->backBuffer->oledData[page * OLED_WIDTH];

    for (uint8_t i = 0U; i < charNum && len + OLED_FONT_WIDTH <= OLED_WIDTH; ++i) {
        uint8_t ch = (uint8_t)str[i];
        if (ch == 0U) {
            break;
        }
        /* 原字库未覆盖全部可打印 ASCII，缺字统一使用问号，避免越界读 Flash。 */
        if (ch < ' ' || (uint16_t)(ch - ' ') >= sizeof(asc2_8x6) / sizeof(asc2_8x6[0])) {
            ch = '?';
        }
        const uint8_t *glyph = asc2_8x6[ch - ' '];
        memcpy(&pixels[len], glyph, OLED_FONT_WIDTH);
        len += OLED_FONT_WIDTH;
    }

    oled_end_back_write(self, previous);
    return OLED_OK;
}

OLED_STATUS oled_clearBuffer(OLED *self)
{
    uint8_t previous = oled_begin_back_write(self);
    oledWord *words = (oledWord *)self->backBuffer->oledData;
    for (uint16_t i = 0U; i < OLED_BUFFER_SIZE / sizeof(oledWord); ++i) {
        words[i] = 0U;
    }
    oled_end_back_write(self, previous);
    return OLED_OK;
}

static void oled_copy_pixels(uint8_t *pixels, const uint8_t *data, uint16_t len)
{
    if (((uintptr_t)data & 3U) != 0U) {
        memcpy(pixels, data, len);
        return;
    }

    /* 调用方保证目标位于页首、长度为整页；源地址已检查，按字复制。 */
    oledWord *destination = (oledWord *)pixels;
    const oledWord *source = (const oledWord *)data;
    for (uint16_t i = 0U; i < len / sizeof(oledWord); ++i) {
        destination[i] = source[i];
    }
}

OLED_STATUS oled_setPages(OLED *self, uint8_t firstPage, const uint8_t *data, uint8_t pageCount)
{
    if (firstPage >= OLED_PAGES || pageCount > OLED_PAGES - firstPage ||
        (data == NULL && pageCount != 0U)) {
        return OLED_INVALID_ARGUMENT;
    }
    if (pageCount != 0U) {
        uint8_t previous = oled_begin_back_write(self);
        oled_copy_pixels(&self->backBuffer->oledData[firstPage * OLED_WIDTH],
                         data, (uint16_t)(pageCount * OLED_WIDTH));
        oled_end_back_write(self, previous);
    }
    return OLED_OK;
}

static uint8_t oled_exchange_buffers(OLED *self)
{
    if (self->exchangeBuffer == 0U || self->isWritingBackBuffer != 0U) {
        /* 不清除请求、不等待；后台仍在写就继续使用原前台，下一帧完成后再试。 */
        return 0U;
    }
    __asm__ volatile ("" ::: "memory");
    oledBuffer *previousFront = self->frontBuffer;
    self->frontBuffer = self->backBuffer;
    self->backBuffer = previousFront;
    /* 新后台继承本帧，下一次仅修改局部也不会混入上一轮的旧内容。 */
    oled_copy_pixels(self->backBuffer->oledData, self->frontBuffer->oledData, OLED_BUFFER_SIZE);
    __asm__ volatile ("" ::: "memory");
    self->exchangeBuffer = 0U;
    return 1U;
}

static OLED_STATUS oled_start_frame(OLED *self)
{
    OLED_STATUS result = oled_send_window(self, 0U, 0U);
    if (result == OLED_OK) {
        result = oled_transmit(self, 0x40U, self->frontBuffer->oledData, OLED_BUFFER_SIZE);
    }
    if (result != OLED_OK) {
        self->status = (uint8_t)result;
        self->isFlushing = 0U;
    }
    return result;
}

OLED_STATUS oled_TransmitCallBack(OLED *self)
{
    if (self->isTransmitting != 0U) {
        return OLED_BUSY; /* 尚未释放 DMA 所有权，禁止提前交换。 */
    }
    if (self->status != OLED_OK) {
        self->isFlushing = 0U;
        return (OLED_STATUS)self->status;
    }
    if (self->isFlushing == 0U) {
        return OLED_OK; /* 独立 WriteData 完成，不进入整屏循环。 */
    }
    uint8_t exchanged = oled_exchange_buffers(self);
    if (self->nonstopFlush != 0U || self->exchangeBuffer != 0U || exchanged != 0U) {
        /* 后台忙时重发旧前台；刚交换的新帧至少发送一次，即使已关闭连续模式。 */
        return oled_start_frame(self);
    }
    self->isFlushing = 0U;
    return OLED_OK;
}

OLED_STATUS oled_Flush(OLED *self)
{
    /* 先登记请求再检查活动状态，避免中断恰在检查之后结束，导致提交无人处理。 */
    self->exchangeBuffer = 1U;
    __asm__ volatile ("" ::: "memory");
    if (self->isFlushing != 0U) {
        return OLED_OK;
    }
    if (self->isTransmitting != 0U) {
        return OLED_BUSY;
    }
    /* 只在空闲时比较指针，避免 DMA 回调恰好交换两指针而读到跨帧的值。 */
    if (self->backBuffer == NULL || self->frontBuffer == NULL ||
        self->backBuffer == self->frontBuffer || self->transmit == NULL ||
        self->transmitCallBack == NULL) {
        return OLED_INVALID_ARGUMENT;
    }
    self->isFlushing = 1U;
    oled_exchange_buffers(self); /* 仅空闲启动时提交首帧，DMA 此时尚未持有缓冲。 */
    return oled_start_frame(self);
}
