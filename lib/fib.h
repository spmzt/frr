#ifndef _LIB_FIB_H
#define _LIB_FIB_H

#include "openbsd-tree.h"
#include "linklist.h"
#include "vty.h"

/* the default FIB ID */
#define FIB_DEFAULT 0

typedef uint16_t fib_id_t;

struct fib {
	RB_ENTRY(fib) entry;

	/* Identifier, same as the vector index */
	fib_id_t fib_id;

	/* Master list of interfaces belonging to this FIB */
	struct list *iflist;

	/* Back Pointer to VRF */
	void *vrf_ctxt;

	/* User data */
	void *info;
};

RB_HEAD(fib_head, fib);
RB_PROTOTYPE(fib_head, fib, entry, fib_compare);

extern void fib_init(void);

extern int have_fib(void);

extern void *fib_info_lookup(fib_id_t fib_id);

extern void fib_init_management(fib_id_t fib_id);

extern struct fib *fib_lookup(fib_id_t fib_id);

/* API that can be used to change from NS */
extern int fib_switch_to_table(uint16_t fibnum);
extern int fib_switchback_to_initial(void);

extern int fib_get_current_id(int *fib_id);

extern int fib_get_current_max(void);

extern int fib_set_current_max(fib_id_t fib_newmax);

extern int fib_bind_if(fib_id_t fib_id, const char *ifname);

extern int fib_enable(struct fib *fib, void (*func)(fib_id_t, void *));

extern void fib_delete(struct fib *fib);

extern void fib_disable(struct fib *fib);

extern int fib_socket(int domain, int type, int protocol, fib_id_t fib_id);

extern struct fib *fib_get_created(struct fib *fib, fib_id_t fib_id);

#endif /*_LIB_FIB_H*/