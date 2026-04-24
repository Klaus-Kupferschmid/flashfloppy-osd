/*
 * usb_dfu.c
 *
 * USB DFU (Device Firmware Upgrade) implementation for STM32F103.
 * Implements USB 1.1 Full-Speed with DFU class.
 *
 * Based on STM32F103 USB peripheral (USB_FS).
 */

#include <stdint.h>
#include <string.h>

/* Import from bootloader.c */
typedef enum {
    LED_DFU_READY,      /* Heartbeat: 50-50-50-850ms */
    LED_USB_CONNECTED,  /* Slow: 500ms on/off */
    LED_FLASHING,       /* Fast: 50ms on/off */
    LED_SUCCESS,        /* Solid 3s then reset */
    LED_ERROR           /* 3x fast, 1s pause */
} led_state_t;

extern led_state_t led_state;

/* USB peripheral registers */
#define USB_BASE        0x40005C00
#define USB_PMA_BASE    0x40006000

#define USB_EP0R        (*(volatile uint32_t *)(USB_BASE + 0x00))
#define USB_CNTR        (*(volatile uint32_t *)(USB_BASE + 0x40))
#define USB_ISTR        (*(volatile uint32_t *)(USB_BASE + 0x44))
#define USB_FNR         (*(volatile uint32_t *)(USB_BASE + 0x48))
#define USB_DADDR       (*(volatile uint32_t *)(USB_BASE + 0x4C))
#define USB_BTABLE      (*(volatile uint32_t *)(USB_BASE + 0x50))

/* EPnR register bits */
#define USB_EP_CTR_RX       (1 << 15)
#define USB_EP_DTOG_RX      (1 << 14)
#define USB_EP_STAT_RX_MASK (3 << 12)
#define USB_EP_STAT_RX_DIS  (0 << 12)
#define USB_EP_STAT_RX_STALL (1 << 12)
#define USB_EP_STAT_RX_NAK  (2 << 12)
#define USB_EP_STAT_RX_VALID (3 << 12)
#define USB_EP_SETUP        (1 << 11)
#define USB_EP_TYPE_MASK    (3 << 9)
#define USB_EP_TYPE_BULK    (0 << 9)
#define USB_EP_TYPE_CONTROL (1 << 9)
#define USB_EP_TYPE_ISO     (2 << 9)
#define USB_EP_TYPE_INT     (3 << 9)
#define USB_EP_KIND         (1 << 8)
#define USB_EP_CTR_TX       (1 << 7)
#define USB_EP_DTOG_TX      (1 << 6)
#define USB_EP_STAT_TX_MASK (3 << 4)
#define USB_EP_STAT_TX_DIS  (0 << 4)
#define USB_EP_STAT_TX_STALL (1 << 4)
#define USB_EP_STAT_TX_NAK  (2 << 4)
#define USB_EP_STAT_TX_VALID (3 << 4)
#define USB_EP_EA_MASK      0x0F

/* CNTR register bits */
#define USB_CNTR_CTRM       (1 << 15)
#define USB_CNTR_RESETM     (1 << 10)
#define USB_CNTR_SUSPM      (1 << 11)
#define USB_CNTR_WKUPM      (1 << 12)
#define USB_CNTR_FRES       (1 << 0)
#define USB_CNTR_PDWN       (1 << 1)

/* ISTR register bits */
#define USB_ISTR_CTR        (1 << 15)
#define USB_ISTR_RESET      (1 << 10)
#define USB_ISTR_SUSP       (1 << 11)
#define USB_ISTR_WKUP       (1 << 12)
#define USB_ISTR_EP_ID_MASK 0x0F
#define USB_ISTR_DIR        (1 << 4)

/* DADDR bits */
#define USB_DADDR_EF        (1 << 7)

/* PMA buffer descriptor table entry
 * STM32F103 USB PMA: 512 bytes accessible via APB at 0x40006000
 * Each 16-bit PMA word occupies 32 bits in CPU address space (upper 16 bits reserved)
 * BTABLE entries contain PMA byte offsets, NOT CPU addresses */
