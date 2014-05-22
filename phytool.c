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

	dev = strtoul(text, &end, 0);
	if (!end[0]) {
		/* simple phy address */
		*phy_id = dev;
		return 0;
	}

	if (end[0] != ':') {
		/* not clause 45 either */
		return 1;
	}

	port = strtoul(end + 1, &end, 0);
	if (end[0])
		return 1;

	*phy_id = mdio_phy_id_c45(port, dev);
	return 0;
}

static int parse_loc(char *text, struct loc *loc, int strict)
{
	char *tok = strtok(text, "/");
	unsigned long reg_in;

	if (!tok)
		goto err_fmt;

	strncpy(loc->ifnam, tok, IFNAMSIZ);

	tok = strtok(NULL, "/");
	if (!tok)
		goto err_fmt;

	if (parse_phy_id(tok, &loc->phy_id))
		goto err_fmt;

	tok = strtok(NULL, "/");
	if (tok)
		reg_in = strtoul(tok, NULL, 0);
	else if (!strict)
		reg_in = REG_SUMMARY;
	else
		goto err_fmt;

	loc->reg = reg_in;
	return 0;
err_fmt:
	fprintf(stderr, "error: bad location format\n");
	return 1;
}

static int phytool_read(int argc, char **argv)
{
	struct loc loc;
	int val;

	if (!argc)
		return 1;

	if (parse_loc(argv[0], &loc, 1))
		return 1;

	val = phy_read (&loc);
	if (val < 0)
		return 1;

	printf("%#.4x\n", val);
	return 0;
}

static int phytool_write(int argc, char **argv)
{
	struct loc loc;
	unsigned long val;
	int err;

	if (argc < 2)
		return 1;

	if (parse_loc(argv[0], &loc, 1))
		return 1;

	val = strtoul(argv[1], NULL, 0);

	err = phy_write (&loc, val);
	if (err)
		return 1;

	return 0;
}

static int phytool_print(int (*print)(const struct loc *loc, int indent),
			 int argc, char **argv)
{
	struct loc loc;
	int err;

	if (!argc)
		return 1;

	if (parse_loc(argv[0], &loc, 0))
		return 1;

	err = print(&loc, 0);
	if (err)
		return 1;
	
	return 0;
}

struct printer {
	const char *name;
	int (*print)(const struct loc *loc, int indent);
};

static struct printer printer[] = {
	{ .name = "phytool", .print = print_phytool },
	{ .name = "mv6tool", .print = print_mv6tool },

	{ .name = NULL }
};

int main(int argc, char **argv)
{
	char *bin = basename(argv[0]);
	struct printer *p;

	if (argc < 2)
		return 1;

	for (p = printer; p->name; p++)
		if (!strcmp(bin, p->name))
			break;

	if (!p->name)
		p = printer;

	if (!strcmp(argv[1], "read"))
		return phytool_read(argc - 2, &argv[2]);
	else if (!strcmp(argv[1], "write"))
		return phytool_write(argc - 2, &argv[2]);
	else if (!strcmp(argv[1], "print"))
		return phytool_print(p->print, argc - 2, &argv[2]);

	fprintf(stderr, "error: unknown command \"%s\"\n", argv[1]);
	return 1;
}
