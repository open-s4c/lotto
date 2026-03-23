# Demos

## leveldb_test

A concurrent LevelDB test that exercises Lotto with a real-world key-value
store workload.

### Build

```bash
cd demos/leveldb_test
make
```

### Run

```bash
lotto stress demos/leveldb_test/leveldb_test
```

## linux (qLotto) *(experimental)*

A Linux kernel demo using qLotto and the `sched_ext` scheduling extension.
This demo runs an arm64 Linux VM under QEMU with Lotto controlling VCPU
scheduling. See [`doc/demos/qlotto-linux.md`](demos/qlotto-linux.md) for
detailed setup and run instructions.

[Go back to main](../README.md)
