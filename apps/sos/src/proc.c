#include <sel4/sel4.h>
#include <stdlib.h>
#include "proc.h"

/*
 * Create a new address space region and add it to a proc region list
 * @param start the virtual addr where the region starts
 * @param end the virtual addr where the region ends
 * @param proc the process control block to add the region to
 * @returns ptr to the new virtual region
 */
vaddr_region *
proc_create_region(seL4_Word start, seL4_Word size, proc_ctl_blk * proc) {
    
    /* sanity check */
    if (proc == NULL)
        return NULL;
    
    /* check for stupid things */
    //if (size < 0)
    //    return 1;
    
    /* create new region */
    vaddr_region *new_region = (vaddr_region *)malloc(sizeof(vaddr_region));
    
    /* check if alloc failed */
    if (new_region == NULL)
        return NULL;

    /* alter details of new region */
    new_region->vaddr_start = start;
    new_region->vaddr_end = start + size;
    
    return new_region;
}


/*
 * Add a region to a process control block region list
 * @param proc a pointer to a process control block
 * @param region a pointer to a new region
 * @returns 1 on failure and 0 on success
 */
int
proc_add_region(vaddr_region *region, proc_ctl_blk *proc) {
    
    // TODO: make this insert in order of vaddr_start so they are in order
    
    /* early termination */
    if (proc->region_list == NULL) {
        proc->region_list = region;
        goto end_add;
    }

    vaddr_region *cur_reg = proc->region_list;
    vaddr_region *prev_reg = proc->region_list;
    while (cur_reg != NULL) {
        prev_reg = cur_reg;       
        cur_reg = cur_reg->next_region;
    }
    prev_reg->next_region = region;
    
end_add:
    region->next_region = NULL;

    return 0;
}

