#ifndef _ZEBRA_FIB_H
#define _ZEBRA_FIB_H

/* the default FIB ID */
#define FIB_DEFAULT 0

extern void fib_init(void);

extern int fib_switch_to_table(const int *fibnum);

extern int fib_switchback_to_initial(void);

extern int fib_get_current_id(int *fib_id);

extern int fib_get_current_max(void);

extern int fib_set_current_max(int *fib_newmax);

extern int fib_bind_if(vrf_id_t *vrf, const char *ifname)

#endif /*_ZEBRA_FIB_H*/