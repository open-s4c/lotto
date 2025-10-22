/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 * SPDX-License-Identifier: 0BSD
 * -----------------------------------------------------------------------------
 * Thread "self-awareness"
 *
 * The Self module implements TLS management with the memory from mempool.h and
 * guards further modules from receiving events before threads are
 * initialized or after they are finished.
 *
 * The Self module subscribes to INTERCEPT_{BEFORE,AFTER,EVENT} chains and
 * interrupts them, ie, any module subscribing after Self module does not
 * receive events in the INTERCEPT chains. The Self module republishes all
 * events in equivalent CAPTURE chains, but these contain TLS metadata object
 * can can be used by the user.
 *
 * When a self object is created, an EVENT_SELF_INIT is published in
 * CAPTURE_EVENT. When a self object is deleted, an EVENT_SELF_FINI is
 * published.
 *
 * Self subscribes THREAD_JOIN events to immediately delete self objects. For
 * other threads (eg, detached threads), Self eventually garbage collect the
 * self object.
 */
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#if defined(__NetBSD__)
    #include <errno.h>
#endif

#ifndef DICE_MODULE_PRIO
    #define DICE_MODULE_PRIO 4
#endif
#include <dice/chains/intercept.h>
#include <dice/events/pthread.h>
#include <dice/log.h>
#include <dice/mempool.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <dice/rbtree.h>
#include <dice/self.h>
#include <dice/thread_id.h>
#include <vsync/atomic.h>
#include <vsync/spinlock/caslock.h>
#include <vsync/stack/quack.h>

/* self object represents each thread in the CAPTURE chains. */
struct self {
    metadata_t md;
    struct quack_node_s retired_node;
    uint64_t oid;
    pthread_t pid;
    thread_id tid;
    struct rbtree tls;
    int guard;
    bool retired;
};

/* tls_item is a memory object allocated in the thread-local storage (tls) */
struct tls_item {
    uintptr_t key;
    struct rbnode node;
    char data[];
};

static void _cleanup_threads(pthread_t id);

// -----------------------------------------------------------------------------
// tls items
// -----------------------------------------------------------------------------

DICE_HIDE int
_tls_cmp(const struct rbnode *a, const struct rbnode *b)
{
    const struct tls_item *ea = container_of(a, struct tls_item, node);
    const struct tls_item *eb = container_of(b, struct tls_item, node);
    return ea->key > eb->key ? 1 : ea->key < eb->key ? -1 : 0;
}

DICE_HIDE void
_tls_init(struct self *self)
{
    rbtree_init(&self->tls, _tls_cmp);
}

DICE_HIDE void
_tls_fini(struct self *self)
{
    // TODO: iterate over all items and mempool_free them
    (void)self;
}

// -----------------------------------------------------------------------------
// public interface
// -----------------------------------------------------------------------------

DICE_HIDE_IF thread_id
self_id(metadata_t *md)
{
    struct self *self = (struct self *)md;
    return self ? self->tid : NO_THREAD;
}

DICE_HIDE_IF bool
self_retired(metadata_t *md)
{
    struct self *self = (struct self *)md;
    return self ? self->retired : false;
}

DICE_HIDE_IF void *
self_tls(metadata_t *md, const void *global, size_t size)
{
    uintptr_t item_key = (uintptr_t)global;
    struct self *self  = (struct self *)md;

    // should never be called before the self initialization
    assert(self != NULL);

    struct tls_item key = {.key = item_key};
    struct rbnode *item = rbtree_find(&self->tls, &key.node);

    if (item) {
        struct tls_item *i = container_of(item, struct tls_item, node);
        return i->data;
    }

    struct tls_item *i = mempool_alloc(size + sizeof(struct tls_item));
    if (i == NULL)
        return NULL;
    memset(&i->data, 0, size);
    i->key = item_key;
    rbtree_insert(&self->tls, &i->node);
    return i->data;
}

// -----------------------------------------------------------------------------
// thread cache using pthread_get/setspecific
// -----------------------------------------------------------------------------

static struct {
    caslock_t lock;
    vatomic64_t count;
    pthread_key_t cache_key;
    quack_t retired;
} _threads;

static void
_thread_cache_destruct(void *arg)
{
    (void)arg;
}

static void
_thread_cache_init(void)
{
    if (pthread_key_create(&_threads.cache_key, _thread_cache_destruct) != 0)
        abort();
}

