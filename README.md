# RSOC-2024-Day4
本文章是基于RT-Thread暑期训练营的第四天所学的知识，进行的一些非完全总结！！！！
## I/O 设备模型
绝大部分的嵌入式系统都包括一些 I/O（Input/Output，输入 / 输出）设备，例如仪器上的数据显示屏、工业设备上的串口通信、数据采集设备上用于保存数据的 Flash 或 SD 卡，以及网络设备的以太网接口等，都是嵌入式系统中容易找到的 I/O 设备例子。  
应用程序通过 I/O 设备管理接口获得正确的设备驱动，然后通过这个设备驱动与底层 I/O 硬件设备进行数据（或控制）交互。

I/O 设备管理层实现了对设备驱动程序的封装。应用程序通过图中的"I/O设备管理层"提供的标准接口访问底层设备，设备驱动程序的升级、更替不会对上层应用产生影响。这种方式使得设备的硬件操作相关的代码能够独立于应用程序而存在，双方只需关注各自的功能实现，从而降低了代码的耦合性、复杂性，提高了系统的可靠性。

设备驱动框架层是对同类硬件设备驱动的抽象，将不同厂家的同类硬件设备驱动中相同的部分抽取出来，将不同部分留出接口，由驱动程序实现。

设备驱动层是一组驱使硬件设备工作的程序，实现访问硬件设备的功能。它负责创建和注册 I/O 设备，对于操作逻辑简单的设备，可以不经过设备驱动框架层，直接将设备注册到 I/O 设备管理器中，使用序列图如下图所示，主要有以下 2 点：

设备驱动根据设备模型定义，创建出具备硬件访问能力的设备实例，将该设备通过 ```rt_device_register() ```接口注册到 I/O 设备管理器中。

