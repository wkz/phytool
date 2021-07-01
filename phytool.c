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

#define _GNU_SOURCE

#include <glob.h>
#include <errno.h>
#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <linux/ethtool.h>
#include <linux/mdio.h>
#include <linux/sockios.h>

#include "phytool.h"

extern char *__progname;

struct applet {
	const char *name;
	int (*usage)(int code);
	int (*parse_loc)(char *text, struct loc *loc, int strict);
	int (*print)(const struct loc *loc, int indent);
};


static int __phy_op(const struct loc *loc, uint16_t *val, int cmd)
{
	static int sd = -1;

	struct ifreq ifr;
	struct mii_ioctl_data* mii = (struct mii_ioctl_data *)(&ifr.ifr_data);
	int err;

	if (sd < 0)
		sd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sd < 0)
		return sd;

	strncpy(ifr.ifr_name, loc->ifnam, sizeof(ifr.ifr_name));

	mii->phy_id  = loc->phy_id;
	mii->reg_num = loc->reg;
	mii->val_in  = *val;
	mii->val_out = 0;

	err = ioctl(sd, cmd, &ifr);
	if (err)
		return -errno;

	*val = mii->val_out;
	return 0;
}

int phy_read(const struct loc *loc)
{
	uint16_t val = 0;
	int err = __phy_op(loc, &val, SIOCGMIIREG);

	if (err) {
		fprintf(stderr, "error: phy_read (%d)\n", err);
		return err;
	}

	return val;
}

int phy_write(const struct loc *loc, uint16_t val)
{
	int err = __phy_op(loc, &val, SIOCSMIIREG);

	if (err)
		fprintf(stderr, "error: phy_write (%d)\n", err);

	return err;
}

uint32_t phy_id(const struct loc *loc)
{
	struct loc loc_id = *loc;
	uint16_t id[2];

	loc_id.reg = MII_PHYSID1;
	id[0] = phy_read(&loc_id);
	
	loc_id.reg = MII_PHYSID2;
	id[1] = phy_read(&loc_id);

	return (id[0] << 16) | id[1];
}

static int parse_phy_id(char *text, uint16_t *phy_id)
{
	unsigned long port, dev;
	char *end;

	port = strtoul(text, &end, 0);
	if (!end[0]) {
		/* simple phy address */
		*phy_id = port;
		return 0;
	}

	if (end[0] != ':') {
		/* not clause 45 either */
		return 1;
	}

	dev = strtoul(end + 1, &end, 0);
	if (end[0])
		return 1;

	*phy_id = mdio_phy_id_c45(port, dev);
	return 0;
}

static int sysfs_readu(const char *path, int *result)
{
	FILE *fp;
	char line[24];

	fp = fopen(path, "r");
	if (!fp)
		return -EIO;

	if (!fgets(line, sizeof(line), fp)) {
		fclose(fp);
		return -EIO;
	}

	/* limit to base ten here, output is zero padded */
	*result = strtol(line, NULL, 10);
	fclose(fp);
	return (*result >= 0) ? 0 : -EINVAL;
}

static int parse_switch_id(const char *dev, int *swid, char *ifnam)
{
	glob_t paths;
	char *src;
	int cmp, err, i;

	*swid = strtol(dev, NULL, 0);
	if (*swid < 0)
		return -EINVAL;

	err = glob("/sys/class/net/*/phys_switch_id", 0, NULL, &paths);
	if (err)
		return -ENOENT;

	err = -ENOENT;
	for (i = 0; i < (int)paths.gl_pathc; i++) {
		err = sysfs_readu(paths.gl_pathv[i], &cmp);
		if (err)
			continue;

		if (cmp == *swid) {
			err = 0;
			break;
		}
	}

	if (!err) {
		src = &paths.gl_pathv[i][strlen("/sys/class/net/")];
		strncpy(ifnam, src, IFNAMSIZ - 1);
		strtok(ifnam, "/");
	}

	globfree(&paths);
	return err;
}