static void
_thread_cache_del(struct self *self)
{
    (void)self;
    if (pthread_setspecific(_threads.cache_key, NULL) != 0)
        log_fatal("could not del key");
}

static void
_thread_cache_set(struct self *self)
{
    if (pthread_setspecific(_threads.cache_key, self) != 0)
        log_fatal("could not set key");
}

DICE_HIDE struct self *
_thread_cache_get(void)
{
    return (struct self *)pthread_getspecific(_threads.cache_key);
}

#if defined(__linux__)

static uint64_t
_thread_oid(void)
{
    return (uint64_t)gettid();
}
static bool
_thread_dead(struct self *self)
{
    return kill((pid_t)self->oid, 0) != 0;
}

#else  // !linux

static uint64_t
_thread_oid(void)
{
    return 0;
}

static bool
_thread_dead(struct self *self)
{
    // pthread_kill with signal 0 does not do anything with the thread, but
    // if an ESRCH error indicates the thread does not exist. See:
    // https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_kill.html
    //
    // This approach to figure out whether are thread is "gone" seems to
    // work most of the time. However, it is not perfect. It could give
    // false positives if the thread ID is reused by a new thread.
    //
    // In Linux, it causes the application to crash with segmentation fault. So
    // there we use the gettid and kill.
    return pthread_kill(self->pid, 0) == ESRCH;
}
#endif // !linux

DICE_HIDE struct self *
_create_self()
{
    struct self *self;
    self          = mempool_alloc(sizeof(struct self));
    self->guard   = 0;
    self->tid     = vatomic64_inc_get(&_threads.count);
    self->pid     = pthread_self();
    self->oid     = _thread_oid();
    self->retired = false;
    _tls_init(self);
    return self;
}

static void
_destroy_self(struct self *self)
{
    _tls_fini(self);
    mempool_free(self);
}


static void
_init_threads(void)
{
    _thread_cache_init();
    caslock_init(&_threads.lock);
    quack_init(&_threads.retired);
}

static struct self *
_get_self(void)
{
    // When a thread is created, it won't find its self object anywhere and this
    // function will return NULL. A new self object will be created and stored
    // in the thread specific area (here called cache).

    // When a thread is retired -- ie, between  THREAD_EXIT and SELF_FINI -- its
    // self object is put into the retired stack. The object is still kept in
    // the thread_cache.

    // So when getting self an existing self, we first look in the cache. If the
    // pthread implementation does not zero the thread specific area on exit,
    // the thread will find its object there until the end of the thread's
    // lifetime. However, if the pthread implementation zeros the thread
    // specific area on exit, we need to look for the thread in the retired
    // stack.

    pthread_t pid     = pthread_self();
    struct self *self = _thread_cache_get();
    if (self)
        return self;

    // We now search the retired stack. To avoid not seeing our self object
    // while other threads are searching theirs, we have to ensure mutual
    // exlusion here.
    caslock_acquire(&_threads.lock);
    struct quack_node_s *item = quack_popall(&_threads.retired);
    struct quack_node_s *next = NULL;

    for (; item; item = next) {
        next = item->next;

        struct self *it = container_of(item, struct self, retired_node);

        if (self == NULL && it->pid == pid)
            self = it;
        quack_push(&_threads.retired, item);
    }
    caslock_release(&_threads.lock);
    assert(self == NULL || self->retired);
    return self;
}

static void
_retire_self(struct self *self)
{
    assert(self);
    assert(!self->retired);
    self->retired = true;
    _thread_cache_del(self);
    quack_push(&_threads.retired, &self->retired_node);
}

// -----------------------------------------------------------------------------
//  pubsub handler
// -----------------------------------------------------------------------------

#define self_guard(chain, type, event, self)                                   \
    do {                                                                       \
        self->guard++;                                                         \
        self->md = (metadata_t){0};                                            \
        log_debug(">> [%lu:0x%lx:%lu] %u_%u: %d", self_id(&self->md),          \
                  (uint64_t)self->pid, self->oid, chain, type, self->guard);   \
        PS_PUBLISH(chain, type, event, &self->md);                             \
        log_debug("<< [%lu:0x%lx:%lu] %u_%u: %d", self_id(&self->md),          \
                  (uint64_t)self->pid, self->oid, chain, type, self->guard);   \
        self->guard--;                                                         \
    } while (0)