typedef struct {
    volatile uint32_t addr_tx;   /* TX buffer PMA offset (bits 15:1, bit 0 must be 0) */
    volatile uint32_t count_tx;  /* TX byte count (bits 9:0) */
    volatile uint32_t addr_rx;   /* RX buffer PMA offset */
    volatile uint32_t count_rx;  /* RX config (15:10) + received count (9:0) */
} pma_entry_t;

/* count_rx config: BL_SIZE=1 (32-byte blocks), NUM_BLOCK=2 -> 64 bytes max */
#define RX_COUNT_64_BYTES  ((1 << 15) | (2 << 10))

/* PMA layout - BTABLE at offset 0, buffers follow
 * These are PMA offsets (each PMA byte = 2 CPU bytes due to 32-bit spacing) */
#define PMA_BTABLE_OFFSET   0     /* BTABLE at start of PMA */
#define PMA_EP0_TX_OFFSET   64    /* After BTABLE (8 entries * 8 bytes) */
#define PMA_EP0_RX_OFFSET   128   /* After EP0 TX buffer (64 bytes) */

/* Access BTABLE entry for endpoint - BTABLE entry is 16 CPU bytes (4 x uint32) */
#define PMA_ENTRY(ep) ((pma_entry_t *)(USB_PMA_BASE + (ep) * 16))

/* PMA memory layout (PMA offsets in left column, CPU address offset = PMA * 2):
 * PMA 0x00 - 0x3F (CPU 0x00-0x7F): Buffer descriptor table (8 eps max)
 * PMA 0x40 - 0x7F (CPU 0x80-0xFF): EP0 TX buffer (64 bytes)
 * PMA 0x80 - 0xBF (CPU 0x100-0x17F): EP0 RX buffer (64 bytes)
 */

/* USB standard request types */
#define USB_REQ_TYPE_MASK       0x60
#define USB_REQ_TYPE_STANDARD   0x00
#define USB_REQ_TYPE_CLASS      0x20
#define USB_REQ_TYPE_VENDOR     0x40

#define USB_REQ_RECIP_MASK      0x1F
#define USB_REQ_RECIP_DEVICE    0x00
#define USB_REQ_RECIP_INTERFACE 0x01
#define USB_REQ_RECIP_ENDPOINT  0x02

/* USB standard requests */
#define USB_REQ_GET_STATUS      0x00
#define USB_REQ_CLEAR_FEATURE   0x01
#define USB_REQ_SET_FEATURE     0x03
#define USB_REQ_SET_ADDRESS     0x05
#define USB_REQ_GET_DESCRIPTOR  0x06
#define USB_REQ_SET_DESCRIPTOR  0x07
#define USB_REQ_GET_CONFIG      0x08
#define USB_REQ_SET_CONFIG      0x09
#define USB_REQ_GET_INTERFACE   0x0A
#define USB_REQ_SET_INTERFACE   0x0B

/* Descriptor types */
#define USB_DESC_TYPE_DEVICE        0x01
#define USB_DESC_TYPE_CONFIG        0x02
#define USB_DESC_TYPE_STRING        0x03
#define USB_DESC_TYPE_INTERFACE     0x04
#define USB_DESC_TYPE_ENDPOINT      0x05
#define USB_DESC_TYPE_DFU_FUNCTIONAL 0x21

/* DFU class requests */
#define DFU_DETACH      0x00
#define DFU_DNLOAD      0x01
#define DFU_UPLOAD      0x02
#define DFU_GETSTATUS   0x03
#define DFU_CLRSTATUS   0x04
#define DFU_GETSTATE    0x05
#define DFU_ABORT       0x06

/* DFU states */
#define DFU_STATE_APP_IDLE              0
#define DFU_STATE_APP_DETACH            1
#define DFU_STATE_DFU_IDLE              2
#define DFU_STATE_DFU_DNLOAD_SYNC       3
#define DFU_STATE_DFU_DNBUSY            4
#define DFU_STATE_DFU_DNLOAD_IDLE       5
#define DFU_STATE_DFU_MANIFEST_SYNC     6
#define DFU_STATE_DFU_MANIFEST          7
#define DFU_STATE_DFU_MANIFEST_WAIT_RST 8
#define DFU_STATE_DFU_UPLOAD_IDLE       9
#define DFU_STATE_DFU_ERROR             10

