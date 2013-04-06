/* linux/arch/arm/mach-msm/display/doubleshot-panel.c
 *
 * Copyright (c) 2011 HTC.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm/io.h>
#include <asm/mach-types.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <mach/msm_fb.h>
#include <mach/msm_iomap.h>
#include <mach/panel_id.h>
#include <mach/msm_bus_board.h>
#include <mach/debug_display.h>
#include <mach/msm_memtypes.h>

#include "../devices.h"
#include "../board-doubleshot.h"
#include "../devices-msm8x60.h"
#include "../../../../drivers/video/msm/mdp_hw.h"

#define MDP_VSYNC_GPIO			28


//extern int panel_type;

static struct regulator *l1_3v;
static struct regulator *lvs1_1v8;
static struct regulator *l4_1v8;

static void doubleshot_id1_im1_switch(int on)
{

#if 0
	int rc = 0;
	pr_info("panel id1 im1 switch %d\n", on);

	if (on) {
	if (system_rev >= 1) {
		rc = gpio_request(GPIO_LCM_ID1_IM1_XB, "LCM_ID1_IM1");
		if (rc < 0)
			pr_err("GPIO (%d) request fail\n", GPIO_LCM_ID1_IM1_XB);
		else if (panel_type == PANEL_ID_DOT_HITACHI)
				gpio_direction_output(GPIO_LCM_ID1_IM1_XB, 1);
			else
				gpio_direction_output(GPIO_LCM_ID1_IM1_XB, 0);
		gpio_free(GPIO_LCM_ID1_IM1_XB);
	} else {
		rc = gpio_request(GPIO_LCM_ID1_IM1, "LCM_ID1_IM1");
		if (rc < 0)
			pr_err("GPIO (%d) request fail\n", GPIO_LCM_ID1_IM1);
		else if (panel_type == PANEL_ID_DOT_HITACHI)
				gpio_direction_output(GPIO_LCM_ID1_IM1, 1);
			else
				gpio_direction_output(GPIO_LCM_ID1_IM1, 0);
		gpio_free(GPIO_LCM_ID1_IM1);
	}
	} else {
		if (system_rev >= 1) {
			rc = gpio_request(GPIO_LCM_ID1_IM1_XB, "LCM_ID1_IM1");
			if (rc < 0)
				pr_err("GPIO (%d) request fail\n", GPIO_LCM_ID1_IM1_XB);
			else
				gpio_direction_input(GPIO_LCM_ID1_IM1_XB);

			gpio_free(GPIO_LCM_ID1_IM1_XB);
		} else {
			rc = gpio_request(GPIO_LCM_ID1_IM1, "LCM_ID1_IM1");
			if (rc < 0)
				pr_err("GPIO (%d) request fail\n", GPIO_LCM_ID1_IM1);
			else
				gpio_direction_input(GPIO_LCM_ID1_IM1);
			gpio_free(GPIO_LCM_ID1_IM1);
		}
	}
#endif
	return;
}

/*
TODO:
1. move regulator initialization to doubleshot_panel_init()
*/
static void doubleshot_panel_power(int on)
{
	static int init;
	int ret;
	int rc;

	pr_info("%s(%d): init=%d\n", __func__, on, init);
	/* If panel is already on (or off), do nothing. */
	if (!init) {
		l1_3v = regulator_get(NULL, "8901_l1");
		if (IS_ERR(l1_3v)) {
			pr_err("%s: unable to get 8901_l1\n", __func__);
			goto fail;
		}

		if (system_rev >= 1) {
			l4_1v8 = regulator_get(NULL, "8901_l4");
			if (IS_ERR(l4_1v8)) {
				pr_err("%s: unable to get 8901_l4\n", __func__);
				goto fail;
			}
		} else {
			lvs1_1v8 = regulator_get(NULL, "8901_lvs1");
			if (IS_ERR(lvs1_1v8)) {
				pr_err("%s: unable to get 8901_lvs1\n", __func__);
				goto fail;
			}
		}

		ret = regulator_set_voltage(l1_3v, 3100000, 3100000);
		if (ret) {
			pr_err("%s: error setting l1_3v voltage\n", __func__);
			goto fail;
		}

		if (system_rev >= 1) {
			ret = regulator_set_voltage(l4_1v8, 1800000, 1800000);
			if (ret) {
				pr_err("%s: error setting l4_1v8 voltage\n", __func__);
				goto fail;
			}
		}

		/* LCM Reset */
		rc = gpio_request(GPIO_LCM_RST_N,
			"LCM_RST_N");
		if (rc) {
			printk(KERN_ERR "%s:LCM gpio %d request"
				"failed\n", __func__,
				 GPIO_LCM_RST_N);
			return;
		}

		//gpio_direction_output(GPIO_LCM_RST_N, 0);
		init = 1;
	}

	if (!l1_3v || IS_ERR(l1_3v)) {
		pr_err("%s: l1_3v is not initialized\n", __func__);
		return;
	}

	if (system_rev >= 1) {
		if (!l4_1v8 || IS_ERR(l4_1v8)) {
			pr_err("%s: l4_1v8 is not initialized\n", __func__);
			return;
		}
	} else {
	if (!lvs1_1v8 || IS_ERR(lvs1_1v8)) {
		pr_err("%s: lvs1_1v8 is not initialized\n", __func__);
		return;
	}
	}

	if (on) {
		if (regulator_enable(l1_3v)) {
			PR_DISP_ERR("%s: Unable to enable the regulator:"
					" l1_3v\n", __func__);
			return;
		}
		hr_msleep(5);

		if (system_rev >= 1) {
			if (regulator_enable(l4_1v8)) {
				pr_err("%s: Unable to enable the regulator:"
					" l4_1v8\n", __func__);
				return;
			}
		} else {
			if (regulator_enable(lvs1_1v8)) {
				pr_err("%s: Unable to enable the regulator:"
						" lvs1_1v8\n", __func__);
				return;
			}
		}
		mdelay(1);
#if 0
		if (regulator_enable(l1_3v)) {
			pr_err("%s: Unable to enable the regulator:"
					" l1_3v\n", __func__);
			return;
		}
#endif
		/* skip reset for the first time panel power up */
		if ( init == 1 ) {
				init = 2;
				return;
		} else {
			if (panel_type == PANEL_ID_DOT_HITACHI) {
				mdelay(1);
				gpio_set_value(GPIO_LCM_RST_N, 1);
				mdelay(1);
				doubleshot_id1_im1_switch(on);
			} else if (panel_type == PANEL_ID_DOT_SONY ||
				panel_type == PANEL_ID_DOT_SONY_C3) {

			hr_msleep(10);

//				mdelay(1);
				doubleshot_id1_im1_switch(on);
			hr_msleep(1);
//				mdelay(1);
				gpio_set_value(GPIO_LCM_RST_N, 1);
			hr_msleep(10);
//				mdelay(10);
				gpio_set_value(GPIO_LCM_RST_N, 0);
			hr_msleep(10);
//				mdelay(10);
				gpio_set_value(GPIO_LCM_RST_N, 1);
			hr_msleep(20);
//				mdelay(10);
			} else {
				pr_err("panel_type=0x%x not support\n", panel_type);
				return;
			}
		}
	} else {
			gpio_set_value(GPIO_LCM_RST_N, 0);
			doubleshot_id1_im1_switch(on);
			hr_msleep(5);
	//		mdelay(120);

			if (system_rev >= 1) {
				if (regulator_disable(l4_1v8)) {
					pr_err("%s: Unable to enable the regulator:"
						" l4_1v8\n", __func__);
					return;
				}
			} else {
				if (regulator_disable(lvs1_1v8)) {
					pr_err("%s: Unable to enable the regulator:"
							" lvs1_1v8\n", __func__);
					return;
				}
			}
			hr_msleep(5);
			if (regulator_disable(l1_3v)) {
				pr_err("%s: Unable to enable the regulator:"
						" l1_3v\n", __func__);
				return;
			}
	}
	return;

fail:
	if (l1_3v)
		regulator_put(l1_3v);
	if (lvs1_1v8)
		regulator_put(lvs1_1v8);
}

