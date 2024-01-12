// Add 	if (fib_debug)
//		zlog_info("FIB %u is to be ...", fibnum);
// fib delete and move interfaces to default fib
// SO_SETFIB
// comment?

#include "log.h"
#include "memory.h"
#include "fib.h"
#include "lib_errors.h"

DEFINE_MTYPE_STATIC(LIB, FIB, "FIB Context");
DEFINE_MTYPE_STATIC(LIB, FIB_ID, "FIB ID");

#define MAXFIB 65536;

static inline int fib_compare(const struct fib *fib, const struct fib *fib2);

RB_GENERATE(fib_head, fib, entry, fib_compare)

static struct fib_head fib_tree = RB_INITIALIZER(&fib_tree);

static struct fib *default_fib;
static int fib_current_id;
static int fib_previous_id;
static int fib_current_max;

static int fib_debug;

/* Holding FIB hooks  */
static struct fib_master {
	int (*fib_new_hook)(struct fib *fib);
	int (*fib_delete_hook)(struct fib *fib);
	int (*fib_enable_hook)(struct fib *fib);
	int (*fib_disable_hook)(struct fib *fib);
} fib_master = {
	0,
};

static inline int fib_compare(const struct fib *a, const struct fib *b)
{
	return (a->fib_id - b->fib_id);
}

int fib_get_current_id(int *fib_id)
{
    size_t len;
    len = sizeof(fib_id);
    return sysctlbyname("net.my_fibnum", fib_id, &len, NULL, 0);
}

/* Look up a FIB by identifier. */
static struct fib *fib_lookup_internal(fib_id_t fib_id)
{
	struct fib fib;

	fib.fib_id = fib_id;
	return RB_FIND(fib_head, &fib_tree, &fib);
}

struct fib *fib_lookup(fib_id_t fib_id)
{
	return fib_lookup_internal(fib_id);
}

int fib_get_current_max(void)
{
    size_t len;
    len = sizeof(fib_current_max);
    return sysctlbyname("net.fibs", &fib_current_max, &len, NULL, 0);
}

int have_fib(void)
{
#if defined(__FreeBSD__) && defined(SO_SETFIB)
    return 1;
#else
	return 0;
#endif
}

static int fib_is_enabled(struct fib *fib)
{
	if (have_fib()) {
		fib_get_current_max();
		return fib && fib->fib_id <= fib_current_max;
	} else
		return fib && fib->fib_id == 0;
}

/*
 * Disable a FIB - that is, let the FIB be unusable.
 * The FIB_DELETE_HOOK callback will be called to inform
 * that they must release the resources in the FIB.
 */
static void fib_disable_internal(struct fib *fib)
{
	if (fib_is_enabled(fib)) {
		if (fib_debug)
			zlog_info("FIB %u is to be disabled.", fib->fib_id);

		if (fib_master.fib_disable_hook)
			(*fib_master.fib_disable_hook)(fib);

		// TODO: Should I clear the FIB table?
	}
}

void fib_disable(struct fib *fib)
{
	fib_disable_internal(fib);
}

/* Delete a FIB. This is called in ns_terminate(). */
void fib_delete(struct fib *fib)
{
	if (fib_debug)
		zlog_info("FIB %u is to be deleted.", fib->fib_id);

	fib_disable(fib);

	if (fib_master.fib_delete_hook)
		(*fib_master.fib_delete_hook)(fib);

	/*
	 * I'm not entirely sure if the vrf->iflist
	 * needs to be moved into here or not.
	 */
	// if_terminate (&fib->iflist);

	RB_REMOVE(fib_head, &fib_tree, fib);
	XFREE(MTYPE_FIB_ID, fib->fib_id);

	XFREE(MTYPE_FIB, fib);
}

int fib_set_current_max(fib_id_t fib_newmax)
{
	if (fib_newmax <= fib_current_max)
		return 0;
    int ret;
    size_t newlen;
    newlen = sizeof(fib_newmax);
    size_t oldlen;
    oldlen = sizeof(fib_current_max);
    ret = sysctlbyname("net.fibs", &fib_current_max, &oldlen, &fib_newmax, newlen);
    if (ret >= 0)
    {
        fib_current_max = fib_newmax;
    }
    return ret;
}

static int fib_enable_internal(struct fib *fib, void (*func)(fib_id_t, void *))
{
	if (!fib_is_enabled(fib)) {
		if (have_fib()) {
			fib_set_current_max(fib->fib_id);
		} else {
			errno = -ENOTSUP;
		}

		if (!fib_is_enabled(fib)) {
			flog_err_sys(EC_LIB_SYSTEM_CALL,
				     "Can not enable FIB %u: %s!", fib->fib_id,
				     safe_strerror(errno));
			return 0;
		}

		if (func)
			func(fib->fib_id, (void *)fib->vrf_ctxt);
		if (fib_debug) {
			zlog_info("FIB %u is enabled.", fib->fib_id);
		}
		/* zebra first receives FIB enable event,
		 * then VRF enable event
		 */
		if (fib_master.fib_enable_hook)
			(*fib_master.fib_enable_hook)(fib);
	}

	return 1;
}

