/*
 * File      : system.c
 *
 *
 * Change Logs:
 * Date           Author       	Notes
 * 2017-09-28     zoujiachi   	the first version
 */

#include "system.h"
#include "statistic.h"
#include "console.h"
#include <string.h>
#include <rtthread.h>

extern struct rt_object_information rt_object_container[];

rt_inline unsigned int rt_list_len(const rt_list_t *l)
{
    unsigned int len = 0;
    const rt_list_t *p = l;
    while (p->next != l)
    {
        p = p->next;
        len ++;
    }

    return len;
}

static void show_wait_queue(struct rt_list_node *list)
{
    struct rt_thread *thread;
    struct rt_list_node *node;

    for (node = list->next; node != list; node = node->next)
    {
        thread = rt_list_entry(node, struct rt_thread, tlist);
        Console.print("%s", thread->name);

        if (node->next != list)
            Console.print("/");
    }
}

static long _sys_list_thread(struct rt_list_node *list)
{
    struct rt_thread *thread;
    struct rt_list_node *node;
    rt_uint8_t *ptr;

    Console.print(" thread  pri  status      sp     stack size max used   left tick  error\n");
    Console.print("-------- ---- ------- ---------- ---------- ---------- ---------- ---\n");
    for (node = list->next; node != list; node = node->next)
    {
        thread = rt_list_entry(node, struct rt_thread, list);
        Console.print("%-8.*s 0x%02x", RT_NAME_MAX, thread->name, thread->current_priority);

        if (thread->stat == RT_THREAD_READY)        rt_kprintf(" ready  ");
        else if (thread->stat == RT_THREAD_SUSPEND) rt_kprintf(" suspend");
        else if (thread->stat == RT_THREAD_INIT)    rt_kprintf(" init   ");
        else if (thread->stat == RT_THREAD_CLOSE)   rt_kprintf(" close  ");

        ptr = (rt_uint8_t *)thread->stack_addr;
        while (*ptr == '#')ptr ++;

        Console.print(" 0x%08x 0x%08x 0x%08x 0x%08x %03d\n",
                   thread->stack_size + ((rt_uint32_t)thread->stack_addr - (rt_uint32_t)thread->sp),
                   thread->stack_size,
                   thread->stack_size - ((rt_uint32_t) ptr - (rt_uint32_t)thread->stack_addr),
                   thread->remaining_tick,
                   thread->error);
    }

    return 0;
}

static long _sys_list_sem(struct rt_list_node *list)
{
    struct rt_semaphore *sem;
    struct rt_list_node *node;

    Console.print("semaphore v   suspend thread\n");
    Console.print("--------  --- --------------\n");
    for (node = list->next; node != list; node = node->next)
    {
        sem = (struct rt_semaphore *)(rt_list_entry(node, struct rt_object, list));
        if (!rt_list_isempty(&sem->parent.suspend_thread))
        {
            Console.print("%-8.*s  %03d %d:",
                       RT_NAME_MAX,
                       sem->parent.parent.name,
                       sem->value,
                       rt_list_len(&sem->parent.suspend_thread));
            show_wait_queue(&(sem->parent.suspend_thread));
            Console.print("\n");
        }
        else
        {
            Console.print("%-8.*s  %03d %d\n",
                       RT_NAME_MAX,
                       sem->parent.parent.name,
                       sem->value,
                       rt_list_len(&sem->parent.suspend_thread));
        }
    }

    return 0;
}

static long _sys_list_event(struct rt_list_node *list)
{
    struct rt_event *e;
    struct rt_list_node *node;

    Console.print("event    set        suspend thread\n");
    Console.print("-------- ---------- --------------\n");
    for (node = list->next; node != list; node = node->next)
    {
        e = (struct rt_event *)(rt_list_entry(node, struct rt_object, list));
        if (!rt_list_isempty(&e->parent.suspend_thread))
        {
            Console.print("%-8.*s  0x%08x %03d:",
                       RT_NAME_MAX,
                       e->parent.parent.name,
                       e->set,
                       rt_list_len(&e->parent.suspend_thread));
            show_wait_queue(&(e->parent.suspend_thread));
            Console.print("\n");
        }
        else
        {
            Console.print("%-8.*s  0x%08x 0\n",
                       RT_NAME_MAX, e->parent.parent.name, e->set);
        }
    }

    return 0;
}

