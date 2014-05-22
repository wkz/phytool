#ifndef __PHYTOOL_H
#define __PHYTOOL_H

#define INDENT 3

#define REG_SUMMARY 0xffff

struct loc {
	char ifnam[IFNAMSIZ];
	uint16_t phy_id;
	uint16_t reg;
};

static inline int loc_is_c45(const struct loc *loc)
{
	return loc->phy_id & MDIO_PHY_ID_C45;
}

static inline int loc_c45_dev(const struct loc *loc)
{
	return loc->phy_id & MDIO_PHY_ID_DEVAD;
}

static inline int loc_c45_port(const struct loc *loc)
{
	return (loc->phy_id & MDIO_PHY_ID_PRTAD) >> 5;
}

int      phy_read (const struct loc *loc);
int      phy_write(const struct loc *loc, uint16_t val);
uint32_t phy_id   (const struct loc *loc);

void print_bit_array(uint16_t val, int indent);
void print_bool(const char *name, int on);

int print_phytool(const struct loc *loc, int indent);
int print_mv6tool(const struct loc *loc, int indent);

#endif	/* __PHYTOOL_H */
