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

void print_attr_name(const char *name, int indent);
void print_bool(const char *name, int on);

int print_phytool(const struct loc *loc, int indent);
int print_mv6tool(const struct loc *loc, int indent);

#endif	/* __PHYTOOL_H */
