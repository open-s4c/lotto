# Finding minimal sufficient set of ordering constraints
## Notation
- $`E`$ &mdash; set of all events
- $`e_c^t\in E`$ &mdash; event happened in thread $`t`$ at clock $`c`$
- $`clock(e_c^t)=c`$ &mdash; clock of the event $`e_c^t`$
- $`T`$ &mdash; input trace
- $`e \in T`$ &mdash; trace $`T`$ contains event $`e`$
- $`\Omega(T)=\{(e_{c_1}^{t_1} \rightarrow e_{c_2}^{t_2})\ |\ e_{c_1}^{t_1}, e_{c_2}^{t_2} \in T \land t_1 \ne t_2 \land c_1 < c_2\}`$ &mdash; total order of $`T`$
- $`\tilde{e}\in E`$ &mdash; event equivalent to $`e`$
- $`T \vdash O \equiv \exists f \in E\rightarrow E: \forall (e_1 \rightarrow e_2)\in O. f(e_1)= \tilde{e_1}\in T \land f(e_2)=\tilde{e_2}\in T \land clock(\tilde{e_1})< clock(\tilde{e_2})`$ &mdash; trace $`T`$ satisfies ordering constraint set $`O`$
- $`after(e, E)=\{(e^\prime, e)\ |\ e^\prime\in E\}`$ &mdash; orders that would be added if $`e`$ was added after every event in $`E`$
## Algorithm
```
Input: T, r
Output: locally smallest O s.t. if trace |- O => trace is buggy

O <- Ω(T)
E <- T
for (e1, e2) in O:
	for i in 1..r:
		trace <- run()
		for e in trace\E:
			O <- O \/ after(e, T)
			E <- E \/ {e}
		if not (trace |- O\{e1 -> e2}) or trace |- {e1 -> e2}
			continue
		if trace is not buggy
			goto l
	O <- O\{e1 -> e2}
	l:
```
The idea is to test whether the invariant $`T \vdash O \implies T \text{ is buggy}`$ is violated after removing some ordering constraint.
## Notes
- `run()` can be done in a way it maximizes the likelihood of `trace |- O\{e1 -> e2} and not trace |- {e1 -> e2}`
- Finding a $`\tilde{e}`$ can be temporarily reduced to comparing tuples $`\langle thread, pc, count\rangle`$
- The algorithm also removes (implicitly) transitive ordering constraints. If we want to keep them, we should find at least one buggy trace refuting the constraint before removing it from the set.
- "Locally smallest" should be read as "no element can be removed without violating the invariant".
- Since the absence of some events in the trace may be important for understanding the bug, we need to consider $`after(e, T)`$ ordering constraint for events not appearing in the input trace.
