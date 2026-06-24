void lotto_runtime_start_phases(void);

static void __attribute__((constructor))
lotto_runtime_loader_start_(void)
{
    lotto_runtime_start_phases();
}
