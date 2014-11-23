#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

/* This is a VERY basic spinlock implementation
 * it simply does a cmpxchg on the spinlock and
 * if the value was already one, continues to
 * try over and over until spinlock becomes
 * zero.
 * 
 * __sync_val_compare_and_swap is a gcc builtin
 * designed to be an atomic compare and exchange
 * function which also returns the old value.
 * 
 * http://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Atomic-Builtins.html
 */

// Use processor dependant size for spinlock to increase speed and
// easier atomicity.
typedef int spinlock_t;

// This is just a wrapper. spin_init should be called before atomicity is needed
#define spin_init(spinlock) do{ spin_unlock((spinlock)); } while(0)
#define init_spin(name) (0)

// lock and unlock a spinlock
void spin_lock(spinlock_t* spinlock);
void spin_unlock(spinlock_t* spinlock);

#endif