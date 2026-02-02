add_runtime_module(init_engine.c intercept_await.c intercept_spin_loop.c)
target_link_libraries(${RUNTIME_MODULE_TARGET} PRIVATE rusty_engine)
