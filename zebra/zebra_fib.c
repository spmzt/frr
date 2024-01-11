#include "zebra.h"

#include "lib/vrf.h"
#include "lib/fib.h"
#include "lib/prefix.h"
#include "lib/memory.h"

#include "zebra_fib.h"
#include "zebra_vrf.h"
#include "rt.h"
#include "zebra_vxlan.h"
#include "debug.h"
#include "zebra_pbr.h"
#include "zebra_tc.h"
#include "rib.h"
#include "table_manager.h"
#include "zebra_errors.h"
#include "zebra_dplane.h"

extern struct zebra_privs_t zserv_privs;

DEFINE_MTYPE_STATIC(ZEBRA, ZEBRA_FIB, "Zebra FIB");

static struct zebra_fib *dzfib;

struct zebra_fib *zebra_fib_lookup(fib_id_t fib_id)
{
	if (fib_id == FIB_DEFAULT)
		return dzfib;
	struct zebra_fib *info = (struct zebra_fib *)fib_info_lookup(fib_id);

	return (info == NULL) ? dzfib : info;
}

static int zebra_fib_new(struct fib *fib)
{
	struct zebra_fib *zfib;
	if (!fib)
		return -1;

	if (IS_ZEBRA_DEBUG_EVENT)
		zlog_info("ZFIB %s with id %u (created)", fib->name, fib->fib_id);

	zfib = XCALLOC(MTYPE_ZEBRA_FIB, sizeof(struct zebra_fib));
	fib->info = zfib;
	zfib->fib = fib;
	zfib->fib_id = fib->fib_id;

	/* Do any needed per-fib data structure allocation. */
	zfib->if_table = route_table_init();

	return 0;
}

/* Do global enable actions, read kernel config etc. */
int zebra_fib_enable(fib_id_t fib_id, void **info)
{
	struct zebra_fib *zfib = (struct zebra_fib *)(*info);

	zfib->fib_id = fib_id;

    // TODO
	kernel_init(zfib);
	zebra_dplane_ns_enable(zfib, true);
	interface_list(zfib);

	return 0;
}

int zebra_fib_init(void)
{
	struct fib *default_fib;
	fib_id_t fib_id;
	struct fib *fib;

    fib_id = FIB_DEFAULT;
    fib_init_management(fib_id);

	default_fib = fib_lookup(FIB_DEFAULT);
	if (!default_fib) {
		flog_err(EC_ZEBRA_NS_NO_DEFAULT,
			 "%s: failed to find default fib", __func__);
		exit(EXIT_FAILURE); /* This is non-recoverable */
	}

	// /* Do any needed per-fib data structure allocation. */
	zebra_fib_new(default_fib);
	dzfib = default_fib->info;

	// /* Register zebra VRF callbacks, create and activate default VRF. */
	zebra_vrf_init();

	// /* Default FIB is activated */
	zebra_fib_enable(fib_id, (void **)&dzns);

    // TODO
	// if (vrf_is_backend_netns()) {
	// 	ns_add_hook(NS_NEW_HOOK, zebra_ns_new);
	// 	ns_add_hook(NS_ENABLE_HOOK, zebra_ns_enabled);
	// 	ns_add_hook(NS_DISABLE_HOOK, zebra_ns_disabled);
	// 	ns_add_hook(NS_DELETE_HOOK, zebra_ns_delete);
	// 	zebra_ns_notify_parse();
	// 	zebra_ns_notify_init();
	// }

	// return 0;
}