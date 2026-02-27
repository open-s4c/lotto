#lang racket

(module binders racket
  (provide load-lib
           cfunc
           call
           mock)

  (require ffi/unsafe)
  (require (for-syntax racket/syntax))

  ;; ---------------------------------------------------------------------------
  ;;
  ;; ---------------------------------------------------------------------------

  ;; add the _ prefix to symbols
  (define-syntax (add-prefix stx)
    (syntax-case stx ()
      [(add-prefix pfx name)
       (format-id #'name "~a~a" #'pfx (syntax-e #'name))]))

  ;; get ffi obj for _sym and define it as 'sym
  (define-syntax-rule (define-ffi clib sym)
    (define sym (get-ffi-obj 'sym clib (add-prefix "_" sym))))

  (define (combine-sym . args)
    (let ([strs (map (lambda (s) (if (symbol? s) (symbol->string s) s)) args)])
      (string->symbol (apply string-append strs))))

  (define mock-funs (mutable-set))

  (define (mockoto-hook clib fun-sym fun-type mock-fun)
    (let* ([hook-type (_fun fun-type -> _void)]
           [hook-sym (combine-sym "mockoto_" fun-sym "_hook")]
           [hook-fun (get-ffi-obj hook-sym clib hook-type)])
      (set-add! mock-funs mock-fun)
      (hook-fun mock-fun)))

  (define-syntax-rule (get-ffi clib fun-sym)
    (let* ([fun-type (add-prefix "_" fun-sym)] ;
           [real-fun (get-ffi-obj 'fun-sym clib fun-type)])
      real-fun))

  (define-syntax-rule (call-ffi clib fun-sym args ...) ;
    ((get-ffi clib fun-sym) args ...))

  ;; the user can define here one library
  (define mockoto-lib '())

  (define (load-lib path #:custodian [custodian #f])
    (set! mockoto-lib (ffi-lib path #:custodian custodian)))

  (define-syntax-rule (mock fun-sym mock-fun)
    ;; ensure mockotolib set
    (mockoto-hook mockoto-lib 'fun-sym (add-prefix "_" fun-sym) mock-fun))

  (define-syntax-rule (call fun-sym args ...)
    ;; ensure mockotolib set
    (call-ffi mockoto-lib fun-sym args ...))

  (define-syntax-rule (cfunc sym)
    ;; ensure mockotolib set
    (get-ffi mockoto-lib sym)))

;; -----------------------------------------------------------------------------
;; binders public interface
;; -----------------------------------------------------------------------------
(require 'binders)
(provide (all-from-out 'binders)
         (all-defined-out))


;; -----------------------------------------------------------------------------
;; WARNING: auto-generated code - Changes will be lost!
;; -----------------------------------------------------------------------------

(require ffi/unsafe)
(define _size_t _uint64)
(define _uintptr_t _uint64)

;; typedef ___u_int
(define ___u_int _uint32)

;; typedef ___int32_t
(define ___int32_t _int32)

;; typedef ___uint32_t
(define ___uint32_t _uint32)

;; typedef ___int_least32_t
(define ___int_least32_t ___int32_t)

;; typedef ___uint_least32_t
(define ___uint_least32_t ___uint32_t)

;; typedef ___uid_t
(define ___uid_t _uint32)

;; typedef ___gid_t
(define ___gid_t _uint32)

;; typedef ___mode_t
(define ___mode_t _uint32)

;; typedef ___pid_t
(define ___pid_t _int32)

;; record ___fsid_t
(define-cstruct ___fsid_t
	([__val (_array/vector _int32 2)]
))
(define (alloc-__fsid_t)
	(cast (malloc 'raw (ctype-sizeof ___fsid_t))
	      _pointer
	      ___fsid_t-pointer))

;; typedef ___fsid_t
;; ___fsid_t already defined



;; typedef ___id_t
(define ___id_t _uint32)

;; typedef ___useconds_t
(define ___useconds_t _uint32)

;; typedef ___daddr_t
(define ___daddr_t _int32)

;; typedef ___key_t
(define ___key_t _int32)

;; typedef ___clockid_t
(define ___clockid_t _int32)

;; typedef ___timer_t
(define ___timer_t _pointer)

;; typedef ___caddr_t
(define ___caddr_t _string)

;; typedef ___socklen_t
(define ___socklen_t _uint32)

;; typedef ___sig_atomic_t
(define ___sig_atomic_t _int32)

;; typedef _int32_t
(define _int32_t ___int32_t)

;; typedef _uint32_t
(define _uint32_t ___uint32_t)

;; typedef _int_least32_t
(define _int_least32_t ___int_least32_t)

;; typedef _uint_least32_t
(define _uint_least32_t ___uint_least32_t)

;; typedef _wchar_t
(define _wchar_t _int32)

;; record _div_t
(define-cstruct _div_t
	([quot _int32]
	 [rem _int32]
))
(define (alloc-div_t)
	(cast (malloc 'raw (ctype-sizeof _div_t))
	      _pointer
	      _div_t-pointer))

;; typedef _div_t
;; _div_t already defined



;; typedef _ldiv_t
;; _ldiv_t already defined



;; typedef _lldiv_t
;; _lldiv_t already defined



;; function ___ctype_get_mb_cur_max
(define ___ctype_get_mb_cur_max
	(_fun  -> _size_t))

;; function _atoi
(define _atoi
	(_fun _string -> _int32))

;; typedef _clockid_t
(define _clockid_t ___clockid_t)

;; typedef _timer_t
(define _timer_t _pointer)

;; function ___bswap_32
(define ___bswap_32
	(_fun ___uint32_t -> ___uint32_t))

;; function ___uint32_identity
(define ___uint32_identity
	(_fun ___uint32_t -> ___uint32_t))

;; typedef ___sigset_t
;; ___sigset_t already defined



;; typedef _fd_set
;; _fd_set already defined



;; record _anon_11
(define-cstruct _anon_11
	([__low _uint32]
	 [__high _uint32]
))
(define (alloc-anon_11)
	(cast (malloc 'raw (ctype-sizeof _anon_11))
	      _pointer
	      _anon_11-pointer))

;; typedef ___atomic_wide_counter
;; ___atomic_wide_counter already defined



;; record ___pthread_internal_list
(define-cstruct ___pthread_internal_list
	([__prev _pointer]
	 [__next _pointer]
))
(define (alloc-__pthread_internal_list)
	(cast (malloc 'raw (ctype-sizeof ___pthread_internal_list))
	      _pointer
	      ___pthread_internal_list-pointer))

;; typedef ___pthread_list_t
(define ___pthread_list_t ___pthread_internal_list)

;; record ___pthread_internal_slist
(define-cstruct ___pthread_internal_slist
	([__next _pointer]
))
(define (alloc-__pthread_internal_slist)
	(cast (malloc 'raw (ctype-sizeof ___pthread_internal_slist))
	      _pointer
	      ___pthread_internal_slist-pointer))

;; typedef ___pthread_slist_t
(define ___pthread_slist_t ___pthread_internal_slist)

;; typedef ___tss_t
(define ___tss_t _uint32)

;; record ___once_flag
(define-cstruct ___once_flag
	([__data _int32]
))
(define (alloc-__once_flag)
	(cast (malloc 'raw (ctype-sizeof ___once_flag))
	      _pointer
	      ___once_flag-pointer))

;; typedef ___once_flag
;; ___once_flag already defined



;; record _pthread_mutexattr_t
(define _pthread_mutexattr_t
	(_union (_array/vector _byte 4)
		_int32
))

;; typedef _pthread_mutexattr_t
;; _pthread_mutexattr_t already defined



;; record _pthread_condattr_t
(define _pthread_condattr_t
	(_union (_array/vector _byte 4)
		_int32
))

;; typedef _pthread_condattr_t
;; _pthread_condattr_t already defined



;; typedef _pthread_key_t
(define _pthread_key_t _uint32)

;; typedef _pthread_once_t
(define _pthread_once_t _int32)

;; typedef _pthread_attr_t
;; _pthread_attr_t already defined



;; typedef _pthread_mutex_t
;; _pthread_mutex_t already defined



;; typedef _pthread_cond_t
;; _pthread_cond_t already defined



;; typedef _pthread_rwlock_t
;; _pthread_rwlock_t already defined



;; typedef _pthread_rwlockattr_t
;; _pthread_rwlockattr_t already defined



;; typedef _pthread_barrier_t
;; _pthread_barrier_t already defined



;; record _pthread_barrierattr_t
(define _pthread_barrierattr_t
	(_union (_array/vector _byte 4)
		_int32
))

;; typedef _pthread_barrierattr_t
;; _pthread_barrierattr_t already defined



;; function _srandom
(define _srandom
	(_fun _uint32 -> _void))

;; function _initstate
(define _initstate
	(_fun _uint32 _string _size_t -> _string))

;; function _setstate
(define _setstate
	(_fun _string -> _string))

;; record _random_data
(define-cstruct _random_data
	([fptr (_cpointer _int32_t) ]
	 [rptr (_cpointer _int32_t) ]
	 [state (_cpointer _int32_t) ]
	 [rand_type _int32]
	 [rand_deg _int32]
	 [rand_sep _int32]
	 [end_ptr (_cpointer _int32_t) ]
))
(define (alloc-random_data)
	(cast (malloc 'raw (ctype-sizeof _random_data))
	      _pointer
	      _random_data-pointer))

;; function _random_r
(define _random_r
	(_fun (_or-null _random_data-pointer) (_cpointer _int32_t)  -> _int32))

;; function _srandom_r
(define _srandom_r
	(_fun _uint32 (_or-null _random_data-pointer) -> _int32))

;; function _initstate_r
(define _initstate_r
	(_fun _uint32 _string _size_t (_or-null _random_data-pointer) -> _int32))

;; function _setstate_r
(define _setstate_r
	(_fun _string (_or-null _random_data-pointer) -> _int32))

;; function _rand
(define _rand
	(_fun  -> _int32))

;; function _srand
(define _srand
	(_fun _uint32 -> _void))

;; function _rand_r
(define _rand_r
	(_fun (_cpointer _uint32)  -> _int32))

;; function _arc4random
(define _arc4random
	(_fun  -> ___uint32_t))

;; function _arc4random_buf
(define _arc4random_buf
	(_fun _pointer _size_t -> _void))

;; function _arc4random_uniform
(define _arc4random_uniform
	(_fun ___uint32_t -> ___uint32_t))

;; function _free
(define _free
	(_fun _pointer -> _void))

;; function _reallocarray
(define _reallocarray
	(_fun _pointer _size_t _size_t -> _pointer))

;; function _reallocarray
;; _reallocarray already defined



;; function _valloc
(define _valloc
	(_fun _size_t -> _pointer))

;; function _abort
(define _abort
	(_fun  -> _void))

;; function _atexit
(define _atexit
	(_fun (_fun  -> _void) -> _int32))

;; function _at_quick_exit
(define _at_quick_exit
	(_fun (_fun  -> _void) -> _int32))

;; function _on_exit
(define _on_exit
	(_fun (_fun _int32 _pointer -> _void) _pointer -> _int32))

;; function _exit
(define _exit
	(_fun _int32 -> _void))

;; function _quick_exit
(define _quick_exit
	(_fun _int32 -> _void))

;; function __Exit
(define __Exit
	(_fun _int32 -> _void))

;; function _getenv
(define _getenv
	(_fun _string -> _string))

;; function _putenv
(define _putenv
	(_fun _string -> _int32))

;; function _setenv
(define _setenv
	(_fun _string _string _int32 -> _int32))

;; function _unsetenv
(define _unsetenv
	(_fun _string -> _int32))

;; function _clearenv
(define _clearenv
	(_fun  -> _int32))

;; function _mktemp
(define _mktemp
	(_fun _string -> _string))

;; function _mkstemp
(define _mkstemp
	(_fun _string -> _int32))

;; function _mkstemps
(define _mkstemps
	(_fun _string _int32 -> _int32))

;; function _mkdtemp
(define _mkdtemp
	(_fun _string -> _string))

;; function _system
(define _system
	(_fun _string -> _int32))

;; function _realpath
(define _realpath
	(_fun _string _string -> _string))

;; typedef ___compar_fn_t
(define ___compar_fn_t
	(_fun _pointer _pointer -> _int32))

;; function _bsearch
(define _bsearch
	(_fun _pointer _pointer _size_t _size_t (_fun _pointer _pointer -> _int32) -> _pointer))

;; function _qsort
(define _qsort
	(_fun _pointer _size_t _size_t (_fun _pointer _pointer -> _int32) -> _void))

;; function _abs
(define _abs
	(_fun _int32 -> _int32))

;; function _div
(define _div
	(_fun _int32 _int32 -> _div_t))

;; function _mblen
(define _mblen
	(_fun _string _size_t -> _int32))

;; function _mbtowc
(define _mbtowc
	(_fun (_cpointer _wchar_t)  _string _size_t -> _int32))

;; function _wctomb
(define _wctomb
	(_fun _string _wchar_t -> _int32))

;; function _mbstowcs
(define _mbstowcs
	(_fun (_cpointer _wchar_t)  _string _size_t -> _size_t))

;; function _wcstombs
(define _wcstombs
	(_fun _string (_cpointer _wchar_t)  _size_t -> _size_t))

;; function _rpmatch
(define _rpmatch
	(_fun _string -> _int32))

;; typedef _clk_t
(define _clk_t _uint64)

;; function _clk_parse
(define _clk_parse
	(_fun _string -> _clk_t))

;; record _base_category
(define _base_category
	(_enum '(CAT_NONE = 0
		CAT_BEFORE_WRITE = 1
		CAT_BEFORE_READ = 2
		CAT_BEFORE_AWRITE = 3
		CAT_BEFORE_AREAD = 4
		CAT_BEFORE_XCHG = 5
		CAT_BEFORE_RMW = 6
		CAT_BEFORE_CMPXCHG = 7
		CAT_BEFORE_FENCE = 8
		CAT_AFTER_AWRITE = 9
		CAT_AFTER_AREAD = 10
		CAT_AFTER_XCHG = 11
		CAT_AFTER_RMW = 12
		CAT_AFTER_CMPXCHG_S = 13
		CAT_AFTER_CMPXCHG_F = 14
		CAT_AFTER_FENCE = 15
		CAT_CALL = 16
		CAT_TASK_BLOCK = 17
		CAT_TASK_CREATE = 18
		CAT_TASK_INIT = 19
		CAT_TASK_FINI = 20
		CAT_MUTEX_ACQUIRE = 21
		CAT_MUTEX_TRYACQUIRE = 22
		CAT_MUTEX_RELEASE = 23
		CAT_RSRC_ACQUIRING = 24
		CAT_RSRC_RELEASED = 25
		CAT_SYS_YIELD = 26
		CAT_USER_YIELD = 27
		CAT_REGION_PREEMPTION = 28
		CAT_FUNC_ENTRY = 29
		CAT_FUNC_EXIT = 30
		CAT_LOG_BEFORE = 31
		CAT_LOG_AFTER = 32
		CAT_REGION_IN = 33
		CAT_REGION_OUT = 34
		CAT_EVEC_PREPARE = 35
		CAT_EVEC_WAIT = 36
		CAT_EVEC_TIMED_WAIT = 37
		CAT_EVEC_CANCEL = 38
		CAT_EVEC_WAKE = 39
		CAT_EVEC_MOVE = 40
		CAT_ENFORCE = 41
		CAT_POLL = 42
		CAT_TASK_VELOCITY = 43
		CAT_KEY_CREATE = 44
		CAT_KEY_DELETE = 45
		CAT_SET_SPECIFIC = 46
		CAT_JOIN = 47
		CAT_DETACH = 48
		CAT_EXIT = 49
		CAT_END_ = 50
)))

(define _base_category-domain
	'(CAT_NONE
		CAT_BEFORE_WRITE
		CAT_BEFORE_READ
		CAT_BEFORE_AWRITE
		CAT_BEFORE_AREAD
		CAT_BEFORE_XCHG
		CAT_BEFORE_RMW
		CAT_BEFORE_CMPXCHG
		CAT_BEFORE_FENCE
		CAT_AFTER_AWRITE
		CAT_AFTER_AREAD
		CAT_AFTER_XCHG
		CAT_AFTER_RMW
		CAT_AFTER_CMPXCHG_S
		CAT_AFTER_CMPXCHG_F
		CAT_AFTER_FENCE
		CAT_CALL
		CAT_TASK_BLOCK
		CAT_TASK_CREATE
		CAT_TASK_INIT
		CAT_TASK_FINI
		CAT_MUTEX_ACQUIRE
		CAT_MUTEX_TRYACQUIRE
		CAT_MUTEX_RELEASE
		CAT_RSRC_ACQUIRING
		CAT_RSRC_RELEASED
		CAT_SYS_YIELD
		CAT_USER_YIELD
		CAT_REGION_PREEMPTION
		CAT_FUNC_ENTRY
		CAT_FUNC_EXIT
		CAT_LOG_BEFORE
		CAT_LOG_AFTER
		CAT_REGION_IN
		CAT_REGION_OUT
		CAT_EVEC_PREPARE
		CAT_EVEC_WAIT
		CAT_EVEC_TIMED_WAIT
		CAT_EVEC_CANCEL
		CAT_EVEC_WAKE
		CAT_EVEC_MOVE
		CAT_ENFORCE
		CAT_POLL
		CAT_TASK_VELOCITY
		CAT_KEY_CREATE
		CAT_KEY_DELETE
		CAT_SET_SPECIFIC
		CAT_JOIN
		CAT_DETACH
		CAT_EXIT
		CAT_END_
))

;; typedef _category_t
(define _category_t _base_category)

;; function _base_category_str
(define _base_category_str
	(_fun _base_category -> _string))

;; typedef _task_id
(define _task_id _uint64)

;; function _get_task_count
(define _get_task_count
	(_fun  -> _task_id))

;; function _sys_abort
(define _sys_abort
	(_fun  -> _void))

;; function _sys_assert_fail
(define _sys_assert_fail
	(_fun _string _string _uint32 _string -> _void))

;; function ___memcmpeq
(define ___memcmpeq
	(_fun _pointer _pointer _size_t -> _int32))

;; function _strcpy
(define _strcpy
	(_fun _string _string -> _string))

;; function _strcat
(define _strcat
	(_fun _string _string -> _string))

;; function _strcmp
(define _strcmp
	(_fun _string _string -> _int32))

;; function _strcoll
(define _strcoll
	(_fun _string _string -> _int32))

;; function _strdup
(define _strdup
	(_fun _string -> _string))

;; function _strchr
(define _strchr
	(_fun _string _int32 -> _string))

;; function _strrchr
(define _strrchr
	(_fun _string _int32 -> _string))

;; function _strchrnul
(define _strchrnul
	(_fun _string _int32 -> _string))

;; function _strpbrk
(define _strpbrk
	(_fun _string _string -> _string))

;; function _strstr
(define _strstr
	(_fun _string _string -> _string))

;; function _strtok
(define _strtok
	(_fun _string _string -> _string))

;; function _strcasestr
(define _strcasestr
	(_fun _string _string -> _string))

;; function _memmem
(define _memmem
	(_fun _pointer _size_t _pointer _size_t -> _pointer))

;; function ___mempcpy
(define ___mempcpy
	(_fun _pointer _pointer _size_t -> _pointer))

;; function _strnlen
(define _strnlen
	(_fun _string _size_t -> _size_t))

;; function _strerror
(define _strerror
	(_fun _int32 -> _string))

;; function _strerror_r
(define _strerror_r
	(_fun _int32 _string _size_t -> _int32))

;; function _index
(define _index
	(_fun _string _int32 -> _string))

;; function _rindex
(define _rindex
	(_fun _string _int32 -> _string))

;; function _ffs
(define _ffs
	(_fun _int32 -> _int32))

;; function _strcasecmp
(define _strcasecmp
	(_fun _string _string -> _int32))

;; function _explicit_bzero
(define _explicit_bzero
	(_fun _pointer _size_t -> _void))

;; function _strsignal
(define _strsignal
	(_fun _int32 -> _string))

;; function ___stpcpy
(define ___stpcpy
	(_fun _string _string -> _string))

;; function _stpcpy
(define _stpcpy
	(_fun _string _string -> _string))

;; function ___stpncpy
(define ___stpncpy
	(_fun _string _string _size_t -> _string))

;; function _strlcpy
(define _strlcpy
	(_fun _string _string _size_t -> _size_t))

;; function _strlcat
(define _strlcat
	(_fun _string _string _size_t -> _size_t))

;; function _sys_memcpy
(define _sys_memcpy
	(_fun _pointer _pointer _size_t -> _pointer))

;; function _sys_memset
(define _sys_memset
	(_fun _pointer _int32 _size_t -> _pointer))

;; function _sys_stpcpy
(define _sys_stpcpy
	(_fun _string _string -> _string))

;; function _sys_strcpy
(define _sys_strcpy
	(_fun _string _string -> _string))

;; function _sys_strcat
(define _sys_strcat
	(_fun _string _string -> _string))

;; function _sys_strcmp
(define _sys_strcmp
	(_fun _string _string -> _int32))

;; function _sys_strncmp
(define _sys_strncmp
	(_fun _string _string _size_t -> _int32))

;; function _sys_memcmp
(define _sys_memcmp
	(_fun _pointer _pointer _size_t -> _int32))

;; function _sys_strlen
(define _sys_strlen
	(_fun _string -> _size_t))

;; function _sys_memmove
(define _sys_memmove
	(_fun _pointer _pointer _size_t -> _pointer))

;; function _sys_strdup
(define _sys_strdup
	(_fun _string -> _string))

;; function _sys_strndup
(define _sys_strndup
	(_fun _string _size_t -> _string))

;; record _arg_u128
(define-cstruct _arg_u128
	([bytes (_array/vector _uint8 16)]
))
(define (alloc-arg_u128)
	(cast (malloc 'raw (ctype-sizeof _arg_u128))
	      _pointer
	      _arg_u128-pointer))

;; typedef _arg_u128_t
(define _arg_u128_t _arg_u128)

;; record _arg_width
(define _arg_width
	(_enum '(ARG_EMPTY = 0
		ARG_U8 = 1
		ARG_U16 = 2
		ARG_U32 = 3
		ARG_U64 = 4
		ARG_U128 = 5
		ARG_PTR = 6
)))

(define _arg_width-domain
	'(ARG_EMPTY
		ARG_U8
		ARG_U16
		ARG_U32
		ARG_U64
		ARG_U128
		ARG_PTR
))

;; record _arg_value
(define _arg_value
	(_union _uint8
		_uint16
		_uint32
		_uint64
		_arg_u128
		_uintptr_t
))

;; record _arg
(define-cstruct _arg
	([width _arg_width]
	 [value _arg_value]
))
(define (alloc-arg)
	(cast (malloc 'raw (ctype-sizeof _arg))
	      _pointer
	      _arg-pointer))

;; typedef _arg_t
(define _arg_t _arg)

;; record _context
(define-cstruct _context
	([cat _base_category]
	 [id _task_id]
	 [vid _task_id]
	 [pc _uintptr_t]
	 [func _string]
	 [func_addr _uintptr_t]
	 [args (_array/vector _arg 4)]
))
(define (alloc-context)
	(cast (malloc 'raw (ctype-sizeof _context))
	      _pointer
	      _context-pointer))

;; typedef _context_t
(define _context_t _context)

;; function _context_print
(define _context_print
	(_fun (_or-null _context-pointer) -> _void))

;; typedef _max_align_t
;; _max_align_t already defined



;; record _reason
(define _reason
	(_enum '(REASON_UNKNOWN = 0
		REASON_DETERMINISTIC = 1
		REASON_NONDETERMINISTIC = 2
		REASON_CALL = 3
		REASON_WATCHDOG = 4
		REASON_SYS_YIELD = 5
		REASON_USER_YIELD = 6
		REASON_USER_ORDER = 7
		REASON_ASSERT_FAIL = 8
		REASON_RSRC_DEADLOCK = 9
		REASON_SEGFAULT = 10
		REASON_SIGINT = 11
		REASON_SIGABRT = 12
		REASON_SIGTERM = 13
		REASON_SIGKILL = 14
		REASON_SUCCESS = 15
		REASON_IGNORE = 16
		REASON_ABORT = 17
		REASON_SHUTDOWN = 18
		REASON_RUNTIME_SEGFAULT = 19
		REASON_RUNTIME_SIGINT = 20
		REASON_RUNTIME_SIGABRT = 21
		REASON_RUNTIME_SIGTERM = 22
		REASON_RUNTIME_SIGKILL = 23
		REASON_IMPASSE = 24
		REASON_END_ = 25
)))

(define _reason-domain
	'(REASON_UNKNOWN
		REASON_DETERMINISTIC
		REASON_NONDETERMINISTIC
		REASON_CALL
		REASON_WATCHDOG
		REASON_SYS_YIELD
		REASON_USER_YIELD
		REASON_USER_ORDER
		REASON_ASSERT_FAIL
		REASON_RSRC_DEADLOCK
		REASON_SEGFAULT
		REASON_SIGINT
		REASON_SIGABRT
		REASON_SIGTERM
		REASON_SIGKILL
		REASON_SUCCESS
		REASON_IGNORE
		REASON_ABORT
		REASON_SHUTDOWN
		REASON_RUNTIME_SEGFAULT
		REASON_RUNTIME_SIGINT
		REASON_RUNTIME_SIGABRT
		REASON_RUNTIME_SIGTERM
		REASON_RUNTIME_SIGKILL
		REASON_IMPASSE
		REASON_END_
))

;; typedef _reason_t
(define _reason_t _reason)

;; function _reason_str
(define _reason_str
	(_fun _reason -> _string))

;; function _reason_is_runtime
(define _reason_is_runtime
	(_fun _reason -> _bool))

;; function _reason_is_shutdown
(define _reason_is_shutdown
	(_fun _reason -> _bool))

;; function _reason_is_abort
(define _reason_is_abort
	(_fun _reason -> _bool))

;; function _reason_is_terminate
(define _reason_is_terminate
	(_fun _reason -> _bool))

;; function _reason_is_record_final
(define _reason_is_record_final
	(_fun _reason -> _bool))

;; record _record
(define _record
	(_enum '(RECORD_NONE = 0
		RECORD_INFO = 1
		RECORD_SCHED = 2
		RECORD_START = 4
		RECORD_EXIT = 8
		RECORD_CONFIG = 16
		RECORD_SHUTDOWN_LOCK = 32
		RECORD_OPAQUE = 64
		RECORD_FORCE = 128
		_RECORD_LAST = 128
)))

(define _record-domain
	'(RECORD_NONE
		RECORD_INFO
		RECORD_SCHED
		RECORD_START
		RECORD_EXIT
		RECORD_CONFIG
		RECORD_SHUTDOWN_LOCK
		RECORD_OPAQUE
		RECORD_FORCE
		_RECORD_LAST
))

;; typedef _kinds_t
(define _kinds_t _uint64)

;; function _kind_str
(define _kind_str
	(_fun _record -> _string))

;; record _record_s
(define-cstruct _record_s
	([next _pointer]
	 [id _task_id]
	 [clk _clk_t]
	 [cat _base_category]
	 [reason _reason]
	 [kind _record]
	 [size _size_t]
	 [pc _uintptr_t]
	 ;; error: [data (_array/vector _byte 0)]
))
(define (alloc-record_s)
	(cast (malloc 'raw (ctype-sizeof _record_s))
	      _pointer
	      _record_s-pointer))

;; typedef _record_t
(define _record_t _record_s)

;; function _record_alloc
(define _record_alloc
	(_fun _size_t -> (_or-null _record_s-pointer)))

;; function _record_clone
(define _record_clone
	(_fun (_or-null _record_s-pointer) -> (_or-null _record_s-pointer)))

;; record _replay_status
(define _replay_status
	(_enum '(REPLAY_DONE = 0
		REPLAY_LOAD = 1
		REPLAY_FORCE = 2
		REPLAY_CONT = 3
		REPLAY_NEXT = 4
		REPLAY_LAST = 5
)))

(define _replay_status-domain
	'(REPLAY_DONE
		REPLAY_LOAD
		REPLAY_FORCE
		REPLAY_CONT
		REPLAY_NEXT
		REPLAY_LAST
))

;; record _replay
(define-cstruct _replay
	([status _replay_status]
	 [id _task_id]
))
(define (alloc-replay)
	(cast (malloc 'raw (ctype-sizeof _replay))
	      _pointer
	      _replay-pointer))

;; typedef _replay_t
(define _replay_t _replay)

;; function _recorder_record
(define _recorder_record
	(_fun (_or-null _context-pointer) _clk_t -> _void))

;; function _recorder_replay
(define _recorder_replay
	(_fun _clk_t -> _replay))

;; function _recorder_fini
(define _recorder_fini
	(_fun _clk_t _task_id _reason -> _void))

;; function _recorder_end_trace
;; function without prototype


;; function _recorder_end_replay
;; function without prototype


;; record _anon_40
(define _anon_40
	(_union _uint32
		(_array/vector _byte 4)
))

;; typedef ___mbstate_t
;; ___mbstate_t already defined



;; typedef __IO_lock_t
(define __IO_lock_t _void)

;; function _remove
(define _remove
	(_fun _string -> _int32))

;; function _rename
(define _rename
	(_fun _string _string -> _int32))

;; function _renameat
(define _renameat
	(_fun _int32 _string _int32 _string -> _int32))

;; function _tmpnam
(define _tmpnam
	(_fun _string -> _string))

;; function _tmpnam_r
(define _tmpnam_r
	(_fun _string -> _string))

;; function _tempnam
(define _tempnam
	(_fun _string _string -> _string))

;; function _printf
(define _printf
	(_fun _string -> _int32))

;; function _sprintf
(define _sprintf
	(_fun _string _string -> _int32))

;; function _dprintf
(define _dprintf
	(_fun _int32 _string -> _int32))

;; function _scanf
(define _scanf
	(_fun _string -> _int32))

;; function _sscanf
(define _sscanf
	(_fun _string _string -> _int32))

;; function _fscanf
;; _fscanf already defined



;; function _scanf
;; _scanf already defined



;; function _sscanf
;; _sscanf already defined



;; function _vfscanf
;; _vfscanf already defined



;; function _vscanf
;; _vscanf already defined



;; function _vsscanf
;; _vsscanf already defined



;; function _getchar
(define _getchar
	(_fun  -> _int32))

;; function _getchar_unlocked
(define _getchar_unlocked
	(_fun  -> _int32))

;; function _putchar
(define _putchar
	(_fun _int32 -> _int32))

;; function _putchar_unlocked
(define _putchar_unlocked
	(_fun _int32 -> _int32))

;; function _puts
(define _puts
	(_fun _string -> _int32))

;; function _perror
(define _perror
	(_fun _string -> _void))

;; function _ctermid
(define _ctermid
	(_fun _string -> _string))

;; record _stream_s
(define-cstruct _stream_s
	([write (_fun _pointer _string _size_t -> _size_t)]
	 [read (_fun _pointer _string _size_t -> _size_t)]
	 [close (_fun _pointer -> _void)]
	 [reset (_fun _pointer -> _void)]
))
(define (alloc-stream_s)
	(cast (malloc 'raw (ctype-sizeof _stream_s))
	      _pointer
	      _stream_s-pointer))

;; typedef _stream_t
(define _stream_t _stream_s)

;; function _stream_write
(define _stream_write
	(_fun (_or-null _stream_s-pointer) _string _size_t -> _size_t))

;; function _stream_close
(define _stream_close
	(_fun (_or-null _stream_s-pointer) -> _void))

;; function _stream_file_alloc
(define _stream_file_alloc
	(_fun  -> (_or-null _stream_s-pointer)))

;; function _stream_file_in
(define _stream_file_in
	(_fun (_or-null _stream_s-pointer) _string -> _void))

;; function _stream_file_init
(define _stream_file_init
	(_fun (_or-null _stream_s-pointer) _int32 -> _void))

;; function _stream_file_out
(define _stream_file_out
	(_fun (_or-null _stream_s-pointer) _string -> _void))

;; function _stream_read
(define _stream_read
	(_fun (_or-null _stream_s-pointer) _string _size_t -> _size_t))

;; function _stream_reset
(define _stream_reset
	(_fun (_or-null _stream_s-pointer) -> _void))

;; pending:   _FILE
;; missing -> __IO_FILE


;; pending:   __Float32
;; missing -> _float


;; pending:   __Float32x
;; missing -> _double


;; pending:   __Float64
;; missing -> _double


;; pending:   __Float64x
;; missing -> _long double


;; pending:   __G_fpos64_t
;; missing -> ___mbstate_t ___off64_t


;; pending:   __G_fpos_t
;; missing -> ___mbstate_t ___off_t


;; pending:   __IO_FILE
;; missing -> __IO_codecvt __IO_marker __IO_wide_data ___off64_t ___off_t _signed char _unsigned short


;; postponed
;; record __IO_codecvt
;; incomplete definition
(define __IO_codecvt _void)(define __IO_codecvt-pointer _pointer)

;; pending:   __IO_cookie_io_functions_t
;; missing -> ___off64_t ___ssize_t


;; postponed
;; record __IO_marker
;; incomplete definition
(define __IO_marker _void)(define __IO_marker-pointer _pointer)

;; postponed
;; record __IO_wide_data
;; incomplete definition
(define __IO_wide_data _void)(define __IO_wide_data-pointer _pointer)

;; pending:   ___FILE
;; missing -> __IO_FILE


;; pending:   ___asprintf
;; missing -> _char *


;; pending:   ___atomic_wide_counter
;; missing -> _(anonymous union)::struct (unnamed at /usr/include/x86_64-linux-gnu/bits/atomic_wide_counter.h:28:3) _unsigned long long


;; pending:   ___blkcnt64_t
;; missing -> _long


;; pending:   ___blkcnt_t
;; missing -> _long


;; pending:   ___blksize_t
;; missing -> _long


;; pending:   ___bswap_16
;; missing -> ___uint16_t


;; pending:   ___bswap_64
;; missing -> ___uint64_t


;; pending:   ___clock_t
;; missing -> _long


;; pending:   ___dev_t
;; missing -> _unsigned long


;; pending:   ___fd_mask
;; missing -> _long


;; pending:   ___fpos64_t
;; missing -> __G_fpos64_t


;; pending:   ___fpos_t
;; missing -> __G_fpos_t


;; pending:   ___fsblkcnt64_t
;; missing -> _unsigned long


;; pending:   ___fsblkcnt_t
;; missing -> _unsigned long


;; pending:   ___fsfilcnt64_t
;; missing -> _unsigned long


;; pending:   ___fsfilcnt_t
;; missing -> _unsigned long


;; pending:   ___fsword_t
;; missing -> _long


;; pending:   ___getdelim
;; missing -> __IO_FILE ___ssize_t _char *


;; pending:   ___gnuc_va_list
;; missing -> ___va_list_tag


;; pending:   ___ino64_t
;; missing -> _unsigned long


;; pending:   ___ino_t
;; missing -> _unsigned long


;; pending:   ___int16_t
;; missing -> _short


;; pending:   ___int64_t
;; missing -> _long


;; pending:   ___int8_t
;; missing -> _signed char


;; pending:   ___int_least16_t
;; missing -> ___int16_t


;; pending:   ___int_least64_t
;; missing -> ___int64_t


;; pending:   ___int_least8_t
;; missing -> ___int8_t


;; pending:   ___intmax_t
;; missing -> _long


;; pending:   ___intptr_t
;; missing -> _long


;; postponed
;; record ___locale_data
;; incomplete definition
(define ___locale_data _void)(define ___locale_data-pointer _pointer)

;; pending:   ___locale_struct
;; missing -> _unsigned short


;; pending:   ___locale_t
;; missing -> ___locale_struct


;; pending:   ___loff_t
;; missing -> ___off64_t


;; pending:   ___mbstate_t
;; missing -> _(anonymous struct)::union (unnamed at /usr/include/x86_64-linux-gnu/bits/types/__mbstate_t.h:16:3)


;; pending:   ___nlink_t
;; missing -> _unsigned long


;; pending:   ___off64_t
;; missing -> _long


;; pending:   ___off_t
;; missing -> _long


;; pending:   ___overflow
;; missing -> __IO_FILE


;; pending:   ___pthread_cond_s
;; missing -> ___atomic_wide_counter


;; pending:   ___pthread_mutex_s
;; missing -> _short


;; pending:   ___pthread_rwlock_arch_t
;; missing -> _signed char _unsigned char _unsigned long


;; pending:   ___quad_t
;; missing -> _long


;; pending:   ___rlim64_t
;; missing -> _unsigned long


;; pending:   ___rlim_t
;; missing -> _unsigned long


;; pending:   ___sigset_t
;; missing -> _unsigned long


;; pending:   ___ssize_t
;; missing -> _long


;; pending:   ___strtok_r
;; missing -> _char *


;; pending:   ___suseconds64_t
;; missing -> _long


;; pending:   ___suseconds_t
;; missing -> _long


;; pending:   ___syscall_slong_t
;; missing -> _long


;; pending:   ___syscall_ulong_t
;; missing -> _unsigned long


;; pending:   ___thrd_t
;; missing -> _unsigned long


;; pending:   ___time_t
;; missing -> _long


;; pending:   ___u_char
;; missing -> _unsigned char


;; pending:   ___u_long
;; missing -> _unsigned long


;; pending:   ___u_quad_t
;; missing -> _unsigned long


;; pending:   ___u_short
;; missing -> _unsigned short


;; pending:   ___uflow
;; missing -> __IO_FILE


;; pending:   ___uint16_identity
;; missing -> ___uint16_t


;; pending:   ___uint16_t
;; missing -> _unsigned short


;; pending:   ___uint64_identity
;; missing -> ___uint64_t


;; pending:   ___uint64_t
;; missing -> _unsigned long


;; pending:   ___uint8_t
;; missing -> _unsigned char


;; pending:   ___uint_least16_t
;; missing -> ___uint16_t


;; pending:   ___uint_least64_t
;; missing -> ___uint64_t


;; pending:   ___uint_least8_t
;; missing -> ___uint8_t


;; pending:   ___uintmax_t
;; missing -> _unsigned long


;; pending:   _a64l
;; missing -> _long


;; pending:   _aligned_alloc
;; missing -> _unsigned long


;; pending:   _alloca
;; missing -> _unsigned long


;; pending:   _asprintf
;; missing -> _char *


;; pending:   _atof
;; missing -> _double


;; pending:   _atol
;; missing -> _long


;; pending:   _atoll
;; missing -> _long long


;; pending:   _bcmp
;; missing -> _unsigned long


;; pending:   _bcopy
;; missing -> _unsigned long


;; pending:   _bzero
;; missing -> _unsigned long


;; pending:   _calloc
;; missing -> _unsigned long


;; pending:   _clearerr
;; missing -> __IO_FILE


;; pending:   _clearerr_unlocked
;; missing -> __IO_FILE


;; pending:   _clock_t
;; missing -> ___clock_t


;; pending:   _cookie_close_function_t
;; missing -> _int (void *)


;; pending:   _cookie_io_functions_t
;; missing -> __IO_cookie_io_functions_t


;; pending:   _cookie_read_function_t
;; missing -> ___ssize_t (void *, char *, size_t)


;; pending:   _cookie_seek_function_t
;; missing -> _int (void *, __off64_t *, int)


;; pending:   _cookie_write_function_t
;; missing -> ___ssize_t (void *, const char *, size_t)


;; pending:   _drand48
;; missing -> _double


;; pending:   _drand48_data
;; missing -> _unsigned long long _unsigned short


;; pending:   _drand48_r
;; missing -> _double _drand48_data


;; pending:   _ecvt
;; missing -> _double


;; pending:   _ecvt_r
;; missing -> _double


;; pending:   _erand48
;; missing -> _double _unsigned short


;; pending:   _erand48_r
;; missing -> _double _drand48_data _unsigned short


;; pending:   _fclose
;; missing -> __IO_FILE


;; pending:   _fcvt
;; missing -> _double


;; pending:   _fcvt_r
;; missing -> _double


;; pending:   _fd_mask
;; missing -> ___fd_mask


;; pending:   _fd_set
;; missing -> ___fd_mask


;; pending:   _fdopen
;; missing -> __IO_FILE


;; pending:   _feof
;; missing -> __IO_FILE


;; pending:   _feof_unlocked
;; missing -> __IO_FILE


;; pending:   _ferror
;; missing -> __IO_FILE


;; pending:   _ferror_unlocked
;; missing -> __IO_FILE


;; pending:   _fflush
;; missing -> __IO_FILE


;; pending:   _fflush_unlocked
;; missing -> __IO_FILE


;; pending:   _ffsl
;; missing -> _long


;; pending:   _ffsll
;; missing -> _long long


;; pending:   _fgetc
;; missing -> __IO_FILE


;; pending:   _fgetc_unlocked
;; missing -> __IO_FILE


;; pending:   _fgetpos
;; missing -> __G_fpos_t __IO_FILE


;; pending:   _fgets
;; missing -> __IO_FILE


;; pending:   _fileno
;; missing -> __IO_FILE


;; pending:   _fileno_unlocked
;; missing -> __IO_FILE


;; pending:   _flockfile
;; missing -> __IO_FILE


;; pending:   _fmemopen
;; missing -> __IO_FILE


;; pending:   _fopen
;; missing -> __IO_FILE


;; pending:   _fopencookie
;; missing -> __IO_FILE __IO_cookie_io_functions_t


;; pending:   _fpos_t
;; missing -> __G_fpos_t


;; pending:   _fprintf
;; missing -> __IO_FILE


;; pending:   _fputc
;; missing -> __IO_FILE


;; pending:   _fputc_unlocked
;; missing -> __IO_FILE


;; pending:   _fputs
;; missing -> __IO_FILE


;; pending:   _fread
;; missing -> __IO_FILE _unsigned long


;; pending:   _fread_unlocked
;; missing -> __IO_FILE


;; pending:   _freopen
;; missing -> __IO_FILE


;; pending:   _fscanf
;; missing -> __IO_FILE


;; pending:   _fseek
;; missing -> __IO_FILE _long


;; pending:   _fseeko
;; missing -> __IO_FILE ___off_t


;; pending:   _fsetpos
;; missing -> __G_fpos_t __IO_FILE


;; pending:   _ftell
;; missing -> __IO_FILE _long


;; pending:   _ftello
;; missing -> __IO_FILE ___off_t


;; pending:   _ftrylockfile
;; missing -> __IO_FILE


;; pending:   _funlockfile
;; missing -> __IO_FILE


;; pending:   _fwrite
;; missing -> __IO_FILE _unsigned long


;; pending:   _fwrite_unlocked
;; missing -> __IO_FILE


;; pending:   _gcvt
;; missing -> _double


;; pending:   _getc
;; missing -> __IO_FILE


;; pending:   _getc_unlocked
;; missing -> __IO_FILE


;; pending:   _getdelim
;; missing -> __IO_FILE ___ssize_t _char *


;; pending:   _getline
;; missing -> __IO_FILE ___ssize_t _char *


;; pending:   _getloadavg
;; missing -> _double


;; pending:   _getsubopt
;; missing -> _char *


;; pending:   _getw
;; missing -> __IO_FILE


;; pending:   _int16_t
;; missing -> ___int16_t


;; pending:   _int64_t
;; missing -> ___int64_t


;; pending:   _int8_t
;; missing -> ___int8_t


;; pending:   _int_fast16_t
;; missing -> _long


;; pending:   _int_fast32_t
;; missing -> _long


;; pending:   _int_fast64_t
;; missing -> _long


;; pending:   _int_fast8_t
;; missing -> _signed char


;; pending:   _int_least16_t
;; missing -> ___int_least16_t


;; pending:   _int_least64_t
;; missing -> ___int_least64_t


;; pending:   _int_least8_t
;; missing -> ___int_least8_t


;; pending:   _intmax_t
;; missing -> ___intmax_t


;; pending:   _intptr_t
;; missing -> _long


;; pending:   _jrand48
;; missing -> _long _unsigned short


;; pending:   _jrand48_r
;; missing -> _drand48_data _long _unsigned short


;; pending:   _l64a
;; missing -> _long


;; pending:   _labs
;; missing -> _long


;; pending:   _lcong48
;; missing -> _unsigned short


;; pending:   _lcong48_r
;; missing -> _drand48_data _unsigned short


;; pending:   _ldiv
;; missing -> _ldiv_t _long


;; pending:   _ldiv_t
;; missing -> _long


;; pending:   _llabs
;; missing -> _long long


;; pending:   _lldiv
;; missing -> _lldiv_t _long long


;; pending:   _lldiv_t
;; missing -> _long long


;; pending:   _locale_t
;; missing -> ___locale_struct


;; pending:   _lrand48
;; missing -> _long


;; pending:   _lrand48_r
;; missing -> _drand48_data _long


;; pending:   _malloc
;; missing -> _unsigned long


;; pending:   _max_align_t
;; missing -> _long double _long long


;; pending:   _memccpy
;; missing -> _unsigned long


;; pending:   _memchr
;; missing -> _unsigned long


;; pending:   _memcmp
;; missing -> _unsigned long


;; pending:   _memcpy
;; missing -> _unsigned long


;; pending:   _memmove
;; missing -> _unsigned long


;; pending:   _mempcpy
;; missing -> _unsigned long


;; pending:   _memset
;; missing -> _unsigned long


;; pending:   _mrand48
;; missing -> _long


;; pending:   _mrand48_r
;; missing -> _drand48_data _long


;; pending:   _nrand48
;; missing -> _long _unsigned short


;; pending:   _nrand48_r
;; missing -> _drand48_data _long _unsigned short


;; pending:   _open_memstream
;; missing -> __IO_FILE _char *


;; pending:   _pclose
;; missing -> __IO_FILE


;; pending:   _popen
;; missing -> __IO_FILE


;; pending:   _posix_memalign
;; missing -> _void *


;; pending:   _pselect
;; missing -> ___sigset_t _fd_set _timespec


;; pending:   _pthread_attr_t
;; missing -> _long


;; pending:   _pthread_barrier_t
;; missing -> _long


;; pending:   _pthread_cond_t
;; missing -> ___pthread_cond_s _long long


;; pending:   _pthread_mutex_t
;; missing -> ___pthread_mutex_s _long


;; pending:   _pthread_rwlock_t
;; missing -> ___pthread_rwlock_arch_t _long


;; pending:   _pthread_rwlockattr_t
;; missing -> _long


;; pending:   _pthread_spinlock_t
;; missing -> _volatile int


;; pending:   _pthread_t
;; missing -> _unsigned long


;; pending:   _ptrdiff_t
;; missing -> _long


;; pending:   _putc
;; missing -> __IO_FILE


;; pending:   _putc_unlocked
;; missing -> __IO_FILE


;; pending:   _putw
;; missing -> __IO_FILE


;; pending:   _qecvt
;; missing -> _long double


;; pending:   _qecvt_r
;; missing -> _long double


;; pending:   _qfcvt
;; missing -> _long double


;; pending:   _qfcvt_r
;; missing -> _long double


;; pending:   _qgcvt
;; missing -> _long double


;; pending:   _random
;; missing -> _long


;; pending:   _realloc
;; missing -> _unsigned long


;; pending:   _recorder_init
;; missing -> _trace


;; pending:   _rewind
;; missing -> __IO_FILE


;; pending:   _seed48
;; missing -> _unsigned short


;; pending:   _seed48_r
;; missing -> _drand48_data _unsigned short


;; pending:   _select
;; missing -> _fd_set _timeval


;; pending:   _setbuf
;; missing -> __IO_FILE


;; pending:   _setbuffer
;; missing -> __IO_FILE


;; pending:   _setlinebuf
;; missing -> __IO_FILE


;; pending:   _setvbuf
;; missing -> __IO_FILE


;; pending:   _sigset_t
;; missing -> ___sigset_t


;; pending:   _size_t
;; missing -> _unsigned long


;; pending:   _snprintf
;; missing -> _unsigned long


;; pending:   _srand48
;; missing -> _long


;; pending:   _srand48_r
;; missing -> _drand48_data _long


;; pending:   _stpncpy
;; missing -> _unsigned long


;; pending:   _strcasecmp_l
;; missing -> ___locale_struct


;; pending:   _strcoll_l
;; missing -> ___locale_struct


;; pending:   _strcspn
;; missing -> _unsigned long


;; pending:   _stream_close_f
;; missing -> _void (stream_t *)


;; pending:   _stream_read_f
;; missing -> _size_t (stream_t *, char *, size_t)


;; pending:   _stream_reset_f
;; missing -> _void (stream_t *)


;; pending:   _stream_write_f
;; missing -> _size_t (stream_t *, const char *, size_t)


;; pending:   _strerror_l
;; missing -> ___locale_struct


;; pending:   _strlen
;; missing -> _unsigned long


;; pending:   _strncasecmp
;; missing -> _unsigned long


;; pending:   _strncasecmp_l
;; missing -> ___locale_struct


;; pending:   _strncat
;; missing -> _unsigned long


;; pending:   _strncmp
;; missing -> _unsigned long


;; pending:   _strncpy
;; missing -> _unsigned long


;; pending:   _strndup
;; missing -> _unsigned long


;; pending:   _strsep
;; missing -> _char *


;; pending:   _strspn
;; missing -> _unsigned long


;; pending:   _strtod
;; missing -> _char * _double


;; pending:   _strtof
;; missing -> _char * _float


;; pending:   _strtok_r
;; missing -> _char *


;; pending:   _strtol
;; missing -> _char * _long


;; pending:   _strtold
;; missing -> _char * _long double


;; pending:   _strtoll
;; missing -> _char * _long long


;; pending:   _strtoq
;; missing -> _char * _long long


;; pending:   _strtoul
;; missing -> _char * _unsigned long


;; pending:   _strtoull
;; missing -> _char * _unsigned long long


;; pending:   _strtouq
;; missing -> _char * _unsigned long long


;; pending:   _strxfrm
;; missing -> _unsigned long


;; pending:   _strxfrm_l
;; missing -> ___locale_struct


;; pending:   _suseconds_t
;; missing -> ___suseconds_t


;; pending:   _time_t
;; missing -> ___time_t


;; pending:   _timespec
;; missing -> ___syscall_slong_t ___time_t


;; pending:   _timeval
;; missing -> ___suseconds_t ___time_t


;; pending:   _tmpfile
;; missing -> __IO_FILE


;; postponed
;; record _trace
;; incomplete definition
(define _trace _void)(define _trace-pointer _pointer)

;; function _recorder_init
(define _recorder_init
	(_fun (_or-null _trace-pointer) (_or-null _trace-pointer) -> _void))

;; function _trace_advance
(define _trace_advance
	(_fun (_or-null _trace-pointer) -> _void))

;; function _trace_append
(define _trace_append
	(_fun (_or-null _trace-pointer) (_or-null _record_s-pointer) -> _int32))

;; function _trace_append_safe
(define _trace_append_safe
	(_fun (_or-null _trace-pointer) (_or-null _record_s-pointer) -> _int32))

;; function _trace_clear
(define _trace_clear
	(_fun (_or-null _trace-pointer) -> _void))

;; function _trace_destroy
(define _trace_destroy
	(_fun (_or-null _trace-pointer) -> _void))

;; function _trace_flat_create
(define _trace_flat_create
	(_fun (_or-null _stream_s-pointer) -> (_or-null _trace-pointer)))

;; function _trace_forget
(define _trace_forget
	(_fun (_or-null _trace-pointer) -> _void))

;; function _trace_last
(define _trace_last
	(_fun (_or-null _trace-pointer) -> (_or-null _record_s-pointer)))

;; function _trace_load
(define _trace_load
	(_fun (_or-null _trace-pointer) -> _void))

;; function _trace_next
(define _trace_next
	(_fun (_or-null _trace-pointer) _record -> (_or-null _record_s-pointer)))

;; function _trace_save
(define _trace_save
	(_fun (_or-null _trace-pointer) -> _void))

;; function _trace_save_to
(define _trace_save_to
	(_fun (_or-null _trace-pointer) (_or-null _stream_s-pointer) -> _void))

;; function _trace_stream
(define _trace_stream
	(_fun (_or-null _trace-pointer) -> (_or-null _stream_s-pointer)))

;; typedef _trace_t
(define _trace_t _trace)

;; pending:   _uint16_t
;; missing -> ___uint16_t


;; pending:   _uint64_t
;; missing -> ___uint64_t


;; pending:   _uint8_t
;; missing -> ___uint8_t


;; pending:   _uint_fast16_t
;; missing -> _unsigned long


;; pending:   _uint_fast32_t
;; missing -> _unsigned long


;; pending:   _uint_fast64_t
;; missing -> _unsigned long


;; pending:   _uint_fast8_t
;; missing -> _unsigned char


;; pending:   _uint_least16_t
;; missing -> ___uint_least16_t


;; pending:   _uint_least64_t
;; missing -> ___uint_least64_t


;; pending:   _uint_least8_t
;; missing -> ___uint_least8_t


;; pending:   _uintmax_t
;; missing -> ___uintmax_t


;; pending:   _uintptr_t
;; missing -> _unsigned long


;; pending:   _ungetc
;; missing -> __IO_FILE


;; pending:   _va_list
;; missing -> ___va_list_tag


;; pending:   _vasprintf
;; missing -> ___va_list_tag _char *


;; pending:   _vdprintf
;; missing -> ___va_list_tag


;; pending:   _vfprintf
;; missing -> __IO_FILE ___va_list_tag


;; pending:   _vfscanf
;; missing -> __IO_FILE ___va_list_tag


;; pending:   _vprintf
;; missing -> ___va_list_tag


;; pending:   _vscanf
;; missing -> ___va_list_tag


;; pending:   _vsnprintf
;; missing -> ___va_list_tag _unsigned long


;; pending:   _vsprintf
;; missing -> ___va_list_tag


;; pending:   _vsscanf
;; missing -> ___va_list_tag