static int parse_switch_addr(const char *addr, int *swaddr)
{
	const char *num;
	int offs;

	if (strstr(addr, "phy") == addr) {
		num = &addr[3];
		offs = 0x0;
	} else if (strstr(addr, "port") == addr) {
		num = &addr[4];
		offs = 0x10;
	} else if (!strcmp(addr, "serdes")) {
		*swaddr = 0xf;
		return 0;
	} else if (strstr(addr, "global") == addr) {
		num = &addr[6];
		offs = 0x1a;
	} else {
		num = addr;
		offs = 0x0;
	}

	*swaddr = strtol(num, NULL, 0);
	if (*swaddr < 0)
		return -EINVAL;

	*swaddr += offs;
	return 0;
}

static int loc_segments(char *text, char **a, char **b, char **c)
{
	*a = strtok(text, "/");
	if (!*a)
		return 0;

	*b = strtok(NULL, "/");
	if (!*b)
		return 1;

	*c = strtok(NULL, "/");
	if (!*c)
		return 2;

	return 3;
}

static int phytool_parse_loc_segs(char *dev, char *addr, char *reg,
				  struct loc *loc)
{
	int err;

	strncpy(loc->ifnam, dev, IFNAMSIZ - 1);

	err = parse_phy_id(addr, &loc->phy_id);
	if (err)
		return err;

	loc->reg = reg ? strtoul(reg, NULL, 0) : REG_SUMMARY;
	return 0;
}

static int phytool_parse_loc(char *text, struct loc *loc, int strict)
{
	char *dev = NULL, *addr = NULL, *reg = NULL;
	int segs;

	segs = loc_segments(text, &dev, &addr, &reg);
	if (segs < (strict ? 3 : 2))
		return -EINVAL;

	return phytool_parse_loc_segs(dev, addr, reg, loc);
}

static int mv6tool_parse_loc_if(char *dev, char *addr, char *reg,
				struct loc *loc)
{
	char *path;
	int err, phy_port, phy_dev;

	strncpy(loc->ifnam, dev, IFNAMSIZ - 1);

	asprintf(&path, "/sys/class/net/%s/phys_switch_id", dev);
	err = sysfs_readu(path, &phy_port);
	free(path);
	if (err)
		return -ENOSYS;

	asprintf(&path, "/sys/class/net/%s/phys_port_id", dev);
	err = sysfs_readu(path, &phy_dev);
	free(path);
	if (err)
		return -ENOSYS;

	if (!addr || !strcmp(addr, "port"))
		phy_dev += 0x10;
	else if (!strcmp(addr, "phy"))
		phy_dev += 0;
	else
		return -EINVAL;

	loc->phy_id = mdio_phy_id_c45(phy_port, phy_dev);
	loc->reg = reg ? strtoul(reg, NULL, 0) : REG_SUMMARY;
	return 0;
}

static int mv6tool_parse_loc(char *text, struct loc *loc, int strict)
{
	char *dev = NULL, *addr = NULL, *reg = NULL;
	int phy_port, phy_dev;
	int err, segs;

	segs = loc_segments(text, &dev, &addr, &reg);
	if (segs < (strict ? 3 : 1))
		return -EINVAL;

	if (if_nametoindex(dev)) {
		err = mv6tool_parse_loc_if(dev, addr, reg, loc);
		if (!err)
			return 0;
	}

	if (segs < (strict ? 3 : 2))
		return -EINVAL;

	err = parse_switch_id(dev, &phy_port, loc->ifnam);
	if (err)
		goto fallback;

	err = parse_switch_addr(addr, &phy_dev);
	if (err)
		goto fallback;

	loc->phy_id = mdio_phy_id_c45(phy_port, phy_dev);
	loc->reg = reg ? strtoul(reg, NULL, 0) : REG_SUMMARY;
	return 0;
fallback:
	return phytool_parse_loc_segs(dev, addr, reg, loc);
}

static int phytool_read(struct applet *a, int argc, char **argv)
{
	struct loc loc;
	int val;

	if (!argc)
		return 1;

	if (a->parse_loc(argv[0], &loc, 1)) {
		fprintf(stderr, "error: bad location format\n");
		return 1;
	}

	val = phy_read (&loc);
	if (val < 0)
		return 1;

	printf("%#.4x\n", val);
	return 0;
}

static int phytool_write(struct applet *a, int argc, char **argv)
{
	struct loc loc;
	unsigned long val;
	int err;

	if (argc < 2)
		return 1;

	if (a->parse_loc(argv[0], &loc, 1)) {
		fprintf(stderr, "error: bad location format\n");
		return 1;
	}

	val = strtoul(argv[1], NULL, 0);

	err = phy_write (&loc, val);
	if (err)
		return 1;

	return 0;
}

