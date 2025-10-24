# Events and their equivalence

## Notation:
- $`E`$: set of all events
- $`T`$: set of all threads
- $`Addr`$: set of all addresses

$`
e := \langle t, pc, cat, params, pred \rangle \\
t \in T \\
pc \in Addr \\
cat \in {R,W} \\
params \in List(*) \\
pred \subseteq E \\
`$

The concrete list of parameters depends on the category (which might eventually contain more than $`R/W`$). 
For now we assume `e.params[0]` is the address and `e.params[1]` the value being read or written.
The set of predecessors contains **ALL** the history of an event. The history contains those events in the same thread that come before in program order and events from different threads for which their effect became known to $`e`$ due to synchronization (there is synchronization if two events access same address and at least one of them is a write). 

The following captures the memory state after having executed events $`E`$.
The state contains the value of the last event writing to that memory address (i.e. any other event writting to that address is a predecessor) 

$`
memState : \mathcal{P}(E) \rightarrow Addr \rightarrow \N \\
memState(E, a) = v \Rightarrow \exists e \in E : e.cat = W \land e.params[0] = a \land e.params[1] = v \land \forall e' \in E \backslash \{e\} : e'.cat = W \land e'.params[0] = a \Rightarrow e' \in e.pred
`$

## Equivalence
Here are several notions of equivalence between events. Each notion is parametric in $`f`$. In the most strict case, $`e_1 = f(e_2) \text{ iff } e_1.pc = e_2.pc`$, i.e. they come from the same instruction. However, the use of $`f`$ allows us to define weaker notions. 

$`
e_1 \sim_1^f e_2 \text{ iff } e_1 = f(e_2) \land e_1.pred = e_2.pred
`$

This is probably to strong in practice and we will never use this notion.

$`
e_1 \sim_2^f e_2 \text{ iff } e_1 = f(e_2) \land \sum_{e \in e_1.pred} e.pc = \sum_{e' \in e_2.pred} e'.pc
`$

This hopefully captures that both events followed the same control flow.

$`
e_1 \sim_3^f e_2 \text{ iff } e_1 = f(e_2) \land memState(e_1.pred) = memState(e_2.pred)
`$

Two events are the same if they are executed from the same memory state.