static long _sys_list_mutex(struct rt_list_node *list)
{
    struct rt_mutex *m;
    struct rt_list_node *node;

    Console.print("mutex    owner    hold suspend thread\n");
    Console.print("-------- -------- ---- --------------\n");
    for (node = list->next; node != list; node = node->next)
    {
        m = (struct rt_mutex *)(rt_list_entry(node, struct rt_object, list));
        Console.print("%-8.*s %-8.*s %04d %d\n",
                   RT_NAME_MAX,
                   m->parent.parent.name,
                   RT_NAME_MAX,
                   m->owner->name,
                   m->hold,
                   rt_list_len(&m->parent.suspend_thread));
    }

    return 0;
}

static long _sys_list_mailbox(struct rt_list_node *list)
{
    struct rt_mailbox *m;
    struct rt_list_node *node;

    Console.print("mailbox  entry size suspend thread\n");
    Console.print("-------- ----  ---- --------------\n");
    for (node = list->next; node != list; node = node->next)
    {
        m = (struct rt_mailbox *)(rt_list_entry(node, struct rt_object, list));
        if (!rt_list_isempty(&m->parent.suspend_thread))
        {
            Console.print("%-8.*s %04d  %04d %d:",
                       RT_NAME_MAX,
                       m->parent.parent.name,
                       m->entry,
                       m->size,
                       rt_list_len(&m->parent.suspend_thread));
            show_wait_queue(&(m->parent.suspend_thread));
            rt_kprintf("\n");
        }
        else
        {
            Console.print("%-8.*s %04d  %04d %d\n",
                       RT_NAME_MAX,
                       m->parent.parent.name,
                       m->entry,
                       m->size,
                       rt_list_len(&m->parent.suspend_thread));
        }
    }

    return 0;
}

static long _sys_list_msgqueue(struct rt_list_node *list)
{
    struct rt_messagequeue *m;
    struct rt_list_node *node;

    Console.print("msgqueue entry suspend thread\n");
    Console.print("-------- ----  --------------\n");
    for (node = list->next; node != list; node = node->next)
    {
        m = (struct rt_messagequeue *)(rt_list_entry(node, struct rt_object, list));
        if (!rt_list_isempty(&m->parent.suspend_thread))
        {
            Console.print("%-8.*s %04d  %d:",
                       RT_NAME_MAX,
                       m->parent.parent.name,
                       m->entry,
                       rt_list_len(&m->parent.suspend_thread));
            show_wait_queue(&(m->parent.suspend_thread));
            Console.print("\n");
        }
        else
        {
            Console.print("%-8.*s %04d  %d\n",
                       RT_NAME_MAX,
                       m->parent.parent.name,
                       m->entry,
                       rt_list_len(&m->parent.suspend_thread));
        }
    }

    return 0;
}

static long _sys_list_mempool(struct rt_list_node *list)
{
    struct rt_mempool *mp;
    struct rt_list_node *node;

    Console.print("mempool  block total free suspend thread\n");
    Console.print("-------- ----  ----  ---- --------------\n");
    for (node = list->next; node != list; node = node->next)
    {
        mp = (struct rt_mempool *)rt_list_entry(node, struct rt_object, list);
        if (mp->suspend_thread_count > 0)
        {
            Console.print("%-8.*s %04d  %04d  %04d %d:",
                       RT_NAME_MAX,
                       mp->parent.name,
                       mp->block_size,
                       mp->block_total_count,
                       mp->block_free_count,
                       mp->suspend_thread_count);
            show_wait_queue(&(mp->suspend_thread));
            Console.print("\n");
        }
        else
        {
            Console.print("%-8.*s %04d  %04d  %04d %d\n",
                       RT_NAME_MAX,
                       mp->parent.name,
                       mp->block_size,
                       mp->block_total_count,
                       mp->block_free_count,
                       mp->suspend_thread_count);
        }
    }

    return 0;
}