应用程序通过 ```rt_device_find() ```接口查找到设备，然后使用 I/O 设备管理接口来访问硬件。  
```
struct rt_device
{
    struct rt_object          parent;        /* 内核对象基类 */
    enum rt_device_class_type type;          /* 设备类型 */
    rt_uint16_t               flag;          /* 设备参数 */
    rt_uint16_t               open_flag;     /* 设备打开标志 */
    rt_uint8_t                ref_count;     /* 设备被引用次数 */
    rt_uint8_t                device_id;     /* 设备 ID,0 - 255 */

    /* 数据收发回调函数 */
    rt_err_t (*rx_indicate)(rt_device_t dev, rt_size_t size);
    rt_err_t (*tx_complete)(rt_device_t dev, void *buffer);

    const struct rt_device_ops *ops;    /* 设备操作方法 */

    /* 设备的私有数据 */
    void *user_data;
};
typedef struct rt_device *rt_device_t;
```
## I/O 设备类型
RT-Thread 支持多种 I/O 设备类型，主要设备类型如下所示：  
```RT_Device_Class_Char             /* 字符设备       */
RT_Device_Class_Block            /* 块设备         */
RT_Device_Class_NetIf            /* 网络接口设备    */
RT_Device_Class_MTD              /* 内存设备       */
RT_Device_Class_RTC              /* RTC 设备        */
RT_Device_Class_Sound            /* 声音设备        */
RT_Device_Class_Graphic          /* 图形设备        */
RT_Device_Class_I2CBUS           /* I2C 总线设备     */
RT_Device_Class_USBDevice        /* USB device 设备  */
RT_Device_Class_USBHost          /* USB host 设备   */
RT_Device_Class_SPIBUS           /* SPI 总线设备     */
RT_Device_Class_SPIDevice        /* SPI 设备        */
RT_Device_Class_SDIO             /* SDIO 设备       */
RT_Device_Class_Miscellaneous    /* 杂类设备        */
```
当系统服务于一个具有大量数据的写操作时，设备驱动程序必须首先将数据划分为多个包，每个包采用设备指定的数据尺寸。而在实际过程中，最后一部分数据尺寸有可能小于正常的设备块尺寸。如上图中每个块使用单独的写请求写入到设备中，头 3 个直接进行写操作。但最后一个数据块尺寸小于设备块尺寸，设备驱动程序必须使用不同于前 3 个块的方式处理最后的数据块。通常情况下，设备驱动程序需要首先执行相对应的设备块的读操作，然后把写入数据覆盖到读出数据上，然后再把这个 “合成” 的数据块作为一整个块写回到设备中。例如上图中的块 4，驱动程序需要先把块 4 所对应的设备块读出来，然后将需要写入的数据覆盖至从设备块读出的数据上，使其合并成一个新的块，最后再写回到块设备中。  
## 创建和注册 I/O 设备
驱动层负责创建设备实例，并注册到 I/O 设备管理器中，可以通过静态申明的方式创建设备实例，也可以用下面的接口进行动态创建：  
```
rt_device_t rt_device_create(int type, int attach_size);
```
调用该接口时，系统会从动态堆内存中分配一个设备控制块，大小为 struct rt_device 和 attach_size 的和，设备的类型由参数 type 设定。设备被创建后，需要实现它访问硬件的操作方法。  
```
struct rt_device_ops
{
    /* common device interface */
    rt_err_t  (*init)   (rt_device_t dev);
    rt_err_t  (*open)   (rt_device_t dev, rt_uint16_t oflag);
    rt_err_t  (*close)  (rt_device_t dev);
    rt_size_t (*read)   (rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size);
    rt_size_t (*write)  (rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size);
    rt_err_t  (*control)(rt_device_t dev, int cmd, void *args);
};
```
## 访问 I/O 设备
应用程序通过 I/O 设备管理接口来访问硬件设备，当设备驱动实现后，应用程序就可以访问该硬件:  
### 查找设备
应用程序根据设备名称获取设备句柄，进而可以操作设备。查找设备函数如下所示：  
```
rt_device_t rt_device_find(const char* name);
```
初始化设备
获得设备句柄后，应用程序可使用如下函数对设备进行初始化操作：  
```
rt_err_t rt_device_init(rt_device_t dev);
```
### 打开和关闭设备 
通过设备句柄，应用程序可以打开和关闭设备，打开设备时，会检测设备是否已经初始化，没有初始化则会默认调用初始化接口初始化设备。通过如下函数打开设备：  
```
rt_err_t rt_device_open(rt_device_t dev, rt_uint16_t oflags);
```
# I2C 总线设备
I2C 简介
I2C（Inter Integrated Circuit）总线是 PHILIPS 公司开发的一种半双工、双向二线制同步串行总线。I2C 总线传输数据时只需两根信号线，一根是双向数据线 SDA（serial data），另一根是双向时钟线 SCL（serial clock）。SPI 总线有两根线分别用于主从设备之间接收数据和发送数据，而 I2C 总线只使用一根线进行数据收发。

I2C 和 SPI 一样以主从的方式工作，不同于 SPI 一主多从的结构，它允许同时有多个主设备存在，每个连接到总线上的器件都有唯一的地址，主设备启动数据传输并产生时钟信号，从设备被主设备寻址，同一时刻只允许有一个主设备。如下图所示：
![111](https://github.com/lqr0323/RSOC-2024-Day4/blob/main/i2c1.png)  
## 查找 I2C 总线设备
在使用 I2C 总线设备前需要根据 I2C 总线设备名称获取设备句柄，进而才可以操作 I2C 总线设备，查找设备函数如下所示:  
```
rt_device_t rt_device_find(const char* name);
```
```
#define AHT10_I2C_BUS_NAME      "i2c1"  /* 传感器连接的I2C总线设备名称 */
struct rt_i2c_bus_device *i2c_bus;      /* I2C总线设备句柄 */

/* 查找I2C总线设备，获取I2C总线设备句柄 */
i2c_bus = (struct rt_i2c_bus_device *)rt_device_find(name);
```
# SPI 设备
SPI 简介
SPI（Serial Peripheral Interface，串行外设接口）是一种高速、全双工、同步通信总线，常用于短距离通讯，主要应用于 EEPROM、FLASH、实时时钟、AD 转换器、还有数字信号处理器和数字信号解码器之间。
MOSI –主机输出 / 从机输入数据线（SPI Bus Master Output/Slave Input）。

MISO –主机输入 / 从机输出数据线（SPI Bus Master Input/Slave Output)。