/* DFU status codes */
#define DFU_STATUS_OK               0x00
#define DFU_STATUS_ERR_TARGET       0x01
#define DFU_STATUS_ERR_FILE         0x02
#define DFU_STATUS_ERR_WRITE        0x03
#define DFU_STATUS_ERR_ERASE        0x04
#define DFU_STATUS_ERR_CHECK_ERASED 0x05
#define DFU_STATUS_ERR_PROG         0x06
#define DFU_STATUS_ERR_VERIFY       0x07
#define DFU_STATUS_ERR_ADDRESS      0x08
#define DFU_STATUS_ERR_NOTDONE      0x09
#define DFU_STATUS_ERR_FIRMWARE     0x0A
#define DFU_STATUS_ERR_VENDOR       0x0B
#define DFU_STATUS_ERR_USBR         0x0C
#define DFU_STATUS_ERR_POR          0x0D
#define DFU_STATUS_ERR_UNKNOWN      0x0E
#define DFU_STATUS_ERR_STALLEDPKT   0x0F

/* Application address and size */
#define APP_ADDRESS     0x08002000
#define APP_MAX_SIZE    (128*1024 - 8*1024 - 2*1024)  /* 128KB - 8KB bootloader - 2KB storage */
#define FLASH_PAGE_SIZE 1024

/* Flash programming */
#define FLASH_BASE      0x40022000
#define FLASH_KEYR      (*(volatile uint32_t *)(FLASH_BASE + 0x04))
#define FLASH_SR        (*(volatile uint32_t *)(FLASH_BASE + 0x0C))
#define FLASH_CR        (*(volatile uint32_t *)(FLASH_BASE + 0x10))
#define FLASH_AR        (*(volatile uint32_t *)(FLASH_BASE + 0x14))

#define FLASH_SR_BSY    (1 << 0)
#define FLASH_SR_EOP    (1 << 5)
#define FLASH_CR_PG     (1 << 0)
#define FLASH_CR_PER    (1 << 1)
#define FLASH_CR_STRT   (1 << 6)
#define FLASH_CR_LOCK   (1 << 7)
#define FLASH_KEY1      0x45670123
#define FLASH_KEY2      0xCDEF89AB

/* USB setup packet */
typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_t;

/* State */
static uint8_t usb_address;
static uint8_t usb_configured;
static uint8_t dfu_state = DFU_STATE_DFU_IDLE;
static uint8_t dfu_status = DFU_STATUS_OK;
static uint16_t dfu_block_num;
static uint8_t dfu_buffer[1024];  /* One flash page */
static uint16_t dfu_buffer_len;

/* Device descriptor */
static const uint8_t device_desc[] = {
    18,                 /* bLength */
    USB_DESC_TYPE_DEVICE, /* bDescriptorType */
    0x00, 0x02,         /* bcdUSB = 2.00 */
    0x00,               /* bDeviceClass (defined at interface level) */
    0x00,               /* bDeviceSubClass */
    0x00,               /* bDeviceProtocol */
    64,                 /* bMaxPacketSize0 */
    0x83, 0x04,         /* idVendor = 0x0483 (STMicroelectronics) */
    0x11, 0xDF,         /* idProduct = 0xDF11 (DFU) */
    0x00, 0x01,         /* bcdDevice = 1.00 */
    1,                  /* iManufacturer */
    2,                  /* iProduct */
    3,                  /* iSerialNumber */
    1                   /* bNumConfigurations */
};

