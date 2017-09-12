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

#include <linux/mdio.h>
#include <net/if.h>

#include "phytool.h"

static const char *mv6_model_str(uint16_t id)
{
	static char str[32];

	switch (id & 0xfff0) {
	case 0x0480:
		return "mv88e6046";
	case 0x0950:
		return "mv88e6095";
	case 0x0990:
		return "mv88e6097";
	case 0x1a70:
		return "mv88e6185";
	case 0x3520:
		return "mv88e6352";
	}

	snprintf(str, sizeof(str), "UNKNOWN(%#.4x)", id);
	return str;
}

static const char *mv6_dev_str(uint16_t dev)
{
	static char str[32];

	if (dev < 0xf)
		snprintf(str, sizeof(str), "phy:%d", dev);
	else if (dev == 0xf)
		return "serdes";
	else if (dev)
		snprintf(str, sizeof(str), "port:%d", dev - 0x10);
	else if (dev == 0x1b)
		return "global:1";
	else if (dev == 0x1c)
		return "global:2";
	else if (dev == 0x1d)
		return "global:3";
	else
		snprintf(str, sizeof(str), "port:RESERVED(%#.2x)", dev);

	return str;
}

static void print_mv6_heading(const struct loc *loc, int indent)
{
	int port = loc_c45_port(loc), dev = loc_c45_dev(loc);
	struct loc loc_id = *loc;
	uint16_t id;

	loc_id.phy_id = mdio_phy_id_c45(port, 0x10);
	loc_id.reg = 3;
	id = phy_read(&loc_id);

	printf("%*smv6: model:%s dev:%d %s\n", indent, "",
	       mv6_model_str(id), port /* [sic] */, mv6_dev_str(dev));
}

static void mv6_port_ps(uint16_t val, int indent)
{
	int speed = 10;
	int mult = (val & 0x0300) >> 8;

	while (mult) {
		speed *= 10;
		mult--;
	}

	printf("%*smv6: reg:PS(0x00) val:%#.4x\n", indent, "", val);

	print_attr_name("flags", indent + INDENT);
	print_bool("pause-en", val & 0x8000);
	putchar(' ');

	print_bool("my-pause", val & 0x4000);
	putchar(' ');

	print_bool("phy-detect", val & 0x1000);
	putchar(' ');

	print_bool("link", val & 0x0800);
	putchar(' ');

	print_bool("eee", val & 0x0040);
	putchar(' ');

	print_bool("tx-paused", val & 0x0020);
	putchar(' ');

	print_bool("flow-ctrl", val & 0x0010);
	putchar('\n');

	print_attr_name("speed", indent + INDENT);
	printf("%d-%s\n", speed, (val & 0x0400)? "full" : "half");
	print_attr_name("mode", indent + INDENT);
	printf("%#x\n", val & 0xf);
}

static const char *mv6_egress_mode_str[] = {
	"00, unmodified",
	"01, untagged",
	"10, tagged",
	"11, reserved"
};

static const char *mv6_frame_mode_str[] = {
	"00, normal",
	"01, DSA",
	"10, provider",
	"11, ether type DSA"
};

static const char *mv6_initial_pri_str[] = {
	"00, port defaults",
	"01, tag prio",
	"10, IP prio",
	"11, tag & IP prio"
};

static const char *mv6_egress_floods_str[] = {
	"00, deny UC & MC",
	"01, allow UC",
	"10, allow MC",
	"11, allow UC & MC"
};

static const char *mv6_port_state_str[] = {
	"00, disabled",
	"01, blocking",
	"10, learning",
	"11, forwarding"
};

static void mv6_port_pc(uint16_t val, int indent)
{
	int speed = 10;
	int mult = (val & 0x0300) >> 8;

	while (mult) {
		speed *= 10;
		mult--;
	}

	printf("%*smv6: reg:PC(0x04) val:%#.4x\n", indent, "", val);

	print_attr_name("flags", indent + INDENT);
	print_bool("router-header", val & 0x0800);
	putchar(' ');

	print_bool("igmp-snoop", val & 0x0400);
	putchar(' ');

	print_bool("vlan-tunnel", val & 0x0080);
	putchar(' ');

	print_bool("tag-if-both", val & 0x0040);
	putchar('\n');

	print_attr_name("egress-mode", indent + INDENT);
	puts(mv6_egress_mode_str[(val & 0x3000) >> 12]);

	print_attr_name("frame-mode", indent + INDENT);
	puts(mv6_frame_mode_str[(val & 0x0300) >> 8]);

	print_attr_name("initial-pri", indent + INDENT);
	puts(mv6_initial_pri_str[(val & 0x0030) >> 4]);

	print_attr_name("egress-floods", indent + INDENT);
	puts(mv6_egress_floods_str[(val & 0x000c) >> 2]);

	print_attr_name("port-state", indent + INDENT);
	puts(mv6_port_state_str[(val & 0x0003)]);
}

struct mv6_port_desc {
	int summary[32];
	void (*printer[32])(uint16_t val, int indent);
};

struct mv6_port_desc mv6_pd_port = {
	.summary = { 0, 4, -1 },
	.printer = {
		mv6_port_ps,
		NULL,
		NULL,
		NULL,
		mv6_port_pc,
	},
};

struct mv6_port_desc mv6_pd_serdes = {
	.summary = { -1 },
	.printer = {
	},
};

struct mv6_port_desc mv6_pd_g1 = {
	.summary = { -1 },
	.printer = {
	},
};

struct mv6_port_desc mv6_pd_g2 = {
	.summary = { -1 },
	.printer = {
	},
};

struct mv6_port_desc mv6_pd_g3 = {
	.summary = { -1 },
	.printer = {
	},
};

int mv6_port_one(const struct loc *loc, int indent, struct mv6_port_desc *pd)
{
	int val = phy_read(loc);

	if (val < 0)
		return val;

	if (loc->reg > 0x1f || !pd || !pd->printer[loc->reg])
		printf("%*smv6: reg:%#.2x val:%#.4x\n", indent, "",
		       loc->reg, val);
	else
		pd->printer[loc->reg](val, indent);

	return 0;
}

int print_mv6_port(const struct loc *loc, int indent, struct mv6_port_desc *pd)
{
	struct loc loc_sum = *loc;
	int i;

	if (loc->reg != REG_SUMMARY)
		return mv6_port_one(loc, indent, pd);

	for (i = 0; pd->summary[i] >= 0; i++) {
		loc_sum.reg = pd->summary[i];
		mv6_port_one(&loc_sum, indent, pd);

		putchar('\n');
	}

	return 0;
}

int print_mv6tool(const struct loc *loc, int indent)
{
	int dev = loc_c45_dev(loc);
	struct mv6_port_desc *pd = NULL;

	if (!loc_is_c45(loc)) {
		fprintf(stderr, "error: PHY must be a C45 dev:port pair\n");
		return 1;
	}

	print_mv6_heading(loc, indent);

	if (dev < 0xf)
		return print_phytool(loc, indent + INDENT);
	else if (dev == 0xf)
		pd = &mv6_pd_serdes;
	else if (dev < 0x1b)
		pd = &mv6_pd_port;
	else if (dev == 0x1b)
		pd = &mv6_pd_g1;
	else if (dev == 0x1c)
		pd = &mv6_pd_g2;
	else if (dev == 0x1d)
		pd = &mv6_pd_g3;

	return print_mv6_port(loc, indent + INDENT, pd);
}
