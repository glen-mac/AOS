/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/****************************************************************************
 *
 *      $Id:  $
 *
 *      Description: Simple milestone 0 test.
 *
 *      Author:			Godfrey van der Linden
 *      Original Author:	Ben Leslie
 *
 ****************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sel4/sel4.h>
#include <utils/page.h>

#include "ttyout.h"

#define NPAGES 27
#define TEST_ADDRESS 0x20000000

static void test_m0(void);
static void test_m3(void);

/* 
 * Block a thread forever
 * we do this by making an unimplemented system call.
 */
static void
thread_block(void) {
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetTag(tag);
    seL4_SetMR(0, 2);
    seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);
}

int main(void) {
    /* initialise communication */
    ttyout_init();

    test_m0();
    test_m3();
    return 0;
}


static void
test_m0(void)
{
    size_t max_msg_size = (seL4_MsgMaxLength - 2) * sizeof(seL4_Word);

    /* Send a simple message */
    char *message = "123456\n";
    size_t bytes_sent = sos_write(message, strlen(message));
    assert(bytes_sent == strlen(message));

    /* Send a long message that is split into 2 packets */
    /* One packet of 472 A's, the next with 7 B's and a newline */
    char *message2 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBB\n";
    bytes_sent = sos_write(message2, strlen(message2));
    assert(bytes_sent == strlen(message2));

    /* Checking that SOS validates nbytes sent and clamps the value to max_msg_size */
    /* Hence, all that should be printed out is all A's, nothing past the end of the buffer */
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, seL4_MsgMaxLength);
    seL4_SetTag(tag);
    seL4_SetMR(0, 1); /* Syscall number */
    seL4_SetMR(1, 99999); /* Number of bytes in the message */
    memcpy(seL4_GetIPCBuffer()->msg + 2, message2, max_msg_size);
    seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);
    assert((size_t)seL4_GetMR(0) == max_msg_size);
}


static void
test_m3(void)
{
    /* need a decent sized stack */
    char buf1[NPAGES * PAGE_SIZE_4K];

    /* check the stack is above phys mem */
    assert((void *) buf1 > (void *) TEST_ADDRESS);
    
    /* stack test */
    /* set */
    for (int i = 0; i < NPAGES; i++) {
        buf1[i * PAGE_SIZE_4K] = i;
    }
    /* check */
    for (int i = 0; i < NPAGES; i++) {
        assert(buf1[i * PAGE_SIZE_4K] == i);
    }

    /* This should generate a Read fault type */
    seL4_Word *addr = (seL4_Word *)TEST_ADDRESS;


    // /* heap test */
    // char *buf2 = malloc(NPAGES * PAGE_SIZE_4K);
    // assert(buf2);
    // do_pt_test(buf2);
    // free(buf2);
}