/* Configuration descriptor (includes interface and DFU functional) */
static const uint8_t config_desc[] = {
    /* Configuration descriptor */
    9,                  /* bLength */
    USB_DESC_TYPE_CONFIG, /* bDescriptorType */
    27, 0,              /* wTotalLength = 27 */
    1,                  /* bNumInterfaces */
    1,                  /* bConfigurationValue */
    0,                  /* iConfiguration */
    0x80,               /* bmAttributes (bus powered) */
    50,                 /* bMaxPower (100mA) */
    
    /* Interface descriptor */
    9,                  /* bLength */
    USB_DESC_TYPE_INTERFACE, /* bDescriptorType */
    0,                  /* bInterfaceNumber */
    0,                  /* bAlternateSetting */
    0,                  /* bNumEndpoints */
    0xFE,               /* bInterfaceClass (Application Specific) */
    0x01,               /* bInterfaceSubClass (DFU) */
    0x02,               /* bInterfaceProtocol (DFU mode) */
    4,                  /* iInterface */
    
    /* DFU Functional descriptor */
    9,                  /* bLength */
    USB_DESC_TYPE_DFU_FUNCTIONAL, /* bDescriptorType */
    0x0B,               /* bmAttributes (download, upload, manifest tolerant) */
    0xFF, 0x00,         /* wDetachTimeout (255ms) */
    0x00, 0x04,         /* wTransferSize (1024 bytes) */
    0x1A, 0x01          /* bcdDFUVersion (1.1a) */
};

/* String descriptors */
static const uint8_t string_desc_0[] = { 4, USB_DESC_TYPE_STRING, 0x09, 0x04 }; /* LANGID = US English */
static const uint8_t string_desc_1[] = { 28, USB_DESC_TYPE_STRING, 
    'F',0,'l',0,'a',0,'s',0,'h',0,'F',0,'l',0,'o',0,'p',0,'p',0,'y',0,'-',0,'O',0 };
static const uint8_t string_desc_2[] = { 32, USB_DESC_TYPE_STRING,
    'D',0,'F',0,'U',0,' ',0,'B',0,'o',0,'o',0,'t',0,'l',0,'o',0,'a',0,'d',0,'e',0,'r',0,0,0 };
static const uint8_t string_desc_3[] = { 10, USB_DESC_TYPE_STRING,
    '1',0,'.',0,'0',0,'0',0 };
static const uint8_t string_desc_4[] = { 26, USB_DESC_TYPE_STRING,
    '@',0,'F',0,'l',0,'a',0,'s',0,'h',0,' ',0,'/',0,'0',0,'x',0,'0',0,'8',0 };

static const uint8_t *string_descs[] = {
    string_desc_0, string_desc_1, string_desc_2, string_desc_3, string_desc_4
};

/* Copy to PMA (Packet Memory Area) */
static void pma_write(uint16_t offset, const void *data, uint16_t len)
{
    volatile uint16_t *dst = (volatile uint16_t *)(USB_PMA_BASE + offset * 2);
    const uint8_t *src = (const uint8_t *)data;
    
    for (uint16_t i = 0; i < len; i += 2) {
        uint16_t val = src[i];
        if (i + 1 < len) val |= (src[i + 1] << 8);
        *dst++ = val;
        dst++;  /* Skip reserved word */
    }
}

/* Copy from PMA */
static void pma_read(uint16_t offset, void *data, uint16_t len)
{
    volatile uint16_t *src = (volatile uint16_t *)(USB_PMA_BASE + offset * 2);
    uint8_t *dst = (uint8_t *)data;
    
    for (uint16_t i = 0; i < len; i += 2) {
        uint16_t val = *src++;
        src++;  /* Skip reserved word */
        dst[i] = val & 0xFF;
        if (i + 1 < len) dst[i + 1] = (val >> 8) & 0xFF;
    }
}

/* Set EP0 TX status (handles toggle bits correctly)
 * STM32 USB EP register bits:
 * - CTR_RX, CTR_TX (bits 15, 7): rc_w0 - write 0 to clear, write 1 to keep
 * - DTOG_RX, DTOG_TX (bits 14, 6): toggle on write 1
 * - STAT_RX, STAT_TX (bits 13:12, 5:4): toggle on write 1
 * - EP_TYPE, EP_KIND, EA: normal read/write
 */
