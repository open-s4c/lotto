add_driver_module(init_cli.c)
target_link_libraries(${DRIVER_MODULE_TARGET} PRIVATE rusty_cli)
