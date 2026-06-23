# REQUIRES: rust-nightly
#
# RUN: rm -rf %T/tokio_lotto_replay
# RUN: cp -R %S/tokio_lotto_replay %T/tokio_lotto_replay
# RUN: bash %S/build_tsan.sh %T/tokio_lotto_replay
# RUN: bash -c 'BIN=$(cat %T/tokio_lotto_replay/binary.path); (! %lotto %stress -r 200 -s random -- "$BIN" 2>&1) | %check %s --check-prefix=STRESS'
# RUN: bash -c '(! %lotto %replay 2>&1) | %check %s --check-prefix=REPLAY'
# RUN: %lotto %show | %check %s --check-prefix=TRACE
#
# STRESS: tokio replay bug: observed stale value
# REPLAY: tokio replay bug: observed stale value
# TRACE: reason:   SIGABRT
