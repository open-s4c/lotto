# Replay enforcement

Lotto replays the schedule and data recorded in its trace. While it can be sufficient for determinism in some scenarios,
others still cannot be replayed reliably. When a replay diverges, it may lead to a deadlock, different execution output,
or a replay integrity check failure in Lotto. The replayer exploits the redundant information from the trace to detect if
the replay is failing to reproduce the recorded execution. However, this check may often be insufficient and report the problem
too late. Therefore, Lotto has a replay enforcement feature, which stores even more information in the trace to allow to
recognize replay divergence as early as possible.

Replay enforcement supports multiple modes. Their combination can be provided through the `--enforcement-mode` flag as
a vertical bar separated list. Each mode adds a specific datum to all records and verifies whether it matches on replay.
The available modes are listed in the table below:

| Mode         | Meaning                                 |
| ------------ | --------------------------------------- |
| PC           | program counter                         |
| CAT          | event category                          |
| TID          | task ID                                 |
| ADDRESS      | read/written address                    |
| DATA         | read/written data                       |
| CUSTOM       | data supplied by special enforce events |

Note that the addressed may differ on replay due to ASLR. Thus, the affected modes (PC, ADDRESS, DATA, CUSTOM) should be complemented
with a suitable stable address method (`--stable-address-method`). Moreover, enforcement modes do not affect how dense the recording
is going to be. A finer record granularity (`--record-granularity`) can help the replay enforcer report the replay divergence earlier.

[Go back to main](../README.md)
