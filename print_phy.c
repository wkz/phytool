#include <stdint.h>
#include <stdio.h>

#include <linux/mdio.h>
#include <net/if.h>

#include "phytool.h"

const char hi[] = { 0xe2, 0x97, 0x89, 0x00 };
const char lo[] = { 0xe2, 0x97, 0x8b, 0x00 };

void print_bit_array(uint16_t val, int indent)
{
	int i;

	printf("%*s", indent, "");

	for (i = 15; i >= 0; i--) {
		fputs((val & (1 << i))? hi : lo, stdout);

		if (i)
			fputs((i % 4 == 0)? "  " : " ", stdout);
	}

	printf("\n%*s   %x        %x        %x        %x\n", indent, "",
	       (val & 0xf000) >> 12, (val & 0x0f00) >> 8,
	       (val & 0x00f0) >>  4, (val & 0x000f));
}

void print_bool(const char *name, int on)
{
	if (on)
		fputs("\e[7m", stdout);

	fputs(name, stdout);

	if (on)
		fputs("\e[0m", stdout);
}

static void ieee_bmcr(uint16_t val, int indent)
{
	int speed = 10;

	if (val & BMCR_SPEED100)
		speed = 100;
	if (val & BMCR_SPEED1000)
		speed = 1000;

	printf("%*sieee-phy: reg:BMCR(0x00) val:%#.4x\n", indent, "", val);

	printf("%*sflags: ", indent + INDENT, "");
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

	printf("%*sspeed: %d-%s\n", indent + INDENT, "", speed,
	       (val & BMCR_FULLDPLX) ? "full" : "half");
}

static void ieee_bmsr(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg:BMSR(0x01) val:%#.4x\n", indent, "", val);

	printf("%*slink capabilities: ", indent + INDENT, "");
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

	printf("%*sflags: ", indent + INDENT, "");
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

	print_bit_array(val, indent + INDENT);
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