static int phytool_print(struct applet *a, int argc, char **argv)
{
	struct loc loc;
	int err;

	if (!argc)
		return 1;

	if (a->parse_loc(argv[0], &loc, 0)) {
		fprintf(stderr, "error: bad location format\n");
		return 1;
	}

	err = a->print(&loc, 0);
	if (err)
		return 1;
	
	return 0;
}

static int phytool_usage(int code)
{
	printf("Usage: %s read  IFACE/ADDR/REG\n"
	       "       %s write IFACE/ADDR/REG <0-0xffff>\n"
	       "       %s print IFACE/ADDR[/REG]\n"
	       "\n"
	       "Clause 22:\n"
	       "\n"
	       "ADDR := <0-0x1f>\n"
	       "REG  := <0-0x1f>\n"
	       "\n"
	       "Clause 45 (not supported by all MDIO drivers):\n"
	       "\n"
	       "ADDR := PORT:DEV\n"
	       "PORT := <0-0x1f>\n"
	       "DEV  := <0-0x1f>\n"
	       "REG  := <0-0xffff>\n"
	       "\n"
	       "Examples:\n"
	       "       %s read  eth0/0:4/0x1000\n"
	       "       %s write eth0/0xa/0 0x1140\n"
	       "       %s print eth0/0x1c\n"
	       "\n"
	       "The `read` and `write` commands are simple register level\n"
	       "accessors. The `print` command will pretty-print a register. When\n"
	       "using the `print` command, the register is optional. If left out, the\n"
	       "most common registers will be shown.\n"
	       "\n"
	       "Bug report address: https://github.com/wkz/phytool/issues\n"
	       "\n",
	       __progname, __progname, __progname, __progname, __progname, __progname);
	return code;
}

static int mv6tool_usage(int code)
{
	printf("Usage: %s read  LOCATION/REG\n"
	       "       %s write LOCATION/REG <0-0xffff>\n"
	       "       %s print LOCATION[/REG]\n"
	       "       %s print IFACE\n"
	       "where\n"
	       "\n"
	       "LOCATION := IFACE/<port|phy> | DEV/<ADDR|phyN|portN|globalG|serdes>\n"
	       "\n"
	       "DEV  := <0-0x1f>\n"
	       "ADDR := <0-0x1f>\n"
	       "N    := <0-0xa>\n"
	       "G    := <0-2>\n"
	       "REG  := <0-0x1f>\n"
	       "\n"
	       "Examples:\n"
	       "       %s read  cpu0/port/0\n"
	       "       %s write 1/global1/2   0x1337\n"
	       "       %s print eth2-1\n"
	       "\n"
	       "The `read` and `write` commands are simple register level\n"
	       "accessors. The `print` command will pretty-print a register. When\n"
	       "using the `print` command, the register is optional. If left out, the\n"
	       "most common registers will be shown.\n"
	       "\n"
	       "Bug report address: https://github.com/wkz/phytool/issues\n"
	       "\n",
	       __progname, __progname, __progname, __progname, __progname, __progname, __progname);

	return code;
}

static struct applet applets[] = {
	{
		.name = "phytool",
		.usage = phytool_usage,
		.parse_loc = phytool_parse_loc,
		.print = print_phytool
	},
	{
		.name = "mv6tool",
		.usage = mv6tool_usage,
		.parse_loc = mv6tool_parse_loc,
		.print = print_mv6tool
	},

	{ .name = NULL }
};

int main(int argc, char **argv)
{
	struct applet *a;

	for (a = applets; a->name; a++) {
		if (!strcmp(__progname, a->name))
			break;
	}

	if (!a->name)
		a = applets;

	if (argc < 2)
		return a->usage(1);

	if (!strcmp(argv[1], "read"))
		return phytool_read(a, argc - 2, &argv[2]);
	else if (!strcmp(argv[1], "write"))
		return phytool_write(a, argc - 2, &argv[2]);
	else if (!strcmp(argv[1], "print"))
		return phytool_print(a, argc - 2, &argv[2]);
	else
		return phytool_print(a, argc - 1, &argv[1]);

	fprintf(stderr, "error: unknown command \"%s\"\n", argv[1]);
	return a->usage(1);
}
