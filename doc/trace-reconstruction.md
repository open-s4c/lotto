# Trace reconstruction algorithm

## Inputs:
- `r_MAX :: int` - number of times we call `lotto-run` before considering that the next event cannot be reached with the current sampling configuration.
- `S :: [int]` - sequence of sampling configurations (for more abstract to more fine-grained) to perform change-point refinement before backtracking.

## Variables
- `i :: int` - Current event index [e_i]
- `r :: int` - Current number of runs [< r_MAX]
- `k :: int` - Current sampling configuration [S_k]

## Initial Values
- `i = 0`
- `r = 0` 
- `k = 0`

## Return Values
- `PASS` - If all events of the trace have been matched
- `FAIL` - If we failed to completely reproduce the trace

## Algorithm
1. We have matched `i` events of the trace, we are currently on event `e_i`.
2. If all events in the trace have been matched - **return `PASS`**. 
3. If the stack is empty (`i == -1`) - **return `FAIL`**. Otherwise, peek the next `r` and `k` from the top of their stacks.
4. Call `lotto-run` with the current sampling configuration `S_k`.
5. If we found the next event - increment `i`, push a fresh `0` onto the stacks and **GOTO 1**.
6. If we haven't yet reached the max number of iterations with the current sampling configuration - increment `r` and **GOTO 4**.
7. If we haven't yet reached the last sampling configuration - take the next sampling configuration (increment `k`), reset the current `r` to `0`, and **GOTO 4**. Otherwise, backtrack to the previous event (`i--`), pop the current `r` and `k` values from the stacks and **GOTO 1**.

## Assumptions
- We have a faithful way to check "found e_i+1?". If this is not the case, we fail to reproduce the full trace even if `r_MAX` and the sampling sequence `S` are good enough.
- `lotto-run -s X` enables the filtering handler with sampling ratio / probability equals to `X`.

## Flowchart

```mermaid
flowchart TD
    entry -- i=0 \n push 0 --> replay[1: lotto-replay \n e_0 ... e_i];
    replay --> finish{2: i == e_size?};
    finish -- // yes --> PASS;
    finish -- // no --> empty{3: i == -1?};
    empty -- // yes --> FAIL;
    empty -- // no \n k = peek \n r = peek --> run[4: lotto-run -s S_k];
    run --> found{5: found e_i+1?}
    found -- // no --> rmax{6: r == r_MAX?};
    found -- // yes \n i++ \n push 0 --> replay;
    rmax -- // no \n r++ --> run;
    rmax -- // yes --> smax{7: k == S_size};
    smax -- // no \n k++ \n r=0 --> run;
    smax -- // yes \n i-- \n pop --> replay;
```
