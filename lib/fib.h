#ifndef _LIB_FIB_H
#define _LIB_FIB_H

#include "openbsd-tree.h"
#include "linklist.h"

/* the default FIB ID */
#define FIB_DEFAULT 0
#define FIB_DEFAULT_NAME    "default-fib"

typedef uint16_t fib_id_t;

struct fib {
	RB_ENTRY(fib) entry;

	/* Identifier, same as the vector index */
	fib_id_t fib_id;

	/* Name */
	char *name;

	/* Master list of interfaces belonging to this FIB */
	struct list *iflist;

	/* Back Pointer to VRF */
	void *vrf_ctxt;

	/* User data */
	void *info;
};

RB_HEAD(fib_head, fib);
RB_PROTOTYPE(fib_head, fib, entry, fib_compare)

extern void fib_init(void);

extern int have_fib(void);

extern void *fib_info_lookup(fib_id_t fib_id);

extern void fib_init_management(fib_id_t fib_id);

extern struct fib *fib_lookup(fib_id_t fib_id);

extern int fib_switch_to_table(const int *fibnum);

extern int fib_switchback_to_initial(void);

extern int fib_get_current_id(int *fib_id);

extern int fib_get_current_max(void);

extern int fib_set_current_max(int *fib_newmax);

extern int fib_bind_if(vrf_id_t *vrf, const char *ifname)

#endif /*_LIB_FIB_H*/