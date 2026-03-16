UNSUPPORTED: aarch64
RUN: (! %lotto %stress -- no_such_file 2>&1) | %check %s
CHECK: [lotto] round: 0/inf, pos
CHECK: error creating child: No such file or directory
