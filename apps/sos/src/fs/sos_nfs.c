 
/*
 * Wrappers for the NFS file system
 * 
 * Glenn McGuire & Cameron Lonsdale
 */

#include "sos_nfs.h"

#include <network.h>

#include "picoro.h"
#include <syscall/syscall.h>

#include <lwip/ip_addr.h>
#include <nfs/nfs.h>

#include <utils/util.h>

static const vnode_ops nfs_dir_ops = {
    .vop_lookup = sos_nfs_lookup,
};

static const vnode_ops nfs_vnode_ops = {
    .vop_close = sos_nfs_close,
    .vop_read = sos_nfs_read,
    .vop_write = sos_nfs_write,
};

static nfs_lookup_cb_t sos_nfs_lookup_callback(uintptr_t token, enum nfs_stat status, fhandle_t* fh, fattr_t* fattr);
static nfs_write_cb_t sos_nfs_write_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr, int count);
static nfs_read_cb_t sos_nfs_read_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr, int count, void* data);

int
sos_nfs_init(void)
{
    vnode *nfs_mount = malloc(sizeof(vnode));
    if (!nfs_mount) {
        LOG_ERROR("malloc failed when creating nfs namespace");
        return 1; 
    }

    nfs_mount->vn_ops = &nfs_dir_ops;
    vfs_mount(nfs_mount);

    return 0;
}

int
sos_nfs_lookup(char *pathname, vnode **result)
{
    vnode *file;
    nfs_lookup(&mnt_point, pathname, sos_nfs_lookup_callback, NULL);

    file = yield(NULL);
    if (!file) {      
        LOG_INFO("creating file %s", pathname);
        const sattr_t file_attr = { .mode = 0664 }; /* Read write for owner and group, read for everyone */
        nfs_create(&mnt_point, pathname, &file_attr, (nfs_create_cb_t)sos_nfs_lookup_callback, NULL);
        file = yield(NULL);
    }

    *result = file;
    return 0;
}

int
sos_nfs_write(vnode *node, uiovec *iov)
{
    if (nfs_write(node->vn_data, iov->uiov_pos, iov->uiov_len, iov->uiov_base, sos_nfs_write_callback, NULL) != RPC_OK)
        return -1;

    int *ret = yield(NULL);
    return *ret;
}

int
sos_nfs_read(vnode *node, uiovec *iov)
{
    if (nfs_read(node->vn_data, iov->uiov_pos, iov->uiov_len, sos_nfs_read_callback, (uintptr_t)iov) != RPC_OK)
        return -1;

    int *ret = yield(NULL);
    return *ret;
}

int
sos_nfs_close(vnode *node)
{
    /* Free the handle */
    free(node->vn_data);
    free(node);
    return 0;
}

/*
 * Callback for the lookup function
 * NFS tells us if the file exists
 * We then build a vnode to represent that file
 */
nfs_lookup_cb_t
sos_nfs_lookup_callback(uintptr_t token, enum nfs_stat status,
                                fhandle_t* fh, fattr_t* fattr)
{
    vnode *file = NULL;
    if (status != NFS_OK)
        goto coro_resume;

    if ((file = malloc(sizeof(vnode))) == NULL) {
        LOG_ERROR("malloc failed when creating file vnode");
        goto coro_resume;
    }

    file->vn_data = malloc(sizeof(fhandle_t));
    memcpy(file->vn_data, fh, sizeof(fhandle_t));

    file->vn_ops = &nfs_vnode_ops;

    coro_resume:
        resume(syscall_coro, file);
}

/*
 * Write callback
 * Return number of bytes written
 */
nfs_write_cb_t
sos_nfs_write_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr, int count)
{
    int ret_val = -1;
    if (status != NFS_OK)
        goto coro_resume;

    ret_val = count;
    coro_resume:
        resume(syscall_coro, &ret_val);
}

/*
 * Read callback
 * Copy the memory into the buffer specified by the iov_base
 */
nfs_read_cb_t
sos_nfs_read_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr, int count, void* data)
{
    int ret_val = -1;
    if (status != NFS_OK) {
        LOG_INFO("error status is %d", status);
        goto coro_resume;
    }

    struct iovec *iov = (struct iovec *)token;
    memcpy(iov->iov_base, data, count);
    ret_val = count;

    coro_resume:
        LOG_INFO("ret val is %d", ret_val);
        resume(syscall_coro, &ret_val);
}