SCLK –串行时钟线（Serial Clock），主设备输出时钟信号至从设备。

CS –从设备选择线 (Chip select)。也叫 SS、CSB、CSN、EN 等，主设备输出片选信号至从设备。

SPI 以主从方式工作，通常有一个主设备和一个或多个从设备。通信由主设备发起，主设备通过 CS 选择要通信的从设备，然后通过 SCLK 给从设备提供时钟信号，数据通过 MOSI 输出给从设备，同时通过 MISO 接收从设备发送的数据。

如下图所示芯片有 2 个 SPI 控制器，SPI 控制器对应 SPI 主设备，每个 SPI 控制器可以连接多个 SPI 从设备。挂载在同一个 SPI 控制器上的从设备共享 3 个信号引脚：SCK、MISO、MOSI，但每个从设备的 CS 引脚是独立的。    
![222](https://github.com/lqr0323/RSOC-2024-Day4/blob/main/spi2.png)

  QSPI: QSPI 是 Queued SPI 的简写，是 Motorola 公司推出的 SPI 接口的扩展，比 SPI 应用更加广泛。在 SPI 协议的基础上，Motorola 公司对其功能进行了增强，增加了队列传输机制，推出了队列串行外围接口协议（即 QSPI 协议）。使用该接口，用户可以一次性传输包含多达 16 个 8 位或 16 位数据的传输队列。一旦传输启动，直到传输结束，都不需要 CPU 干预，极大的提高了传输效率。与 SPI 相比，QSPI 的最大结构特点是以 80 字节的 RAM 代替了 SPI 的发送和接收数据寄存器。

Dual SPI Flash: 对于 SPI Flash 而言全双工并不常用，可以发送一个命令字节进入 Dual 模式，让它工作在半双工模式，用以加倍数据传输。这样 MOSI 变成 SIO0（serial io 0），MISO 变成 SIO1（serial io 1）,这样一个时钟周期内就能传输 2 个 bit 数据，加倍了数据传输。

Quad SPI Flash: 与 Dual SPI 类似，Quad SPI Flash增加了两根 I/O 线（SIO2,SIO3），目的是一个时钟内传输 4 个 bit 数据。

所以对于 SPI Flash，有标准 SPI Flash，Dual SPI Flash, Quad SPI Flash 三种类型。在相同时钟下，线数越多传输速率越高。  
## 挂载 SPI 设备
SPI 驱动会注册 SPI 总线，SPI 设备需要挂载到已经注册好的 SPI 总线上。  
```
rt_err_t rt_spi_bus_attach_device_cspin(struct rt_spi_device *device,
                                        const char           *name,
                                        const char           *bus_name,
                                        rt_base_t            cs_pin,
                                        void                 *user_data)
```
## 配置 SPI 设备
挂载 SPI 设备到 SPI 总线后需要配置 SPI 设备的传输参数。  
```
rt_err_t rt_spi_configure(struct rt_spi_device *device,
                          struct rt_spi_configuration *cfg)
```
## 查找 SPI 设备
在使用 SPI 设备前需要根据 SPI 设备名称获取设备句柄，进而才可以操作 SPI 设备，查找设备函数如下所示:  
```
rt_device_t rt_device_find(const char* name);
```
```
#define W25Q_SPI_DEVICE_NAME     "qspi10"   /* SPI 设备名称 */
struct rt_spi_device *spi_dev_w25q;     /* SPI 设备句柄 */

/* 查找 spi 设备获取设备句柄 */
spi_dev_w25q = (struct rt_spi_device *)rt_device_find(W25Q_SPI_DEVICE_NAME);
```
## 测试代码   

### main.c
```
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#define I2C_DEV_NAME "i2c1"    // 根据你的 BSP 配置修改
#define SPI_DEV_NAME "spi10"   // 根据你的 BSP 配置修改
#define VIRTUAL_DEV_NAME "virtual_dev"

static rt_device_t i2c_dev;
static rt_device_t spi_dev;
static rt_device_t virtual_dev;

/* 线程入口函数 */
void demo_thread_entry(void *parameter)
{
    rt_uint8_t eeprom_data[16];
    rt_uint8_t flash_data[16];
    rt_err_t result;

    /* 打开I2C设备 */
    i2c_dev = rt_device_find(I2C_DEV_NAME);
    if (i2c_dev == RT_NULL)
    {
        rt_kprintf("Failed to find I2C device\n");
        return;
    }
    rt_device_open(i2c_dev, RT_DEVICE_FLAG_RDWR);

    /* 打开SPI设备 */
    spi_dev = rt_device_find(SPI_DEV_NAME);
    if (spi_dev == RT_NULL)
    {
        rt_kprintf("Failed to find SPI device\n");
        rt_device_close(i2c_dev);
        return;
    }
    rt_device_open(spi_dev, RT_DEVICE_FLAG_RDWR);

    /* 打开虚拟设备 */
    virtual_dev = rt_device_find(VIRTUAL_DEV_NAME);
    if (virtual_dev == RT_NULL)
    {
        rt_kprintf("Failed to find virtual device\n");
        rt_device_close(i2c_dev);
        rt_device_close(spi_dev);
        return;
    }
    rt_device_open(virtual_dev, RT_DEVICE_FLAG_RDWR);

    /* 从EEPROM读取数据 */
    result = rt_device_read(i2c_dev, 0x00, eeprom_data, sizeof(eeprom_data));
    if (result != sizeof(eeprom_data))
    {
        rt_kprintf("Failed to read data from EEPROM\n");
    }

    /* 将数据写入到虚拟设备 */
    result = rt_device_write(virtual_dev, 0x00, eeprom_data, sizeof(eeprom_data));
    if (result != sizeof(eeprom_data))
    {
        rt_kprintf("Failed to write data to virtual device\n");
    }

    /* 从虚拟设备读取数据 */
    result = rt_device_read(virtual_dev, 0x00, flash_data, sizeof(flash_data));
    if (result != sizeof(flash_data))
    {
        rt_kprintf("Failed to read data from virtual device\n");
    }

    /* 比较读取到的数据是否一致 */
    if (rt_memcmp(eeprom_data, flash_data, sizeof(eeprom_data)) == 0)
    {
        rt_kprintf("Data transfer successful!\n");
    }
    else
    {
        rt_kprintf("Data transfer failed!\n");
    }

    /* 关闭设备 */
    rt_device_close(i2c_dev);
    rt_device_close(spi_dev);
    rt_device_close(virtual_dev);
}

/* 初始化线程 */
int demo_init(void)
{
    rt_thread_t demo_thread;

    demo_thread = rt_thread_create("demo_thread",
                                   demo_thread_entry,
                                   RT_NULL,
                                   2048,
                                   RT_THREAD_PRIORITY_MAX - 2,
                                   20);
    if (demo_thread != RT_NULL)
    {
        rt_thread_startup(demo_thread);
    }
    else
    {
        rt_kprintf("Failed to create demo thread\n");
    }

    return 0;
}

int main(void)
{
    /* 系统初始化代码 */
    rt_kprintf("RT-Thread initialization...\n");

    /* 调用演示初始化函数 */
    demo_init();

    /* 系统其他初始化代码，如果需要的话 */

    return 0;
}
```

### drv_virtual.c  
```
#include <rtthread.h>
#include <rtdevice.h>

#define VIRTUAL_DEVICE_NAME "virtual_dev"
#define VIRTUAL_BUFFER_SIZE 128

static char virtual_buffer[VIRTUAL_BUFFER_SIZE];
static struct rt_device virtual_device;

/* 读操作 */
static rt_size_t virtual_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    if (pos + size > VIRTUAL_BUFFER_SIZE)
        size = VIRTUAL_BUFFER_SIZE - pos;

    rt_memcpy(buffer, &virtual_buffer[pos], size);
    return size;
}

/* 写操作 */
static rt_size_t virtual_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    if (pos + size > VIRTUAL_BUFFER_SIZE)
        size = VIRTUAL_BUFFER_SIZE - pos;

    rt_memcpy(&virtual_buffer[pos], buffer, size);
    return size;
}

/* 打开设备 */
static rt_err_t virtual_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

/* 关闭设备 */
static rt_err_t virtual_close(rt_device_t dev)
{
    return RT_EOK;
}

/* 控制设备 */
static rt_err_t virtual_control(rt_device_t dev, int cmd, void *args)
{
    return RT_EOK;
}

int rt_hw_virtual_device_init(void)
{
    /* 初始化设备结构体 */
    virtual_device.type = RT_Device_Class_Char;
    virtual_device.init = RT_NULL;
    virtual_device.open = virtual_open;
    virtual_device.close = virtual_close;
    virtual_device.read = virtual_read;
    virtual_device.write = virtual_write;
    virtual_device.control = virtual_control;
    virtual_device.user_data = RT_NULL;

    return rt_device_register(&virtual_device, VIRTUAL_DEVICE_NAME, RT_DEVICE_FLAG_RDWR);
}
INIT_DEVICE_EXPORT(rt_hw_virtual_device_init);
```
## rt_pin_irq_example.c
```
#include<rtthread.h>
#include<rtdevice.h>
#include<drv_gpio.h>

#define KEY_UP      GET_PIN(C,5)
#define KEY_DOWN    GET_PIN(C,1)
#define KEY_LEFT    GET_PIN(C,0)
#define KEY_RIGHT   GET_PIN(C,4)

#define LOG_TAG     "pin.irp"
#define LOG_LVL     LOG_LVL_DBG
#include<ulog.h>

void key_up_callback(void *args)
{
    int value = rt_pin_read(KEY_UP);
    LOG_I("key up! %d",value);
}

void key_down_callback(void *args)
{
    int value = rt_pin_read(KEY_DOWN);
    LOG_I("key down! %d",value);
}

void key_left_callback(void *args)
{
    int value = rt_pin_read(KEY_LEFT);
    LOG_I("key left! %d",value);
}

void key_right_callback(void *args)
{
    int value = rt_pin_read(KEY_RIGHT);
    LOG_I("key right! %d",value);
}


static int rt_pin_irq_example(void)
{
    rt_pin_mode(KEY_UP,PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(KEY_DOWN,PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(KEY_LEFT,PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(KEY_RIGHT,PIN_MODE_INPUT_PULLUP);

    rt_pin_attach_irq(KEY_UP,PIN_IRQ_MODE_FALLING,key_up_callback,RT_NULL);
    rt_pin_attach_irq(KEY_DOWN,PIN_IRQ_MODE_FALLING,key_up_callback,RT_NULL);
    rt_pin_attach_irq(KEY_DOWN,PIN_IRQ_MODE_FALLING,key_up_callback,RT_NULL);
    rt_pin_attach_irq(KEY_DOWN,PIN_IRQ_MODE_FALLING,key_up_callback,RT_NULL);
    
    rt_pin_irq_enable(KEY_UP,PIN_IRQ_ENABLE);
    rt_pin_irq_enable(KEY_DOWN,PIN_IRQ_ENABLE);
    rt_pin_irq_enable(KEY_LEFT,PIN_IRQ_ENABLE);
    rt_pin_irq_enable(KEY_RIGHT,PIN_IRQ_ENABLE);

}
MSH_CMD_EXPORT(rt_pin_irq_example,rt_pin_irq_example);
```
![332](https://github.com/lqr0323/RSOC-2024-Day4/blob/main/%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE%202024-07-25%20174836.png)
## 感觉今天学的信息量有点大，还是要再思考思考！！！！
