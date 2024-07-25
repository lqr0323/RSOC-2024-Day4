#include<rtthread.h>
#include<rtdevice.h>
static int rt_device_test_init(void)
{
    rt_device_t test_dev = rt_device_create(RT_Device_Class_Char,0);
    if(!test_dev)
    {
        rt_kprintf("test_dev create failled!\n");
        return -RT_ERROR;
    }

    if(rt_device_register(test_dev,"tset_dev",RT_DEVICE_FLAG_RDWR) != RT_EOK)
    {
        rt_kprintf("test_dev create failled!\n");
        return -RT_ERROR;
    }

    return RT_EOK;

}
MSH_CMD_EXPORT(rt_device_test_init,rt_device_test_init);