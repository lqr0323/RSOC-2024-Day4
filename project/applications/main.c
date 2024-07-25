#include <board.h>
#include <rtthread.h>
#include <drv_gpio.h>
#ifndef RT_USING_NANO
#include <rtdevice.h>
#endif /* RT_USING_NANO */

#define PIN_KEY0      GET_PIN(C, 0)     // PC0:  KEY0         --> KEY
#define GPIO_LED_R    GET_PIN(F, 12)

#define THREAD_PRIORITY         25
#define THREAD_STACK_SIZE       1024
#define THREAD_TIMESLICE        5

static rt_thread_t tid1 = RT_NULL;
static rt_thread_t tid2 = RT_NULL;
static rt_sem_t key_sem = RT_NULL;
static rt_mailbox_t mb = RT_NULL;
static rt_uint32_t key_press_count = 0;  // 记录按键按下次数

static void key_thread_entry(void *parameter);
static void led_thread_entry(void *parameter);

int main(void)
{
    // 初始化LED引脚为输出模式
    rt_pin_mode(GPIO_LED_R, PIN_MODE_OUTPUT);
    // 初始化按键引脚为输入模式
    rt_pin_mode(PIN_KEY0, PIN_MODE_INPUT_PULLUP);

    // 创建一个动态信号量
    key_sem = rt_sem_create("key_sem", 0, RT_IPC_FLAG_PRIO);
    if (key_sem == RT_NULL)
    {
        rt_kprintf("Create semaphore failed.\n");
        return -1;
    }

    // 创建一个邮箱
    mb = rt_mb_create("mb", 10, RT_IPC_FLAG_PRIO);
    if (mb == RT_NULL)
    {
        rt_kprintf("Create mailbox failed.\n");
        return -1;
    }

    // 创建按键线程
    tid1 = rt_thread_create("key_thread",
                            key_thread_entry, RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY, THREAD_TIMESLICE);
    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);

    // 创建LED线程
    tid2 = rt_thread_create("led_thread",
                            led_thread_entry, RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY, THREAD_TIMESLICE);
    if (tid2 != RT_NULL)
        rt_thread_startup(tid2);

    return 0;
}

static void key_thread_entry(void *parameter)
{
    int key_state = 1; // 初始状态为松开
    int prev_key_state = 1;
    while (1)
    {
        key_state = rt_pin_read(PIN_KEY0);
        if (key_state == PIN_LOW && prev_key_state == PIN_HIGH)
        {
            // 按键按下
            rt_kprintf("KEY0 pressed!\r\n");
            key_press_count++; // 增加按键按下次数
            rt_mb_send(mb, 1); // 发送按下信号
            rt_sem_release(key_sem);
        }
        else if (key_state == PIN_HIGH && prev_key_state == PIN_LOW)
        {
            // 按键松开
            rt_kprintf("KEY0 released!\r\n");
            rt_mb_send(mb, 0); // 发送松开信号
            rt_sem_release(key_sem);
        }
        prev_key_state = key_state;
        rt_thread_mdelay(10);
    }
}

static void led_thread_entry(void *parameter)
{
    rt_uint32_t key_event = 0;
    while (1)
    {
        rt_sem_take(key_sem, RT_WAITING_FOREVER); // 等待信号量
        if (rt_mb_recv(mb, &key_event, RT_WAITING_FOREVER) == RT_EOK)
        {
            if (key_event == 1)
            {
                // 按键按下时点亮LED
                rt_kprintf("LED ON\r\n");
                rt_kprintf("Key press count: %d\r\n", key_press_count);
                rt_pin_write(GPIO_LED_R, PIN_HIGH);
            }
            else
            {
                // 按键松开时熄灭LED
                rt_kprintf("LED OFF\r\n");
            }
        }
    }
}
