#ifndef _LIB_FIB_H
#define _LIB_FIB_H

#include "openbsd-tree.h"
#include "linklist.h"
#include "vty.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t fib_id_t;

/* the default FIB ID */
#define FIB_DEFAULT 0

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

/*
 * FIB hooks
 */

#define FIB_NEW_HOOK        0   /* a new fib is just created */
#define FIB_DELETE_HOOK     1   /* a fib is to be deleted */
#define FIB_ENABLE_HOOK     2   /* a fib is ready to use */
#define FIB_DISABLE_HOOK    3   /* a fib is to be unusable */

/*
 * Add a specific hook fib module.
 * @param1: hook type
 * @param2: the callback function
 *          - param 1: the FIB ID
 *          - param 2: the address of the user data pointer (the user data
 *                     can be stored in or freed from there)
 */
extern void fib_add_hook(int type, int (*)(struct fib *));


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

#ifdef __cplusplus
}
#endif

#endif /*_LIB_FIB_H*/