/*
TODO:
1. remove unused LCDC stuff.
*/
static uint32_t lcd_panel_gpios[] = {
	GPIO_CFG(0,  1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_pclk */
	GPIO_CFG(1,  1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_hsync*/
	GPIO_CFG(2,  1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_vsync*/
	GPIO_CFG(3,  1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_den */
	GPIO_CFG(4,  1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_red7 */
	GPIO_CFG(5,  1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_red6 */
	GPIO_CFG(6,  1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_red5 */
	GPIO_CFG(7,  1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_red4 */
	GPIO_CFG(8,  1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_red3 */
	GPIO_CFG(9,  1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_red2 */
	GPIO_CFG(10, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_red1 */
	GPIO_CFG(11, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_red0 */
	GPIO_CFG(12, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_grn7 */
	GPIO_CFG(13, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_grn6 */
	GPIO_CFG(14, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_grn5 */
	GPIO_CFG(15, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_grn4 */
	GPIO_CFG(16, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_grn3 */
	GPIO_CFG(17, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_grn2 */
	GPIO_CFG(18, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_grn1 */
	GPIO_CFG(19, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_grn0 */
	GPIO_CFG(20, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_blu7 */
	GPIO_CFG(21, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_blu6 */
	GPIO_CFG(22, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_blu5 */
	GPIO_CFG(23, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_blu4 */
	GPIO_CFG(24, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_blu3 */
	GPIO_CFG(25, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_blu2 */
	GPIO_CFG(26, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_blu1 */
	GPIO_CFG(27, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_16MA), /* lcdc_blu0 */
};

static void lcdc_samsung_panel_power(int on)
{
	int n;

	/*TODO if on = 0 free the gpio's */
	for (n = 0; n < ARRAY_SIZE(lcd_panel_gpios); ++n)
		gpio_tlmm_config(lcd_panel_gpios[n], 0);
}

static int lcdc_panel_power(int on)
{
	int flag_on = !!on;
	static int lcdc_power_save_on;

	if (lcdc_power_save_on == flag_on)
		return 0;

	lcdc_power_save_on = flag_on;

	lcdc_samsung_panel_power(on);

	return 0;
}


#ifdef CONFIG_MSM_BUS_SCALING


static struct msm_bus_vectors rotator_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 0,
		.ib = 0,
	},
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
		},
	};

static struct msm_bus_vectors rotator_ui_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = (1024 * 600 * 4 * 2 * 60),
		.ib  = (1024 * 600 * 4 * 2 * 60 * 1.5),
	},
};

static struct msm_bus_vectors rotator_vga_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = (640 * 480 * 2 * 2 * 30),
		.ib  = (640 * 480 * 2 * 2 * 30 * 1.5),
	},
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = (640 * 480 * 2 * 2 * 30),
		.ib  = (640 * 480 * 2 * 2 * 30 * 1.5),
	},
};

static struct msm_bus_vectors rotator_720p_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = (1280 * 736 * 2 * 2 * 30),
		.ib  = (1280 * 736 * 2 * 2 * 30 * 1.5),
	},
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = (1280 * 736 * 2 * 2 * 30),
		.ib  = (1280 * 736 * 2 * 2 * 30 * 1.5),
	},
};

static struct msm_bus_vectors rotator_1080p_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = (1920 * 1088 * 2 * 2 * 30),
		.ib  = (1920 * 1088 * 2 * 2 * 30 * 1.5),
	},
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = (1920 * 1088 * 2 * 2 * 30),
		.ib  = (1920 * 1088 * 2 * 2 * 30 * 1.5),
	},
};

static struct msm_bus_paths rotator_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(rotator_init_vectors),
		rotator_init_vectors,
	},
	{
		ARRAY_SIZE(rotator_ui_vectors),
		rotator_ui_vectors,
	},
	{
		ARRAY_SIZE(rotator_vga_vectors),
		rotator_vga_vectors,
	},
	{
		ARRAY_SIZE(rotator_720p_vectors),
		rotator_720p_vectors,
	},
	{
		ARRAY_SIZE(rotator_1080p_vectors),
		rotator_1080p_vectors,
	},
};

struct msm_bus_scale_pdata rotator_bus_scale_pdata = {
	rotator_bus_scale_usecases,
	ARRAY_SIZE(rotator_bus_scale_usecases),
	.name = "rotator",
};


#if 0
static struct msm_bus_vectors mdp_init_vectors[] = {
	/* For now, 0th array entry is reserved.
	 * Please leave 0 as is and don't use it
	 */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 0,
		.ib = 0,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors mdp_sd_smi_vectors[] = {
	/* Default case static display/UI/2d/3d if FB SMI */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 147460000,
		.ib = 184325000,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors mdp_sd_ebi_vectors[] = {
	/* Default case static display/UI/2d/3d if FB SMI */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 0,
		.ib = 0,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 334080000,
		.ib = 417600000,
	},
};

static struct msm_bus_vectors mdp_vga_vectors[] = {
	/* VGA and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 175110000,
		.ib = 218887500,
	},
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 175110000,
		.ib = 218887500,
	},
};

static struct msm_bus_vectors mdp_720p_vectors[] = {
	/* 720p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 230400000,
		.ib = 288000000,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 230400000,
		.ib = 288000000,
	},
};

static struct msm_bus_vectors mdp_1080p_vectors[] = {
	/* 1080p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 334080000,
		.ib = 417600000,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 334080000,
		.ib = 417600000,
	},
};
#endif

static struct msm_bus_vectors mdp_init_vectors[] = {
/* For now, 0th array entry is reserved.
 * Please leave 0 as is and don't use it
 */
{
.src = MSM_BUS_MASTER_MDP_PORT0,
.dst = MSM_BUS_SLAVE_SMI,
.ab = 0,
.ib = 0,
},
/* Master and slaves can be from different fabrics */
{
.src = MSM_BUS_MASTER_MDP_PORT0,
.dst = MSM_BUS_SLAVE_EBI_CH0,
.ab = 0,
.ib = 0,
},
};


static struct msm_bus_vectors mdp_sd_smi_vectors[] = {
/* Default case static display/UI/2d/3d if FB SMI */
{
.src = MSM_BUS_MASTER_MDP_PORT0,
.dst = MSM_BUS_SLAVE_SMI,
.ab = 175110000,
.ib = 218887500,
},
/* Master and slaves can be from different fabrics */
{
.src = MSM_BUS_MASTER_MDP_PORT0,
.dst = MSM_BUS_SLAVE_EBI_CH0,
.ab = 0,
.ib = 0,
},
};

static struct msm_bus_vectors mdp_sd_ebi_vectors[] = {
/* Default case static display/UI/2d/3d if FB SMI */
{
.src = MSM_BUS_MASTER_MDP_PORT0,
.dst = MSM_BUS_SLAVE_SMI,
.ab = 0,
.ib = 0,
},
/* Master and slaves can be from different fabrics */
{
.src = MSM_BUS_MASTER_MDP_PORT0,
.dst = MSM_BUS_SLAVE_EBI_CH0,
.ab = 216000000,
.ib = 270000000 * 2,
},
};
static struct msm_bus_vectors mdp_vga_vectors[] = {
/* VGA and less video */
{
.src = MSM_BUS_MASTER_MDP_PORT0,
.dst = MSM_BUS_SLAVE_SMI,
.ab = 216000000,
.ib = 270000000,
},
{
.src = MSM_BUS_MASTER_MDP_PORT0,
.dst = MSM_BUS_SLAVE_EBI_CH0,
.ab = 216000000,
.ib = 270000000 * 2,
},
};

static struct msm_bus_vectors mdp_720p_vectors[] = {
/* 720p and less video */
{
.src = MSM_BUS_MASTER_MDP_PORT0,
.dst = MSM_BUS_SLAVE_SMI,
.ab = 230400000,
.ib = 288000000,
},
/* Master and slaves can be from different fabrics */
{
.src = MSM_BUS_MASTER_MDP_PORT0,
.dst = MSM_BUS_SLAVE_EBI_CH0,
.ab = 230400000,
.ib = 288000000 * 2,
},
};

static struct msm_bus_vectors mdp_1080p_vectors[] = {
/* 1080p and less video */
{
.src = MSM_BUS_MASTER_MDP_PORT0,
.dst = MSM_BUS_SLAVE_SMI,
.ab = 334080000,
.ib = 417600000,
},
/* Master and slaves can be from different fabrics */
{
.src = MSM_BUS_MASTER_MDP_PORT0,
.dst = MSM_BUS_SLAVE_EBI_CH0,
.ab = 334080000,
.ib = 550000000 * 2,
},
};




static struct msm_bus_paths mdp_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(mdp_init_vectors),
		mdp_init_vectors,
	},
	{
		ARRAY_SIZE(mdp_sd_smi_vectors),
		mdp_sd_smi_vectors,
	},
	{
		ARRAY_SIZE(mdp_sd_ebi_vectors),
		mdp_sd_ebi_vectors,
	},
	{
		ARRAY_SIZE(mdp_vga_vectors),
		mdp_vga_vectors,
	},
	{
		ARRAY_SIZE(mdp_720p_vectors),
		mdp_720p_vectors,
	},
	{
		ARRAY_SIZE(mdp_1080p_vectors),
		mdp_1080p_vectors,
	},
};
static struct msm_bus_scale_pdata mdp_bus_scale_pdata = {
	mdp_bus_scale_usecases,
	ARRAY_SIZE(mdp_bus_scale_usecases),
	.name = "mdp",
};
#endif

#ifdef CONFIG_FB_MSM_TVOUT
#ifdef CONFIG_MSM_BUS_SCALING
static struct msm_bus_vectors atv_bus_init_vectors[] = {
	/* For now, 0th array entry is reserved.
	 * Please leave 0 as is and don't use it
	 */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 0,
		.ib = 0,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};
static struct msm_bus_vectors atv_bus_def_vectors[] = {
	/* For now, 0th array entry is reserved.
	 * Please leave 0 as is and don't use it
	 */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 236390400,
		.ib = 265939200,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 236390400,
		.ib = 265939200,
	},
};
static struct msm_bus_paths atv_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(atv_bus_init_vectors),
		atv_bus_init_vectors,
	},
	{
		ARRAY_SIZE(atv_bus_def_vectors),
		atv_bus_def_vectors,
	},
};
static struct msm_bus_scale_pdata atv_bus_scale_pdata = {
	atv_bus_scale_usecases,
	ARRAY_SIZE(atv_bus_scale_usecases),
	.name = "atv",
};
#endif
#endif

static char mipi_dsi_splash_is_enabled(void);
static int mipi_panel_power(int on)
{
	int flag_on = !!on;
	static int mipi_power_save_on;

	if (mipi_power_save_on == flag_on)
		return 0;

	mipi_power_save_on = flag_on;

	doubleshot_panel_power(on);

	return 0;
}

static struct lcdc_platform_data lcdc_pdata = {
	.lcdc_power_save   = lcdc_panel_power,
};

#ifdef CONFIG_FB_MSM_TVOUT
static struct regulator *reg_8058_l13;

static int atv_dac_power(int on)
{
	int rc = 0;
	pr_info("%s: on/off=%d\n", __func__, on);

	#define _GET_REGULATOR(var, name) do {				\
		var = regulator_get(NULL, name);			\
		if (IS_ERR(var)) {					\
			pr_err("'%s' regulator not found, rc=%ld\n",	\
				name, IS_ERR(var));			\
			var = NULL;					\
			return -ENODEV;					\
		}							\
	} while (0)

	if (!reg_8058_l13)
		_GET_REGULATOR(reg_8058_l13, "8058_l13");
	#undef _GET_REGULATOR

	if (on) {
		//gpio_set_value(PM8058_GPIO_PM_TO_SYS(DOUBLESHOT_TVOUT_SW - 1), 1);
		rc = regulator_set_voltage(reg_8058_l13, 2050000, 2050000);
		if (rc) {
			pr_err("%s: '%s' regulator set voltage failed,\
				rc=%d\n", __func__, "8058_l13", rc);
			goto end;
		}

		rc = regulator_enable(reg_8058_l13);
		if (rc) {
			pr_err("%s: '%s' regulator enable failed,\
				rc=%d\n", __func__, "8058_l13", rc);
			goto end;
		}
	} else {
		//gpio_set_value(PM8058_GPIO_PM_TO_SYS(DOUBLESHOT_TVOUT_SW - 1), 0);
		rc = regulator_force_disable(reg_8058_l13);
		if (rc)
			pr_warning("%s: '%s' regulator disable failed, rc=%d\n",
				__func__, "8058_l13", rc);
	}
end:
	return rc;
}
#endif

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio = 28,
	.dsi_power_save   = mipi_panel_power,
	.splash_is_enabled = mipi_dsi_splash_is_enabled,
};

#ifdef CONFIG_FB_MSM_TVOUT
static struct tvenc_platform_data atv_pdata = {
	.poll		 = 0,
	.pm_vid_en	 = atv_dac_power,
#ifdef CONFIG_MSM_BUS_SCALING
	.bus_scale_table = &atv_bus_scale_pdata,
#endif
};
#endif

#define GPIO_BACKLIGHT_PWM0 0
#define GPIO_BACKLIGHT_PWM1 1

static int pmic_backlight_gpio[2]
= { GPIO_BACKLIGHT_PWM0, GPIO_BACKLIGHT_PWM1 };
static struct msm_panel_common_pdata lcdc_samsung_panel_data = {
	.gpio_num = pmic_backlight_gpio, /* two LPG CHANNELS for backlight */
};

static struct platform_device lcdc_samsung_panel_device = {
	.name = "lcdc_samsung_wsvga",
	.id = 0,
	.dev = {
		.platform_data = &lcdc_samsung_panel_data,
	}
};

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 143
#define BRI_SETTING_MAX                 255

#define PWM_MIN              		9	/* 3.5% of max pwm */
#define PWM_DEFAULT			140	/* 55% of max pwm  */
#define PWM_MAX				255	/* 100% of max pwm */

#if 0
static unsigned char doubleshot_shrink_pwm(int val)
{
	unsigned char shrink_br;
	if (val <= 0) {
		shrink_br = 0;
	} else if (val > 0 && (val < BRI_SETTING_MIN))
		shrink_br = PWM_MIN;
	else if ((val >= BRI_SETTING_MIN) && (val <= BRI_SETTING_DEF))
		shrink_br = (PWM_DEFAULT - PWM_MIN) * (val - BRI_SETTING_MIN) /
		(BRI_SETTING_DEF - BRI_SETTING_MIN) + PWM_MIN;
	else if ((val > BRI_SETTING_DEF) && (val <= BRI_SETTING_MAX))
		shrink_br = (PWM_MAX - PWM_DEFAULT) * (val - BRI_SETTING_DEF) /
		(BRI_SETTING_MAX - BRI_SETTING_DEF) + PWM_DEFAULT;
	else
		shrink_br = PWM_MAX;

	pr_debug("%s: brightness orig=%d, transformed=%d\n", __func__, val, shrink_br);
	return shrink_br;
}
#endif

/*
static struct msm_panel_common_pdata mipi_panel_data = {
	.gpio = MDP_VSYNC_GPIO,
	.mdp_max_clk = 200000000,
#ifdef CONFIG_MSM_BUS_SCALING
	.mdp_bus_scale_table = &mdp_bus_scale_pdata,
#endif
	.mdp_rev = MDP_REV_41,
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
	.mem_hid = BIT(ION_CP_WB_HEAP_ID),
#else
	.mem_hid = MEMTYPE_EBI1,
#endif
	.cont_splash_enabled = 0x01,
	.shrink_pwm = doubleshot_shrink_pwm,
};

static struct platform_device mipi_dsi_cmd_wvga_panel_device = {
	.name = "mipi_novatek",
	.id = 0,
	.dev = {
		.platform_data = &mipi_panel_data,
	}
};
*/


static int msm_fb_detect_panel(const char *name)
{
	if (panel_type == PANEL_ID_DOT_HITACHI) {
		if (!strcmp(name, "mipi_cmd_renesas_wvga"))
		return 0;
	} else if (panel_type == PANEL_ID_DOT_SONY ||
		panel_type == PANEL_ID_DOT_SONY_C3) {
		if (!strcmp(name, "mipi_cmd_novatek_wvga"))
			return 0;
	}
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
	else if (!strcmp(name, "hdmi_msm"))
		return 0;
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */

	pr_warning("%s: not supported '%s'", __func__, name);
	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.dev.platform_data = &msm_fb_pdata,
};

#ifdef CONFIG_FB_MSM_OVERLAY0_WRITEBACK
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((1376 * 768 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY0_WRITEBACK */

#ifdef CONFIG_FB_MSM_OVERLAY1_WRITEBACK
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE roundup((1920 * 1088 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY1_WRITEBACK */

// PANEL common data

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = MDP_VSYNC_GPIO,
	.mdp_max_clk = 200000000,
#ifdef CONFIG_MSM_BUS_SCALING
	.mdp_bus_scale_table = &mdp_bus_scale_pdata,
#endif
	.mdp_rev = MDP_REV_41,
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
	.mem_hid = BIT(ION_CP_WB_HEAP_ID),
#else
	.mem_hid = MEMTYPE_EBI1,
#endif
	.ov0_wb_size = MSM_FB_OVERLAY0_WRITEBACK_SIZE,
	.ov1_wb_size = MSM_FB_OVERLAY1_WRITEBACK_SIZE,
};


// panel platform data
#define FPGA_3D_GPIO_CONFIG_ADDR	0xB5

static struct mipi_dsi_phy_ctrl dsi_novatek_cmd_mode_phy_db = {

/* DSI_BIT_CLK at 500MHz, 2 lane, RGB888 */
       {0x0F, 0x0a, 0x04, 0x00, 0x20}, /* regulator */
       /* timing   */
       {0xab, 0x8a, 0x18, 0x00, 0x92, 0x97, 0x1b, 0x8c,
       0x0c, 0x03, 0x04, 0xa0},
       {0x5f, 0x00, 0x00, 0x10},	/* phy ctrl */
       {0xff, 0x00, 0x06, 0x00},	/* strength */
       /* pll control */
       {0x40, 0xf9, 0x30, 0xda, 0x00, 0x40, 0x03, 0x62,
       0x40, 0x07, 0x03,
       0x00, 0x1a, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01},
};

static struct mipi_dsi_panel_platform_data novatek_pdata = {
       .fpga_3d_config_addr  = FPGA_3D_GPIO_CONFIG_ADDR,
       .fpga_ctrl_mode = FPGA_SPI_INTF,
       .phy_ctrl_settings = &dsi_novatek_cmd_mode_phy_db,
};

static struct platform_device mipi_dsi_novatek_panel_device = {
       .name = "mipi_novatek",
       .id = 0,
       .dev = {
              .platform_data = &novatek_pdata,
       }
};


static void __init msm_fb_add_devices(void)
{
	printk(KERN_INFO "panel ID= 0x%x ? sony %x sc3 %x hitachi %x \n", panel_type,PANEL_ID_DOT_SONY,PANEL_ID_DOT_SONY_C3,PANEL_ID_DOT_HITACHI);
	msm_fb_register_device("mdp", &mdp_pdata);

	msm_fb_register_device("lcdc", &lcdc_pdata);
	if (panel_type != PANEL_ID_NONE)
		msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
#ifdef CONFIG_FB_MSM_TVOUT
	msm_fb_register_device("tvenc", &atv_pdata);
	msm_fb_register_device("tvout_device", NULL);
	atv_dac_power(0);
#endif
}

static char mipi_dsi_splash_is_enabled(void)
{
	return mdp_pdata.cont_splash_enabled;
}


/*
TODO:
1.find a better way to handle msm_fb_resources, to avoid passing it across file.
2.error handling
 */
int __init dot_init_panel(struct resource *res, size_t size)
{
	int ret;

	pr_info("%s: res=%p, size=%d\n", __func__, res, size);
	//mipi_panel_data.shrink_pwm = doubleshot_shrink_pwm;
	if (panel_type == PANEL_ID_DOT_HITACHI)
//		mipi_dsi_cmd_wvga_panel_device.name = "mipi_renesas";
		mipi_dsi_novatek_panel_device.name = "mipi_renesas";
//	pr_info("%s: %s\n", __func__, mipi_dsi_cmd_wvga_panel_device.name);

	msm_fb_device.resource = res;
	msm_fb_device.num_resources = size;

	ret = platform_device_register(&msm_fb_device);
	ret = platform_device_register(&lcdc_samsung_panel_device);
	//ret = platform_device_register(&mipi_dsi_cmd_wvga_panel_device);
	ret = platform_device_register(&mipi_dsi_novatek_panel_device);

	msm_fb_add_devices();

	return 0;
}
