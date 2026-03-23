use lotto_sys as raw;

pub type Category = raw::base_category;

pub fn effective_event_type(ctx: &raw::context_t) -> u32 {
    if ctx.type_ != 0 {
        ctx.type_ as u32
    } else {
        ctx.src_type as u32
    }
}

pub fn effective_category(ctx: &raw::context_t) -> Category {
    match effective_event_type(ctx) {
        raw::EVENT_TASK_CREATE => Category::CAT_TASK_CREATE,
        raw::EVENT_CALL => Category::CAT_CALL,
        raw::EVENT_TASK_INIT => Category::CAT_TASK_INIT,
        raw::EVENT_TASK_FINI => Category::CAT_TASK_FINI,
        raw::EVENT_TASK_DETACH => Category::CAT_DETACH,
        raw::EVENT_KEY_CREATE => Category::CAT_KEY_CREATE,
        raw::EVENT_KEY_DELETE => Category::CAT_KEY_DELETE,
        raw::EVENT_SET_SPECIFIC => Category::CAT_SET_SPECIFIC,
        raw::EVENT_BEFORE_READ => Category::CAT_BEFORE_READ,
        raw::EVENT_BEFORE_WRITE => Category::CAT_BEFORE_WRITE,
        raw::EVENT_BEFORE_AREAD => Category::CAT_BEFORE_AREAD,
        raw::EVENT_BEFORE_AWRITE => Category::CAT_BEFORE_AWRITE,
        raw::EVENT_BEFORE_RMW => Category::CAT_BEFORE_RMW,
        raw::EVENT_BEFORE_XCHG => Category::CAT_BEFORE_XCHG,
        raw::EVENT_BEFORE_CMPXCHG => Category::CAT_BEFORE_CMPXCHG,
        raw::EVENT_BEFORE_FENCE => Category::CAT_BEFORE_FENCE,
        raw::EVENT_AFTER_AREAD => Category::CAT_AFTER_AREAD,
        raw::EVENT_AFTER_AWRITE => Category::CAT_AFTER_AWRITE,
        raw::EVENT_AFTER_RMW => Category::CAT_AFTER_RMW,
        raw::EVENT_AFTER_XCHG => Category::CAT_AFTER_XCHG,
        raw::EVENT_AFTER_CMPXCHG_S => Category::CAT_AFTER_CMPXCHG_S,
        raw::EVENT_AFTER_CMPXCHG_F => Category::CAT_AFTER_CMPXCHG_F,
        raw::EVENT_AFTER_FENCE => Category::CAT_AFTER_FENCE,
        raw::EVENT_FUNC_ENTRY => Category::CAT_FUNC_ENTRY,
        raw::EVENT_FUNC_EXIT => Category::CAT_FUNC_EXIT,
        raw::EVENT_REGION_IN => Category::CAT_REGION_IN,
        raw::EVENT_REGION_OUT => Category::CAT_REGION_OUT,
        raw::EVENT_MUTEX_ACQUIRE => Category::CAT_MUTEX_ACQUIRE,
        raw::EVENT_MUTEX_TRYACQUIRE => Category::CAT_MUTEX_TRYACQUIRE,
        raw::EVENT_MUTEX_RELEASE => Category::CAT_MUTEX_RELEASE,
        raw::EVENT_EVEC_PREPARE => Category::CAT_EVEC_PREPARE,
        raw::EVENT_EVEC_WAIT => Category::CAT_EVEC_WAIT,
        raw::EVENT_EVEC_TIMED_WAIT => Category::CAT_EVEC_TIMED_WAIT,
        raw::EVENT_EVEC_CANCEL => Category::CAT_EVEC_CANCEL,
        raw::EVENT_EVEC_WAKE => Category::CAT_EVEC_WAKE,
        raw::EVENT_EVEC_MOVE => Category::CAT_EVEC_MOVE,
        raw::EVENT_RWLOCK_RDLOCK | raw::EVENT_RWLOCK_TIMEDRDLOCK => Category::CAT_RWLOCK_RDLOCK,
        raw::EVENT_RWLOCK_WRLOCK | raw::EVENT_RWLOCK_TIMEDWRLOCK => Category::CAT_RWLOCK_WRLOCK,
        raw::EVENT_RWLOCK_UNLOCK => Category::CAT_RWLOCK_UNLOCK,
        raw::EVENT_RWLOCK_TRYRDLOCK => Category::CAT_RWLOCK_TRYRDLOCK,
        raw::EVENT_RWLOCK_TRYWRLOCK => Category::CAT_RWLOCK_TRYWRLOCK,
        raw::EVENT_RSRC_ACQUIRING => Category::CAT_RSRC_ACQUIRING,
        raw::EVENT_RSRC_RELEASED => Category::CAT_RSRC_RELEASED,
        raw::EVENT_POLL => Category::CAT_POLL,
        raw::EVENT_TASK_VELOCITY => Category::CAT_TASK_VELOCITY,
        raw::EVENT_TASK_JOIN => Category::CAT_JOIN,
        raw::EVENT_REGION_PREEMPTION => Category::CAT_REGION_PREEMPTION,
        raw::EVENT_ORDER | raw::EVENT_FORK_EXECVE | raw::EVENT_CXA_GUARD_CALL => Category::CAT_CALL,
        raw::EVENT_SCHED_YIELD | raw::EVENT_USER_YIELD => Category::CAT_USER_YIELD,
        raw::EVENT_SYS_YIELD | raw::EVENT_TIME_YIELD => Category::CAT_SYS_YIELD,
        _ => ctx.cat,
    }
}