static long _sys_list_timer(struct rt_list_node *list)
{
    struct rt_timer *timer;
    struct rt_list_node *node;

    Console.print("timer    periodic   timeout    flag\n");
    Console.print("-------- ---------- ---------- -----------\n");
    for (node = list->next; node != list; node = node->next)
    {
        timer = (struct rt_timer *)(rt_list_entry(node, struct rt_object, list));
        Console.print("%-8.*s 0x%08x 0x%08x ",
                   RT_NAME_MAX,
                   timer->parent.name,
                   timer->init_tick,
                   timer->timeout_tick);
        if (timer->parent.flag & RT_TIMER_FLAG_ACTIVATED)
            Console.print("activated\n");
        else
            Console.print("deactivated\n");
    }

    Console.print("current tick:0x%08x\n", rt_tick_get());

    return 0;
}

static long _sys_list_device(struct rt_list_node *list)
{
    struct rt_device *device;
    struct rt_list_node *node;
    char *const device_type_str[] =
    {
        "Character Device",
        "Block Device",
        "Network Interface",
        "MTD Device",
        "CAN Device",
        "RTC",
        "Sound Device",
        "Graphic Device",
        "I2C Bus",
        "USB Slave Device",
        "USB Host Bus",
        "SPI Bus",
        "SPI Device",
        "SDIO Bus",
        "PM Pseudo Device",
        "Pipe",
        "Portal Device",
        "Timer Device",
        "Miscellaneous Device",
        "Unknown"
    };

    Console.print("device   type                 ref count\n");
    Console.print("-------- -------------------- ----------\n");
    for (node = list->next; node != list; node = node->next)
    {
        device = (struct rt_device *)(rt_list_entry(node, struct rt_object, list));
        Console.print("%-8.*s %-20s %-8d\n",
                   RT_NAME_MAX,
                   device->parent.name,
                   (device->type <= RT_Device_Class_Unknown) ?
                   device_type_str[device->type] :
                   device_type_str[RT_Device_Class_Unknown],
                   device->ref_count);
    }

    return 0;
}

int handle_sys_shell_cmd(int argc, char** argv)
{
	float cpu_usage = get_cpu_usage();
	
	Console.print("CPU Usage: %.2f\n", cpu_usage);
	
	return 0;
}

int handle_rtt_shell_cmd(int argc, char** argv)
{
	if(argc >= 2){
		if( strcmp(argv[1], "list_thread") == 0 ){
			_sys_list_thread(&rt_object_container[RT_Object_Class_Thread].object_list);
		}
		if( strcmp(argv[1], "list_sem") == 0 ){
			_sys_list_sem(&rt_object_container[RT_Object_Class_Semaphore].object_list);
		}
		if( strcmp(argv[1], "list_event") == 0 ){
			_sys_list_event(&rt_object_container[RT_Object_Class_Event].object_list);
		}
		if( strcmp(argv[1], "list_mutex") == 0 ){
			_sys_list_mutex(&rt_object_container[RT_Object_Class_Mutex].object_list);
		}
		if( strcmp(argv[1], "list_mailbox") == 0 ){
			_sys_list_mailbox(&rt_object_container[RT_Object_Class_MailBox].object_list);
		}
		if( strcmp(argv[1], "list_msgqueue") == 0 ){
			_sys_list_msgqueue(&rt_object_container[RT_Object_Class_MessageQueue].object_list);
		}
		if( strcmp(argv[1], "list_mempool") == 0 ){
			_sys_list_mempool(&rt_object_container[RT_Object_Class_MemPool].object_list);
		}
		if( strcmp(argv[1], "list_timer") == 0 ){
			_sys_list_timer(&rt_object_container[RT_Object_Class_Timer].object_list);
		}
		if( strcmp(argv[1], "list_device") == 0 ){
			_sys_list_device(&rt_object_container[RT_Object_Class_Device].object_list);
		}
	}
	
	return 0;
}