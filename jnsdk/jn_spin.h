#ifndef __JN_SPIN_H__
#define __JN_SPIN_H__

typedef u32 spin_t;

#define jn_spin_init(spin) jn_spin_unlock(spin)

ERROR jn_spin_lock(spin_t *spin);
ERROR jn_spin_unlock(spin_t *spin);

#endif // __JN_SPIN_H__



