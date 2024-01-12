#if !defined(__ZEBRA_FIB_H__)
#define __ZEBRA_FIB_H__

#include <lib/fib.h>
#include <lib/vrf.h>

#include "zebra/rib.h"
#include "zebra/zebra_vrf.h"

struct zebra_fib {
	/* Identifier. */
	fib_id_t fib_id;

	struct route_table *if_table;

	/* Back pointer */
	struct fib *fib;
};

int zebra_fib_init(void);

int zebra_fib_enable(fib_id_t fib_id, void **info);

int zebra_fib_disabled(struct fib *fib);

int zebra_fib_config_write(struct vty *vty, struct fib *fib);

struct zebra_fib *zebra_fib_lookup(fib_id_t fib_id);

#endif