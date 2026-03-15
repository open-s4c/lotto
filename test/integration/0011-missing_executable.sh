UNSUPPORTED: aarch64
RUN: (! %lotto %stress -- no_such_file 2>&1) | filecheck %s
CHECK: [lotto] round: 0/inf, pos
CHECK-NEXT: error creating child: No such file or directory
