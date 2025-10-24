# Demos

Working:

- shmec
- unbounded_queue
- skiplist
- buf_ring

Not working yet:
- safe stack
- ebr

## Build demos

    git submodule update --init
    cd demos
    make -C skiplist  ## ignore warnings
    make -C shmec
    make -C unbounded_queue
    make -C buf_ring

## Run demos

Something like this should be enough:

    cd demos
    lotto stress -d 5 skiplist/bug
    lotto stress -d 10 unbounded_queue/bug
    lotto stress -d 3 shmec/bug
    lotto stress -d 3 buf_ring/bug

For details see the README.md in each of the demo directories.

[Go back to main](../README.md)
