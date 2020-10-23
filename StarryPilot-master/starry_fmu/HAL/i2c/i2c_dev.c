/*
 * File      : i2c_dev.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2012, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author        Notes
 * 2012-04-25     weety         first version
 * 2014-08-03     bernard       fix some compiling warning
 * 2016-06-06	  zoujiachi		add i2c init func
 */

#include <rtdevice.h>
#include <string.h>
#include "i2c_soft.h"
#include "i2c-bit-ops.h"
#include "console.h"
#include <stdio.h>
#include <stdlib.h>

struct rt_i2c_bus_device rt_i2c1;
struct rt_i2c_bus_device rt_i2c2;
BSP_I2C_Def i2c1_def = BSP_I2C1 , i2c2_def = BSP_I2C2;

static rt_size_t i2c_bus_device_read(rt_device_t dev,
                                     rt_off_t    pos,
                                     void       *buffer,
                                     rt_size_t   count)
{
    rt_uint16_t addr;
    rt_uint16_t flags;
    struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)dev->user_data;

    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(buffer != RT_NULL);

    i2c_dbg("I2C bus dev [%s] reading %u bytes.\n", dev->parent.name, count);

    addr = pos & 0xffff;
    flags = (pos >> 16) & 0xffff;

    return rt_i2c_master_recv(bus, addr, flags, buffer, count);
}

static rt_size_t i2c_bus_device_write(rt_device_t dev,
                                      rt_off_t    pos,
                                      const void *buffer,
                                      rt_size_t   count)
{
    rt_uint16_t addr;
    rt_uint16_t flags;
    struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)dev->user_data;

    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(buffer != RT_NULL);

    i2c_dbg("I2C bus dev %s writing %u bytes.\n", dev->parent.name, count);

    addr = pos & 0xffff;
    flags = (pos >> 16) & 0xffff;

    return rt_i2c_master_send(bus, addr, flags, buffer, count);
}

static rt_err_t i2c_bus_device_control(rt_device_t dev,
                                       rt_uint8_t  cmd,
                                       void       *args)
{
    rt_err_t ret;
    struct rt_i2c_priv_data *priv_data;
    struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)dev->user_data;

    RT_ASSERT(bus != RT_NULL);

    switch (cmd)
    {
    /* set 10-bit addr mode */
    case RT_I2C_DEV_CTRL_10BIT:
        bus->flags |= RT_I2C_ADDR_10BIT;
        break;
    case RT_I2C_DEV_CTRL_ADDR:
        bus->addr = *(rt_uint16_t *)args;
        break;
    case RT_I2C_DEV_CTRL_TIMEOUT:
        bus->timeout = *(rt_uint32_t *)args;
        break;
    case RT_I2C_DEV_CTRL_RW:
        priv_data = (struct rt_i2c_priv_data *)args;
        ret = rt_i2c_transfer(bus, priv_data->msgs, priv_data->number);
        if (ret < 0)
        {
            return -RT_EIO;
        }
        break;
    default:
        break;
    }

    return RT_EOK;
}

rt_err_t i2c_bus_device_init(rt_device_t dev)
{
	BSP_I2C_Def i2c_dev;
	rt_err_t err;
	
	//struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)dev->user_data;
	struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)dev;
	i2c_dev = *(BSP_I2C_Def*)((struct rt_i2c_bit_ops*)bus->priv)->data;

	err = stm32_i2c_pin_init(i2c_dev);
	
	return err;
}

rt_err_t rt_i2c_bus_device_device_init(struct rt_i2c_bus_device *bus,
                                       const char               *name)
{
    struct rt_device *device;
    RT_ASSERT(bus != RT_NULL);

    device = &bus->parent;

    device->user_data = bus;

    /* set device type */
    device->type    = RT_Device_Class_I2CBUS;
    /* initialize device interface */
    device->init    = i2c_bus_device_init;
    device->open    = RT_NULL;
    device->close   = RT_NULL;
    device->read    = i2c_bus_device_read;
    device->write   = i2c_bus_device_write;
    device->control = i2c_bus_device_control;

    /* register to device manager */
    rt_device_register(device, name, RT_DEVICE_FLAG_RDWR);

    return RT_EOK;
}

rt_err_t device_i2c_init(char* name)
{
	rt_err_t err;
	
	if(!strcmp(name, "i2c1"))
	{
		rt_memset((void *)&rt_i2c1, 0, sizeof(struct rt_i2c_bus_device));

		stm32_i2c1_bit_ops.data = (void*)&i2c1_def;
		rt_i2c1.priv = (void *)&stm32_i2c1_bit_ops;
		rt_i2c1.retries = 3;
		err = rt_i2c_bit_add_bus(&rt_i2c1, name);
	}
	else if(!strcmp(name, "i2c2"))
	{
		rt_memset((void *)&rt_i2c2, 0, sizeof(struct rt_i2c_bus_device));

		stm32_i2c2_bit_ops.data = (void*)&i2c2_def;
		rt_i2c2.priv = (void *)&stm32_i2c2_bit_ops;
		rt_i2c2.retries = 3;
		err = rt_i2c_bit_add_bus(&rt_i2c2, name);
	}
	else
	{
		return RT_ERROR;
	}
	
	return err;
}

