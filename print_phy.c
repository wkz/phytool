/* This file is part of phytool
 *
 * phytool is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * phytool is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with phytool.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <linux/mdio.h>
#include <net/if.h>

#include "phytool.h"

void print_bool(const char *name, int on)
{
	if (on)
		fputs("\e[1m+", stdout);
	else
		fputs("-", stdout);

	fputs(name, stdout);

	if (on)
		fputs("\e[0m", stdout);
}

void print_attr_name(const char *name, int indent)
{
	int len;

	printf("%*s%s:", indent, "", name);

	len = strlen(name) + 1;
	printf("%*s", (len > 16) ? 0 : 16 - len, "");
}

static void ieee_bmcr(uint16_t val, int indent)
{
	int speed = 10;

	if (val & BMCR_SPEED100)
		speed = 100;
	if (val & BMCR_SPEED1000)
		speed = 1000;

	printf("%*sieee-phy: reg:BMCR(0x00) val:%#.4x\n", indent, "", val);

	print_attr_name("flags", indent + INDENT);
	print_bool("reset", val & BMCR_RESET);
	putchar(' ');

	print_bool("loopback", val & BMCR_LOOPBACK);
	putchar(' ');

	print_bool("aneg-enable", val & BMCR_ANENABLE);
	putchar(' ');

	print_bool("power-down", val & BMCR_PDOWN);
	putchar(' ');

	print_bool("isolate", val & BMCR_ISOLATE);
	putchar(' ');

	print_bool("aneg-restart", val & BMCR_ANRESTART);
	putchar(' ');

	print_bool("collision-test", val & BMCR_CTST);
	putchar('\n');

	print_attr_name("speed", indent + INDENT);
	printf("%d-%s\n", speed, (val & BMCR_FULLDPLX) ? "full" : "half");
}

static void ieee_bmsr(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg:BMSR(0x01) val:%#.4x\n", indent, "", val);

	print_attr_name("capabilities", indent + INDENT);
	print_bool("100-b4", val & BMSR_100BASE4);
	putchar(' ');

	print_bool("100-f", val & BMSR_100FULL);
	putchar(' ');

	print_bool("100-h", val & BMSR_100HALF);
	putchar(' ');

	print_bool("10-f", val & BMSR_10FULL);
	putchar(' ');

	print_bool("10-h", val & BMSR_10HALF);
	putchar(' ');

	print_bool("100-t2-f", val & BMSR_100FULL2);
	putchar(' ');

	print_bool("100-t2-h", val & BMSR_100HALF2);
	putchar('\n');

	print_attr_name("flags", indent + INDENT);
	print_bool("ext-status", val & BMSR_ESTATEN);
	putchar(' ');

	print_bool("aneg-complete", val & BMSR_ANEGCOMPLETE);
	putchar(' ');

	print_bool("remote-fault", val & BMSR_RFAULT);
	putchar(' ');

	print_bool("aneg-capable", val & BMSR_ANEGCAPABLE);
	putchar(' ');

	print_bool("link", val & BMSR_LSTATUS);
	putchar(' ');

	print_bool("jabber", val & BMSR_JCD);
	putchar(' ');

	print_bool("ext-register", val & BMSR_ERCAP);
	putchar('\n');
}

static void (*ieee_reg_printers[32])(uint16_t, int) = {
	ieee_bmcr,
	ieee_bmsr,
};

static int ieee_one(const struct loc *loc, int indent)
{
	int val = phy_read(loc);

	if (val < 0)
		return val;

	if (loc->reg > 0x1f || !ieee_reg_printers[loc->reg])
		printf("%*sieee-phy: reg:%#.2x val:%#.4x\n", indent, "",
		       loc->reg, val);
	else
		ieee_reg_printers[loc->reg](val, indent);

	return 0;
}

int print_phy_ieee(const struct loc *loc, int indent)
{
	struct loc loc_sum = *loc;

	if (loc->reg != REG_SUMMARY)
		return ieee_one(loc, indent);

	printf("%*sieee-phy: id:%#.8x\n\n", indent, "", phy_id(loc));

	loc_sum.reg = 0;
	ieee_one(&loc_sum, indent + INDENT);

	putchar('\n');

	loc_sum.reg = 1;
	ieee_one(&loc_sum, indent + INDENT);
	return 0;
}

struct printer {
	uint32_t id;
	uint32_t mask;

	int (*print)(const struct loc *loc, int indent);
};

struct printer printer[] = {
	/* { .id = 0x01410eb0, .mask = 0xffffff0, .print = print_mv1112 }, */

	{ .id = 0, .mask = 0, .print = print_phy_ieee },
	{ .print = NULL }
};

int print_phytool(const struct loc *loc, int indent)
{
	struct printer *p;
	uint32_t id = phy_id(loc);

	for (p = printer; p->print; p++)
		if ((id & p->mask) == p->id)
			return p->print(loc, indent);

	return -1;
}
