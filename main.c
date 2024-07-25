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
