/*
 * Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <sel4/sel4.h>

/*
 * Process a System Call
 * @param badge, the badge of the capability
 */
void handle_syscall(seL4_Word badge);

#endif /* _SYSCALL_H_ */
