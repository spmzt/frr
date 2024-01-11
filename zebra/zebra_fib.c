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

	kernel_fib_init(zfib);
	// TODO: zebra_dplane_fib_enable(zfib, true);
	interface_list_fib(zfib);

	return 0;
}

static int zebra_fib_delete(struct fib *fib)
{
	struct zebra_fib *zfib = (struct zebra_fib *)fib->info;

	if (IS_ZEBRA_DEBUG_EVENT)
		zlog_info("ZFIB with id %u (deleted)", fib->fib_id);
	if (!zfib)
		return 0;
	XFREE(MTYPE_ZEBRA_FIB, fib->info);
	return 0;
}

static int zebra_fib_enabled(struct fib *fib)
{
	struct zebra_fib *zfib = fib->info;

	if (IS_ZEBRA_DEBUG_EVENT)
		zlog_info("ZFIB id %u (enabled)", zfib->fib_id);
	if (!zfib)
		return 0;
	return zebra_fib_enable(fib->fib_id, (void **)&zfib);
}

int zebra_fib_disabled(struct fib *fib)
{
	struct zebra_fib *zfib = fib->info;

	if (IS_ZEBRA_DEBUG_EVENT)
		zlog_info("ZFIB id %u (disabled)", zfib->fib_id);
	if (!zfib)
		return 0;
	return zebra_fib_disable_internal(zfib, true);
}

/* Common handler for fib disable - this can be called during fib config,
 * or during zebra shutdown.
 */
static int zebra_fib_disable_internal(struct zebra_fib *zfib, bool complete)
{
	if (zfib->if_table)
		route_table_finish(zfib->if_table);
	zfib->if_table = NULL;

	// TODO: zebra_dplane_fib_enable(zfib, false /*Disable*/);

	kernel_fib_terminate(zfib, complete);

	zfib->fib_id = FIB_DEFAULT;

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

	if (vrf_is_backend_fib()) {
		ns_add_hook(FIB_NEW_HOOK, zebra_fib_new);
		ns_add_hook(FIB_ENABLE_HOOK, zebra_fib_enabled);
		ns_add_hook(FIB_DISABLE_HOOK, zebra_fib_disabled);
		ns_add_hook(FIB_DELETE_HOOK, zebra_fib_delete);
		// TODO: should I implement fib_notify?
	}

	return 0;
}

int zebra_fib_config_write(struct vty *vty, struct fib *fib)
{
	if (fib && fib->fib_id != NULL)
		vty_out(vty, " fib %d\n", fib->fib_id);
	return 0;
}