DICE_HIDE enum ps_err
_self_handle_before(const chain_id chain, const type_id type, void *event,
                    struct self *self)
{
    (void)chain;
    assert(self);

    if (likely(self->guard++ == 0))
        self_guard(CAPTURE_BEFORE, type, event, self);
    else
        log_debug(">>> [%lu:0x%lx:%lu] %u_%u: %d", self_id(&self->md),
                  (uint64_t)self->pid, self->oid, chain, type, self->guard);

    assert(self->guard >= 0);
    return PS_STOP_CHAIN;
}

DICE_HIDE enum ps_err
_self_handle_after(const chain_id chain, const type_id type, void *event,
                   struct self *self)
{
    (void)chain;
    assert(self);

    if (likely(self->guard-- == 1))
        self_guard(CAPTURE_AFTER, type, event, self);
    else
        log_debug("<<< [%lu:0x%lx:%lu] %u_%u: %d", self_id(&self->md),
                  (uint64_t)self->pid, self->oid, chain, type, self->guard);

    assert(self->guard >= 0);
    return PS_STOP_CHAIN;
}

DICE_HIDE enum ps_err
_self_handle_event(const chain_id chain, const type_id type, void *event,
                   struct self *self)
{
    (void)chain;
    assert(self);

    if (likely(self->guard == 0))
        self_guard(CAPTURE_EVENT, type, event, self);
    else
        log_debug("!!! [%lu:0x%lx:%lu] %u_%u: %d", self_id(&self->md),
                  (uint64_t)self->pid, self->oid, chain, type, self->guard);

    assert(self->guard >= 0);
    return PS_STOP_CHAIN;
}

static struct self *
_get_or_create_self(bool publish)
{
    struct self *self = _get_self();
    if (likely(self)) {
        return self;
    }

    self = _create_self();

    _thread_cache_set(self);

    if (publish)
        _self_handle_event(CAPTURE_EVENT, EVENT_SELF_INIT, 0, self);
    return self;
}

PS_SUBSCRIBE(INTERCEPT_EVENT, ANY_TYPE, {
    return _self_handle_event(chain, type, event, _get_or_create_self(true));
})
PS_SUBSCRIBE(INTERCEPT_BEFORE, ANY_TYPE, {
    if (type == EVENT_THREAD_CREATE)
        _cleanup_threads(0);
    return _self_handle_before(chain, type, event, _get_or_create_self(true));
})
PS_SUBSCRIBE(INTERCEPT_AFTER, ANY_TYPE, {
    return _self_handle_after(chain, type, event, _get_or_create_self(true));
})

// -----------------------------------------------------------------------------
// init, fini and registration
// -----------------------------------------------------------------------------

static void
_self_fini(struct self *self)
{
    if (self == NULL)
        return;

    // announce thread self is really over
    _self_handle_event(CAPTURE_EVENT, EVENT_SELF_FINI, 0, self);

    // free self resources
    _destroy_self(self);
}

static void
_cleanup_threads(pthread_t pid)
{
    caslock_acquire(&_threads.lock);
    struct quack_node_s *item = quack_popall(&_threads.retired);
    struct quack_node_s *next = NULL;

    for (; item; item = next) {
        next = item->next;

        struct self *self = container_of(item, struct self, retired_node);

        if ((pid != 0 && self->pid == pid) || (pid == 0 && _thread_dead(self)))
            _self_fini(self);
        else
            quack_push(&_threads.retired, item);
    }
    caslock_release(&_threads.lock);
}

PS_SUBSCRIBE(INTERCEPT_EVENT, EVENT_THREAD_EXIT, {
    struct self *self = _get_or_create_self(true);
    _self_handle_event(chain, type, event, self);
    _retire_self(self);
    return PS_STOP_CHAIN;
})

PS_SUBSCRIBE(INTERCEPT_AFTER, EVENT_THREAD_JOIN, {
    struct self *self             = _get_or_create_self(true);
    struct pthread_join_event *ev = EVENT_PAYLOAD(ev);
    _self_handle_after(chain, type, event, self);
    return PS_STOP_CHAIN;
})

DICE_MODULE_INIT({ _init_threads(); })
DICE_MODULE_FINI({
    _cleanup_threads(0);
    _self_fini(_get_self());
})