int fib_enable(struct fib *fib, void (*func)(fib_id_t, void *))
{
	return fib_enable_internal(fib, func);
}

static struct fib *fib_get_created_internal(struct fib *fib, fib_id_t fib_id)
{
	int created = 0;
	/*
	 * Initialize interfaces.
	 */
	if (!fib && fib_id == FIB_DEFAULT)
		fib = fib_lookup(fib_id);
	if (!fib) {
		fib = XCALLOC(MTYPE_FIB, sizeof(struct fib));
		fib->fib_id = fib_id;
		RB_INSERT(fib_head, &fib_tree, fib);
		created = 1;
	}
	if (fib_id != fib->fib_id) {
		RB_REMOVE(fib_head, &fib_tree, fib);
		fib->fib_id = fib_id;
		RB_INSERT(fib_head, &fib_tree, fib);
	}
	if (!created)
		return fib;
	if (fib_debug) {
		zlog_info("FIB %u is created.", fib->fib_id);
	}
	if (fib_master.fib_new_hook)
		(*fib_master.fib_new_hook)(fib);
	return fib;
}

struct fib *fib_get_created(struct fib *fib, fib_id_t fib_id)
{
	return fib_get_created_internal(fib, fib_id);
}

/* Look up the data pointer of the specified Zebra VRF. */
void *fib_info_lookup(fib_id_t fib_id)
{
	struct fib *fib = fib_lookup_internal(fib_id);

	return fib ? fib->info : NULL;
}

void fib_init(void)
{
    fib_debug = 0;
	static int fib_initialised;

	/* silently return as initialisation done */
	if (fib_initialised == 1)
		return;
	errno = 0;
	if (have_fib()) {
		fib_get_current_id(&fib_current_id);
        fib_get_current_max();
	} else {
		fib_current_id = -1;
		fib_current_max = 0;
	}
    fib_previous_id = fib_current_id;
	fib_initialised = 1;
}

int fib_switch_to_table(uint16_t fibnum)
{
    int ret;
    int ret2;

    if (fib_current_id == -1)
		return -1;

    if (fibnum > fib_current_max) {
        ret2 = fib_set_current_max(fibnum);
        if (ret2 < 0)
        {
            return ret2;
        }
    }
    fib_get_current_id(&fib_previous_id);
    ret = setfib(fibnum);
    fib_current_id = fibnum;
    return ret;
}

int fib_switchback_to_initial(void)
{
    if (fib_current_id != -1 && fib_previous_id != -1)
    {
        return setfib(fib_previous_id);
    }
	/* silently ignore if fib is not initialised */
    return 1;
}

/* Create a socket for the FIB. */
int fib_socket(int domain, int type, int protocol, fib_id_t fib_id)
{
	struct fib *fib = fib_lookup(fib_id);
	int ret;

	if (!fib || !fib_is_enabled(fib)) {
		errno = EINVAL;
		return -1;
	}

	ret = socket(domain, type, protocol);
	if (have_fib()) {
		if (fib_id != FIB_DEFAULT) {
			if(setsockopt(ret, SOL_SOCKET, SO_SETFIB, fib_id,
							sizeof(fib_id)) == -1) {
				flog_err_sys(EC_LIB_SOCKET,
						"Can't set SO_SETFIB on socket %u: %s!", fib->fib_id,
						safe_strerror(errno));
				return -1;
			}
		}
	}

	return ret;
}

int fib_bind_if(fib_id_t fib_id, const char *ifname)
{
	int ret;
	struct ifreq ifr = {};
	strlcpy(ifr.ifr_name, ifname, sizeof(ifname));
	ifr.ifr_fib = fib_id;
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	ret = ioctl(sock, SIOCSIFFIB, &ifr);
	if (ret < 0)
		zlog_err("bind to fib %u failed, errno=%d", fib_id, errno);
    return ret;
}

void fib_init_management(fib_id_t fib_id)
{
	fib_init();
	default_fib = fib_get_created(NULL, fib_id);
	if (!default_fib) {
		flog_err(EC_LIB_FIB, "%s: failed to create the default FIB!",
			 __func__);
		exit(1);
	}

	/* Enable the default FIB. */
	if (!fib_enable(default_fib, NULL)) {
		flog_err(EC_LIB_FIB, "%s: failed to enable the default FIB!",
			 __func__);
		exit(1);
	}
}