static void ep0_set_tx_status(uint16_t status)
{
    uint16_t epr = USB_EP0R;
    /* Calculate which bits need to toggle to reach desired status */
    uint16_t toggle = (epr & USB_EP_STAT_TX_MASK) ^ status;
    /* Build write value:
     * - Keep invariant rw bits (type, kind, address)
     * - Set CTR bits to 1 (don't accidentally clear them)
     * - Set DTOG bits to 0 (don't toggle data toggle)
     * - Set calculated toggle bits for STAT */
    epr = (epr & (USB_EP_TYPE_MASK | USB_EP_KIND | USB_EP_EA_MASK))
        | USB_EP_CTR_RX | USB_EP_CTR_TX
        | toggle;
    USB_EP0R = epr;
}

/* Set EP0 RX status */
static void ep0_set_rx_status(uint16_t status)
{
    uint16_t epr = USB_EP0R;
    uint16_t toggle = (epr & USB_EP_STAT_RX_MASK) ^ status;
    epr = (epr & (USB_EP_TYPE_MASK | USB_EP_KIND | USB_EP_EA_MASK))
        | USB_EP_CTR_RX | USB_EP_CTR_TX
        | toggle;
    USB_EP0R = epr;
}

/* Send data on EP0 */
static void ep0_send(const void *data, uint16_t len)
{
    pma_write(PMA_EP0_TX_OFFSET, data, len);
    PMA_ENTRY(0)->count_tx = len;
    ep0_set_tx_status(USB_EP_STAT_TX_VALID);
}

/* Send zero-length packet */
static void ep0_send_zlp(void)
{
    PMA_ENTRY(0)->count_tx = 0;
    ep0_set_tx_status(USB_EP_STAT_TX_VALID);
}

/* Stall EP0 */
static void ep0_stall(void)
{
    ep0_set_tx_status(USB_EP_STAT_TX_STALL);
    ep0_set_rx_status(USB_EP_STAT_RX_STALL);
}

/* Flash unlock */
static void flash_unlock(void)
{
    if (FLASH_CR & FLASH_CR_LOCK) {
        FLASH_KEYR = FLASH_KEY1;
        FLASH_KEYR = FLASH_KEY2;
    }
}

/* Flash lock */
static void flash_lock(void)
{
    FLASH_CR |= FLASH_CR_LOCK;
}

/* Erase flash page */
static int flash_erase_page(uint32_t address)
{
    flash_unlock();
    
    /* Wait for flash not busy */
    while (FLASH_SR & FLASH_SR_BSY);
    
    /* Set page erase */
    FLASH_CR |= FLASH_CR_PER;
    FLASH_AR = address;
    FLASH_CR |= FLASH_CR_STRT;
    
    /* Wait for completion */
    while (FLASH_SR & FLASH_SR_BSY);
    
    /* Clear PER bit */
    FLASH_CR &= ~FLASH_CR_PER;
    
    /* Check for errors */
    if (FLASH_SR & 0x14) {  /* WRPRTERR or PGERR */
        FLASH_SR = 0x14;    /* Clear errors */
        flash_lock();
        return -1;
    }
    
    flash_lock();
    return 0;
}

/* Program flash (halfword at a time) */
static int flash_program(uint32_t address, const uint8_t *data, uint16_t len)
{
    flash_unlock();
    
    for (uint16_t i = 0; i < len; i += 2) {
        /* Wait for flash not busy */
        while (FLASH_SR & FLASH_SR_BSY);
        
        /* Set programming mode */
        FLASH_CR |= FLASH_CR_PG;
        
        /* Write halfword */
        uint16_t val = data[i];
        if (i + 1 < len) val |= (data[i + 1] << 8);
        else val |= 0xFF00;  /* Pad with 0xFF */
        
        *(volatile uint16_t *)(address + i) = val;
        
        /* Wait for completion */
        while (FLASH_SR & FLASH_SR_BSY);
        
        /* Clear PG bit */
        FLASH_CR &= ~FLASH_CR_PG;
        
        /* Verify */
        if (*(volatile uint16_t *)(address + i) != val) {
            flash_lock();
            return -1;
        }
    }
    
    flash_lock();
    return 0;
}

