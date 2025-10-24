# Racket tests

Lotto has a set of test cases written in Racket. To run these tests you have to install racket and a few packages and configure Lotto to run the Racket tests. The tests are located in `tests/racket`.

## Installing Racket

The best option is to install one of the latest releases with the install script.
Here is the link for version 8.10:

    https://download.racket-lang.org/installers/8.10/racket-8.10-x86_64-linux-cs.sh

After that be sure that `racket` and `raco` commands are in your path.

In our tests, we employ three packages that also must be installed.  If you are not behind a proxy, the following command should be sufficient to install these packages:

    raco pkg install quickcheck fmt unix-signals

Otherwise, you'll have to install the packages manually.

The `fmt` package is used to format the Racket code.

    curl -LO https://github.com/sorawee/pretty-expressive/archive/refs/heads/main.tar.gz
    raco pkg install -D -n pretty-expressive main.tar.gz

    curl -LO https://github.com/sorawee/fmt/archive/refs/heads/master.tar.gz
    raco pkg install -D -n fmt master.tar.gz

The `unix-signals` package is used to capture `SIGABRT` in the test cases instead of aborting the test process.

    curl -LO https://github.com/tonyg/racket-unix-signals/archive/refs/heads/master.tar.gz
    raco pkg install -D -n unix-signals master.tar.gz

The `quickcheck` package is used in some tests cases to  explore larger set of test scenarios in a compact way.

    curl -LO https://github.com/jackfirth/doc-coverage/archive/refs/heads/master.tar.gz
    raco pkg install -D -n doc-coverage master.tar.gz

    curl -LO https://github.com/ifigueroap/racket-quickcheck/archive/refs/heads/master.tar.gz
    raco pkg install -D -n quickcheck master.tar.gz

Note that you can ignore the errors reported while installing the following two packages. These errors are related to building the
documentation generation, which we do not need.

## Configuring and running Racket tests

The `tests` target runs the Racket tests in addition to the normal unit tests when Lotto is configured with `LOTTO_RACKET_TESTS=on`:

    cd lotto/build
    cmake .. -DLOTTO_RACKET_TESTS=on
    make
    make test

The racket test can also be used when measure the test coverage:

    cd build
    cmake .. -DLOTTO_RACKET_TESTS=on -DLOTTO_TEST_COVERAGE=on
    make
    make test
    make coverage

Note that for test coverage you also need to install `gcovr` (eg, `sudo apt install gcovr`).


## Interface bindings and mock components

To generate or update the Racket interface bindings, we use a tool called `mockoto`.

TBD: installation instructions.
