#ifndef DT_BINDINGS_STM32_SRAM_H
#define DT_BINDINGS_STM32_SRAM_H

#define WRITE_BURST_DISABLE       0
#define WRITE_BURST_ENABLE        1

#define PAGE_SIZE_NONE            0
#define PAGE_SIZE_128             1
#define PAGE_SIZE_256             2
#define PAGE_SIZE_512             3
#define PAGE_SIZE_1024            4

#define ASYNCHRONOUS_WAIT_DISABLE 0
#define ASYNCHRONOUS_WAIT_ENABLE  1

#define EXTENDED_MODE_DISNABLE    0
#define EXTENDED_MODE_ENABLE      1

#define WAIT_SIGNAL_DISABLE       0
#define WAIT_SIGNAL_ENABLE        1

#define WRITE_OPERATION_DISABLE   0
#define WRITE_OPERATION_ENABLE    1

#define WAIT_TIMING_BEFORE_WS     0
#define WAIT_TIMING_DURING_WS     1

#define WRAP_MODE_DISABLE         0
#define WRAP_MODE_ENABLE          1

#define WAIT_SIGNAL_POLARITY_LOW  0
#define WAIT_SIGNAL_POLARITY_HIGH 1

#define BURST_ACCESS_MODE_DISABLE 0
#define BURST_ACCESS_MODE_ENABLE  1

#define NORSRAM_MEM_BUS_WIDTH_8   0
#define NORSRAM_MEM_BUS_WIDTH_16  1
#define NORSRAM_MEM_BUS_WIDTH_32  2

#define MEMORY_TYPE_SRAM          0
#define MEMORY_TYPE_PSRAM         1
#define MEMORY_TYPE_NOR           2

#define DATA_ADDRESS_MUX_DISABLE  0
#define DATA_ADDRESS_MUX_ENABLE   1

#define ACCESS_MODE_A             0
#define ACCESS_MODE_B             1
#define ACCESS_MODE_C             2
#define ACCESS_MODE_D             3

#endif