/* Handle DFU class requests */
static void handle_dfu_request(usb_setup_t *setup)
{
    uint8_t status_resp[6];
    
    switch (setup->bRequest) {
    case DFU_GETSTATUS:
        status_resp[0] = dfu_status;           /* bStatus */
        status_resp[1] = 10;                   /* bwPollTimeout (10ms) */
        status_resp[2] = 0;
        status_resp[3] = 0;
        status_resp[4] = dfu_state;            /* bState */
        status_resp[5] = 0;                    /* iString */
        ep0_send(status_resp, 6);
        
        /* State transitions after GETSTATUS */
        if (dfu_state == DFU_STATE_DFU_DNLOAD_SYNC) {
            dfu_state = DFU_STATE_DFU_DNBUSY;
        } else if (dfu_state == DFU_STATE_DFU_DNBUSY) {
            /* Program the data */
            led_state = LED_FLASHING;
            
            if (dfu_buffer_len > 0) {
                uint32_t addr = APP_ADDRESS + (dfu_block_num * 1024);
                
                /* Erase page if at page boundary */
                if ((addr & (FLASH_PAGE_SIZE - 1)) == 0) {
                    if (flash_erase_page(addr) < 0) {
                        dfu_state = DFU_STATE_DFU_ERROR;
                        dfu_status = DFU_STATUS_ERR_ERASE;
                        led_state = LED_ERROR;
                        break;
                    }
                }
                
                /* Program data */
                if (flash_program(addr, dfu_buffer, dfu_buffer_len) < 0) {
                    dfu_state = DFU_STATE_DFU_ERROR;
                    dfu_status = DFU_STATUS_ERR_PROG;
                    led_state = LED_ERROR;
                    break;
                }
            }
            dfu_state = DFU_STATE_DFU_DNLOAD_IDLE;
        } else if (dfu_state == DFU_STATE_DFU_MANIFEST_SYNC) {
            dfu_state = DFU_STATE_DFU_MANIFEST;
        } else if (dfu_state == DFU_STATE_DFU_MANIFEST) {
            /* Download complete */
            led_state = LED_SUCCESS;
            dfu_state = DFU_STATE_DFU_MANIFEST_WAIT_RST;
        }
        break;
        
    case DFU_GETSTATE:
        ep0_send(&dfu_state, 1);
        break;
        
    case DFU_CLRSTATUS:
        dfu_state = DFU_STATE_DFU_IDLE;
        dfu_status = DFU_STATUS_OK;
        ep0_send_zlp();
        break;
        
    case DFU_ABORT:
        dfu_state = DFU_STATE_DFU_IDLE;
        ep0_send_zlp();
        break;
        
    case DFU_DNLOAD:
        if (setup->wLength == 0) {
            /* End of download */
            dfu_state = DFU_STATE_DFU_MANIFEST_SYNC;
            ep0_send_zlp();
        } else {
            /* Prepare to receive data */
            dfu_block_num = setup->wValue;
            dfu_buffer_len = setup->wLength;
            dfu_state = DFU_STATE_DFU_DNLOAD_SYNC;
            ep0_set_rx_status(USB_EP_STAT_RX_VALID);
        }
        break;
        
    case DFU_UPLOAD:
        /* Upload (read from device) - optional, not implemented */
        ep0_stall();
        break;
        
    case DFU_DETACH:
        /* Detach - trigger reset */
        dfu_state = DFU_STATE_APP_DETACH;
        ep0_send_zlp();
        break;
        
    default:
        ep0_stall();
        break;
    }
}

