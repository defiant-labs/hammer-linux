#include "dfly_wrap.h"
#include <linux/errno.h>

// from sys/sysctl.h
int desiredvnodes = KERN_MAXVNODES; // Maximum number of vnodes

// from kern/vfs_nlookup.c
int nlookup_init(struct nlookupdata *nd, const char *path, enum uio_seg seg, int flags) {
    panic("nlookup_init");
    return ENOENT;
}

int nlookup(struct nlookupdata *nd) {
    panic("nlookup");
    return ENOENT;
}

void nlookup_done(struct nlookupdata *nd) {
    panic("nlookup_done");
}

// from kern/vfs_subr.c
int count_udev (int x, int y) {
    panic("count_udev");
    return 0;
}

int vfs_mountedon(struct vnode *vp) {
    panic("vfs_mountedon");
    return 0;
}

int vinvalbuf(struct vnode *vp, int flags, int slpflag, int slptimeo) {
    panic("vinvalbuf");
    return 0;
}

int vn_isdisk(struct vnode *vp, int *errp) {
    panic("vn_isdisk");
    return 1;
}

int vn_lock(struct vnode *vp, int flags) {
    panic("vn_lock");
    return ENOENT;
}

void vn_unlock(struct vnode *vp) {
    panic("vn_unlock");
}

// from kern/vopops.c
int vop_open(struct vop_ops *ops, struct vnode *vp, int mode, struct ucred *cred,
             struct file *fp) {
    panic("vop_open");
    return 0;
}

int vop_close(struct vop_ops *ops, struct vnode *vp, int fflag) {
    panic("vop_close");
    return 0;
}

int vop_fsync(struct vop_ops *ops, struct vnode *vp, int waitfor) {
    panic("vop_fsync");
    return 0;
}

// from kern/vfs_lock.c
void vrele(struct vnode *vp) {
    panic("vrele");
}

// from kern/vfs_cache.c
int cache_vref(struct nchandle *nch, struct ucred *cred, struct vnode **vpp) {
    panic("cache_vref");
    return ENOENT;
}

// from platform/*/*/db_interface.c
void Debugger (const char *msg) {
    panic("Debugger");
}

// from platform/*/platform/copyio.c
int copyout(const void *kaddr, void *udaddr, size_t len) {
    panic("copyout");
    return 0;
}

int copyin(const void *udaddr, void *kaddr, size_t len) {
    panic("copyin");
    return 0;
}

// from libkern/bcmp.c
int bcmp(const void *b1, const void *b2, size_t length) {
    panic("bcmp");
    return 0;
}

// from kern/vfs_bio.c
int hidirtybufspace;

int bread(struct vnode *vp, off_t loffset, int size, struct buf **bpp) {
    panic("bread");
    return 0;
}

void brelse(struct buf *bp) {
    panic("brelse");
}

int bd_heatup (void) {
    panic("bd_heatup");
    return 0;
}

// from ??
void bzero (volatile void *buf, size_t len) {
    panic("bzero");
}

void bcopy (volatile const void *from, volatile void *to, size_t len) {
    panic("bcopy");
}

// from kern/vfs_mount.c
int vmntvnodescan(
    struct mount *mp, 
    int flags,
    int (*fastfunc)(struct mount *mp, struct vnode *vp, void *data),
    int (*slowfunc)(struct mount *mp, struct vnode *vp, void *data),
    void *data
) {
    panic("vmntvnodescan");
    return 0;
}

// from kern/kern_slaballoc.c
#undef kfree
void dfly_kfree(void *ptr, struct malloc_type *type) {
    panic("dfly_kfree");
}

#undef kmalloc
void *dfly_kmalloc(unsigned long size, struct malloc_type *type, int flags) {
    return kmalloc(size, GFP_KERNEL);
}

MALLOC_DEFINE(M_TEMP, "temp", "misc temporary data buffers");

// from kern/kern_synch.c
int tsleep(void *ident, int flags, const char *wmesg, int timo) {
    panic("tsleep");
    return EWOULDBLOCK;
}

void wakeup(void *ident) {
    panic("wakeup");
}

// from kern/clock.c
time_t time_second;             /* read-only 'passive' uptime in seconds */

void getmicrotime(struct timeval *tvp) {
    panic("getmicrotime");
}

// from sys/signal2.h
int __cursig(struct lwp *lp, int mayblock, int maytrace) {
    panic("__cursig");
    return 0;
}

// from kern/lwkt_thread.c
int lwkt_create(void (*func)(void *), void *arg,
    struct thread **tdp, thread_t template, int tdflags, int cpu,
    const char *fmt, ...)
{
    panic("lwkt_create");
    return 0;
}

void lwkt_exit(void) {
    panic("lwkt_exit");
}

// from kern/subr_param.c
int hz;

// from kern/kern_iosched.c
void bwillwrite(int bytes) {
    panic("bwillwrite");
}

// from kern/kern_prot.c
int priv_check_cred(struct ucred *cred, int priv, int flags) {
    panic("priv_check_cred");
    return 0;
}

// from kern/subr_prf.c
int kvprintf(const char *fmt, __va_list ap) {
    panic("kvprintf");
    return 0;
}
