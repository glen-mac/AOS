/*
 * Event loop
 *
 * Cameron Lonsdale & Glenn McGuire
 */

#include "event.h"

#include <vm/vm.h>
#include <clock/clock.h>
#include <cspace/cspace.h>
#include <sys/debug.h>
#include <sys/panic.h>
#include <syscall/syscall.h>
#include <proc/proc.h>
#include <utils/util.h>
#include "network.h"

static void start_first_proc(void);
static void reap_dead_orphans(proc *init);

void
event_loop(const seL4_CPtr ep)
{
    seL4_Word badge;
    seL4_Word label;
    seL4_MessageInfo_t message;

    /* Start the startup user application */
    resume(coroutine(start_first_proc), NULL);

    LOG_INFO("returned from start_first_proc");

    proc *init = get_proc(0);
    assert(init != NULL);

    while (TRUE) {
        reap_dead_orphans(init);

        message = seL4_Wait(ep, &badge);
        label = seL4_MessageInfo_get_label(message);
        if (badge & IRQ_EP_BADGE) {
            /* Interrupt */
            if (badge & IRQ_BADGE_NETWORK)
                network_irq();

            /* Needs to be an if, interrupts can group together */
            if (badge & IRQ_BADGE_TIMER)
                timer_interrupt();

        } else if (label == seL4_VMFault) {
            resume(coroutine(vm_fault), GET_PROCID_BADGE(badge));
        } else if (label == seL4_NoFault) {
            resume(coroutine(handle_syscall), GET_PROCID_BADGE(badge));
        } else {
            LOG_INFO("Rootserver got an unknown message");
        }
    }
}

/*
 * Destroy a zombie child of init process
 * @param init, the init process
 */
static void 
reap_dead_orphans(proc *init)
{
    for (struct list_node *curr = init->children->head; curr != NULL; curr = curr->next) {
        proc *child = get_proc(curr->data);
        assert(child != NULL);
        if (child->p_state == ZOMBIE) {
            proc_destroy(child);
            /* 
             * We break because we dont want to keep iterating
             * as proc_destroy modifies the list, so if we keep iterating
             * we get invalid data.
             * Breaking is fine, we will cleanup other children later
             */
            break;
        }
    }
}

/*
 * Start the first process 
 * Needs to be here because of NFS read
 */
static void
start_first_proc(void)
{
    assert(proc_start(CONFIG_SOS_STARTUP_APP, _sos_ipc_ep_cap, 0) != -1);
}