/* Handle standard requests */
static void handle_standard_request(usb_setup_t *setup)
{
    uint8_t desc_type = setup->wValue >> 8;
    uint8_t desc_index = setup->wValue & 0xFF;
    uint16_t len;
    
    switch (setup->bRequest) {
    case USB_REQ_GET_DESCRIPTOR:
        switch (desc_type) {
        case USB_DESC_TYPE_DEVICE:
            len = sizeof(device_desc);
            if (len > setup->wLength) len = setup->wLength;
            ep0_send(device_desc, len);
            break;
            
        case USB_DESC_TYPE_CONFIG:
            len = sizeof(config_desc);
            if (len > setup->wLength) len = setup->wLength;
            ep0_send(config_desc, len);
            break;
            
        case USB_DESC_TYPE_STRING:
            if (desc_index < sizeof(string_descs)/sizeof(string_descs[0])) {
                len = string_descs[desc_index][0];
                if (len > setup->wLength) len = setup->wLength;
                ep0_send(string_descs[desc_index], len);
            } else {
                ep0_stall();
            }
            break;
            
        default:
            ep0_stall();
            break;
        }
        break;
        
    case USB_REQ_SET_ADDRESS:
        usb_address = setup->wValue & 0x7F;
        ep0_send_zlp();
        /* Address will be set after status stage */
        break;
        
    case USB_REQ_SET_CONFIG:
        usb_configured = setup->wValue;
        led_state = LED_USB_CONNECTED;
        ep0_send_zlp();
        break;
        
    case USB_REQ_GET_CONFIG:
        ep0_send(&usb_configured, 1);
        break;
        
    case USB_REQ_GET_STATUS:
        {
            uint16_t status = 0;
            ep0_send(&status, 2);
        }
        break;
        
    default:
        ep0_stall();
        break;
    }
}

