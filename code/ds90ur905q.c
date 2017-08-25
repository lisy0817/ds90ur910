/*
 * Copyright (C) 2005-2006 Micronas USA Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/delay.h>
#include <linux/device.h>

static const u8 initial_registers[] = {
	0x00, 0x00, /* Ser Config 1*/
	0x01, 0x68, /* Device ID */
	0x02, 0x00, /* De-Emphasis Control */
	0xff, 0xff, /* Terminator  */
};


static int ds90ur905q_read_reg(struct i2c_client *client, u8 reg)
{
	return i2c_smbus_read_byte_data(client, reg);
}

static int write_reg(struct i2c_client *client, u8 reg, u8 value)
{
	return i2c_smbus_write_byte_data(client, reg, value);
}

static int write_regs(struct i2c_client *client, const u8 *regs)
{
	int i;

	for (i = 0; regs[i] != 0xff; i += 2)
		if (write_reg(client, regs[i], regs[i + 1]) < 0)
			return -1;
	return 0;
}
	
static int ds90ur905_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	enum of_gpio_flags flags;
	int pdn_gpio = -1, pdn_active = 0, rstb_gpio = -1, rstb_active = 0, lcden_gpio = -1, lcden_active = 0;;
	struct pinctrl *pinctrl;
	struct device *dev = &client->dev;

	printk("ds90ur905_probe ds90ur905\n");
	/* Check if the adapter supports the needed features */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	printk("chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);

	/* pinctrl */
	/*pinctrl = devm_pinctrl_get_select_default(dev);
	if (IS_ERR(pinctrl)) {
		dev_err(dev, "ds90ur905_probe setup pinctrl failed\n");
		return PTR_ERR(pinctrl);
	}*/
	
	/* request power down pin */
	pdn_gpio = of_get_named_gpio_flags(dev->of_node, "pdb-gpios", 0, &flags);
	if (gpio_is_valid(pdn_gpio)) {
		unsigned long gpiof;
		if (flags == OF_GPIO_ACTIVE_LOW) {
			pdn_active = 0;
			gpiof = GPIOF_OUT_INIT_HIGH;
		}
		else {
			pdn_active = 1;
			gpiof = GPIOF_OUT_INIT_LOW;
		}

		if (devm_gpio_request_one(dev, pdn_gpio, GPIOF_OUT_INIT_HIGH, "ds90ur905_pdb")) {
			dev_warn(dev, "ds90ur905_probe no power pin available!\n");
			pdn_gpio = -1;
		}
	}
	else
		dev_err(dev, "ds90ur905_probe no sensor pdb pin available\n");

	/* request bisten pin */
	rstb_gpio = of_get_named_gpio_flags(dev->of_node, "bisten-gpios", 0, &flags);
	if (gpio_is_valid(rstb_gpio)) {
		unsigned long gpiof;
		if (flags == OF_GPIO_ACTIVE_LOW) {
			rstb_active = 0;
			gpiof = GPIOF_OUT_INIT_HIGH;
		}
		else {
			rstb_active = 1;
			gpiof = GPIOF_OUT_INIT_LOW;
		}

		if (devm_gpio_request_one(dev, rstb_gpio, GPIOF_OUT_INIT_HIGH, "ds90ur905_bisten")) {
			dev_warn(dev, "ds90ur905_probe no bisten pin available!\n");
			rstb_gpio = -1;
		}
	}
	else
		dev_err(dev, "ds90ur905_probe no sensor bisten pin available\n");

	/*if (pdn_gpio >= 0) {
		gpio_set_value_cansleep(pdn_gpio, !pdn_active);
		msleep(10);
	}*/
	
	gpio_set_value(pdn_gpio, 1);
	msleep(20);
	
	/* Initialize ds90ur905 */

	/*if (write_regs(client, initial_registers) < 0) {
		dev_err(client, "error initializing ds90ur905\n");
		return -EINVAL;
	}*/
	
	//write_reg(client, 0x00,  (ds90ur905q_read_reg(client, 0x00) & ~(0x03<< 2)) | 0x03<<2 | 0x01);
	gpio_set_value(rstb_gpio, 1);

	printk( "read_reg @ 0x%x\n", ds90ur905q_read_reg(client, 0x01));
	dev_err(dev, "read_reg 0x00 = 0x%x \n",  ds90ur905q_read_reg(client, 0x00));
	
	return 0;
}

static int ds90ur905_remove(struct i2c_client *client)
{
	return 0;
}

/* ----------------------------------------------------------------------- */

static const struct i2c_device_id ds90ur905_id[] = {
	{ "ds90ur905q", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, ds90ur905_id);

static const struct of_device_id ds90ur905q_of_match[] = {
       { .compatible = "ti,ds90ur905q", },
       { }
};
MODULE_DEVICE_TABLE(of, ds90ur905q_of_match);

static struct i2c_driver ds90ur905_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "ds90ur905q",
		.of_match_table = ds90ur905q_of_match,
	},
	.probe = ds90ur905_probe,
	.remove = ds90ur905_remove,
	.id_table = ds90ur905_id,
};

module_i2c_driver(ds90ur905_driver);

MODULE_DESCRIPTION("ds90ur905 I2C subdev driver");
MODULE_LICENSE("GPL v2");
