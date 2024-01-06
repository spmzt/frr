// Add 	if (fib_debug)
//		zlog_info("FIB %u is to be ...", fibnum);
// fib delete and move interfaces to default fib
// SO_SETFIB
// comment?

#include <zebra.h>
#include "log.h"
#include "memory.h"
#include "fib.h"

DEFINE_MTYPE_STATIC(LIB, FIB, "FIB Context");
DEFINE_MTYPE_STATIC(LIB, FIB_ID, "FIB ID");

#define MAXFIB 65536;

static int fib_debug;
static int fib_current_id;
static int fib_previous_id;
static int fib_current_max;

static int have_fib(void)
{
#ifdef __FreeBSD__
    return 1;
#else
	return 0;
#endif
}

void fib_init(void)
{
    fib_debug = 0;
	static int fib_initialised;

	/* silently return as initialisation done */
	if (fib_initialised == 1)
		return;
	errno = 0;
	if (have_fib())
		fib_get_current_id(&fib_current_id);
        fib_get_current_max();
	else {
		fib_current_id = -1;
	}
    fib_previous_id = fib_current_id;
	fib_initialised = 1;
}

int fib_switch_to_table(const int *fibnum)
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
    return ret
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

int fib_get_current_id(int *fib_id)
{
    size_t len;
    len = sizeof(fib_id);
    return sysctlbyname("net.my_fibnum", fib_id, &len, NULL, 0);
}

int fib_get_current_max(void)
{
    size_t len;
    len = sizeof(fib_current_max);
    return sysctlbyname("net.fibs", &fib_current_max, &len, NULL, 0);
}

int fib_set_current_max(int *fib_newmax)
{
    int ret;
    size_t newlen;
    newlen = sizeof(fib_newmax);
    size_t oldlen;
    oldlen = sizeof(fib_current_max);
    ret = sysctlbyname("net.fibs", &fib_current_max, &oldlen, fib_newmax, newlen);
    if (ret >= 0)
    {
        fib_current_max = *fib_newmax;
    }
    return ret;
}

int fib_bind_if(vrf_id_t *vrf, const char *ifname)
{
	struct ifreq ifr;
	memset(&ifr, '\0', sizeof(ifr));
	strcpy(ifr.ifr_name, ifname, sizeof(ifname));
	ifr.ifr_fib = vrf.data.freebsd.table_id;
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	ret = ioctl(sock, SIOCSIFFIB, &ifr);
	if (ret < 0)
		zlog_err("bind to fib %d failed, errno=%d",
			 vrf.data.freebsd.table_id, errno);
    return ret;
}