/* Handle SETUP packet */
static void handle_setup(void)
{
    usb_setup_t setup;
    pma_read(PMA_EP0_RX_OFFSET, &setup, 8);
    
    if ((setup.bmRequestType & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_CLASS) {
        handle_dfu_request(&setup);
    } else {
        handle_standard_request(&setup);
    }
}

/* Handle EP0 data received (for DFU DNLOAD) */
static void handle_ep0_rx(void)
{
    uint16_t count = PMA_ENTRY(0)->count_rx & 0x3FF;
    
    if (dfu_state == DFU_STATE_DFU_DNLOAD_SYNC && count > 0) {
        pma_read(PMA_EP0_RX_OFFSET, dfu_buffer, count);
        dfu_buffer_len = count;
        ep0_send_zlp();  /* Acknowledge */
    }
    
    ep0_set_rx_status(USB_EP_STAT_RX_VALID);
}

/* Handle EP0 TX complete */
static void handle_ep0_tx(void)
{
    /* Set address after SET_ADDRESS status stage */
    if (usb_address != 0 && (USB_DADDR & 0x7F) == 0) {
        USB_DADDR = USB_DADDR_EF | usb_address;
    }
    
    ep0_set_rx_status(USB_EP_STAT_RX_VALID);
}

/* USB reset handler */
static void handle_reset(void)
{
    /* Set buffer table address */
    USB_BTABLE = PMA_BTABLE_OFFSET;
    
    /* Configure EP0 buffer descriptors in PMA */
    PMA_ENTRY(0)->addr_tx = PMA_EP0_TX_OFFSET;
    PMA_ENTRY(0)->count_tx = 0;
    PMA_ENTRY(0)->addr_rx = PMA_EP0_RX_OFFSET;
    /* COUNT_RX format: BL_SIZE (bit 15), NUM_BLOCK (bits 14:10)
     * BL_SIZE=1 means 32-byte blocks, NUM_BLOCK=2 gives 64 bytes */
    PMA_ENTRY(0)->count_rx = (1 << 15) | (2 << 10);  /* 64 bytes */
    
    /* EP0: Control endpoint
     * Clear all toggle bits and set type/address */
    USB_EP0R = USB_EP_TYPE_CONTROL | 0;  /* EA = 0, clears toggles */
    
    /* Set endpoint status */
    ep0_set_tx_status(USB_EP_STAT_TX_NAK);
    ep0_set_rx_status(USB_EP_STAT_RX_VALID);
    
    /* Enable device */
    USB_DADDR = USB_DADDR_EF;
    
    /* Reset state */
    usb_address = 0;
    usb_configured = 0;
    dfu_state = DFU_STATE_DFU_IDLE;
    dfu_status = DFU_STATUS_OK;
}

/* Initialize USB */
void usb_dfu_init(void)
{
    /* RCC register for USB peripheral reset */
    #define RCC_APB1RSTR    (*(volatile uint32_t *)0x40021010)
    #define RCC_APB1RSTR_USBRST (1 << 23)
    
    /* Step 0: Reset USB peripheral via RCC */
    RCC_APB1RSTR |= RCC_APB1RSTR_USBRST;
    for (volatile int i = 0; i < 10; i++);
    RCC_APB1RSTR &= ~RCC_APB1RSTR_USBRST;
    
    /* Step 1: Force USB reset, USB transceiver powered down */
    USB_CNTR = USB_CNTR_FRES | USB_CNTR_PDWN;
    
    /* Step 2: Wait before clearing power down (minimum 1us) */
    for (volatile int i = 0; i < 100; i++);
    
    /* Step 3: Clear PDWN to enable USB transceiver, keep FRES */
    USB_CNTR = USB_CNTR_FRES;
    
    /* Step 4: Wait for USB transceiver startup (tSTARTUP ~1us) */
    for (volatile int i = 0; i < 100; i++);
    
    /* Step 5: Clear FRES to release USB from reset */
    USB_CNTR = 0;
    
    /* Step 6: Clear any pending interrupts */
    USB_ISTR = 0;
    
    /* Step 7: Initialize buffer table address */
    USB_BTABLE = PMA_BTABLE_OFFSET;
    
    /* Step 8: Enable device function and set address 0 */
    USB_DADDR = USB_DADDR_EF;
    
    /* Step 9: Wait for USB to stabilize */
    for (volatile int i = 0; i < 1000; i++);
    
    /* Step 10: Enable reset and transfer complete interrupts (polled mode) */
    USB_CNTR = USB_CNTR_RESETM | USB_CNTR_CTRM;
    
    /* The USB host will detect the D+ pull-up and send a reset,
     * which will be handled by usb_dfu_poll() -> handle_reset() */
}

/* Poll USB */
void usb_dfu_poll(void)
{
    uint16_t istr = USB_ISTR;
    
    /* USB Reset */
    if (istr & USB_ISTR_RESET) {
        USB_ISTR = ~USB_ISTR_RESET;
        handle_reset();
        return;
    }
    
    /* Correct transfer */
    if (istr & USB_ISTR_CTR) {
        uint8_t ep = istr & USB_ISTR_EP_ID_MASK;
        
        if (ep == 0) {
            uint16_t epr = USB_EP0R;
            
            if (epr & USB_EP_CTR_RX) {
                /* Clear CTR_RX only.
                 * STM32 USB EPnR register rules:
                 * - CTR bits (15,7): rc_w0 - write 0 to clear, 1 to keep
                 * - DTOG bits (14,6): toggle - write 1 to toggle, 0 to keep
                 * - STAT bits (13:12,5:4): toggle - write 1 to toggle, 0 to keep
                 * - TYPE/KIND/EA: normal r/w
                 * To keep STAT/DTOG unchanged, write 0. To keep CTR, write 1. */
                USB_EP0R = (epr & (USB_EP_TYPE_MASK | USB_EP_KIND | USB_EP_EA_MASK))
                         | USB_EP_CTR_TX;  /* Write 0 to CTR_RX (clears), 1 to CTR_TX (keeps) */
                
                if (epr & USB_EP_SETUP) {
                    handle_setup();
                } else {
                    handle_ep0_rx();
                }
            }
            
            if (epr & USB_EP_CTR_TX) {
                /* Clear CTR_TX only */
                USB_EP0R = (epr & (USB_EP_TYPE_MASK | USB_EP_KIND | USB_EP_EA_MASK))
                         | USB_EP_CTR_RX;  /* Write 0 to CTR_TX (clears), 1 to CTR_RX (keeps) */
                handle_ep0_tx();
            }
        }
    }
}
