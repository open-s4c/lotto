# Finding minimal sufficient set of ordering constraints

## Algorithm
```
Input: T, r
Output: A set of OC - O that is sufficient and necessary for some bug $b_j$ to appear

rec_inflex(T, r) {
    O <- {}
    C <- {}
    while(!always_fails(T, r, C)) {
        
        c <- f(T, r, C)
        
        if (always_succeeds(T, r, C U !c)) {
            O <- O U c;
            C <- C U c;
        } else {
            C <- C U !c;
        }
    }

    return O;
}
```
The idea here is to use an oracle `f(T, r, O)` that returns the latest (based on source) OC of T, that must be satisfied for the bug to appear, given that all OC-s in O are satisfied. After the OC is found, we add it to the set O. In the next iteration, we constrain all checks of O to only theones that consider all OC-s in O.

## Oracle - Siblings
```
Input: T, r, O
Output: Latest OC - c that in some buggy trace that also satisfies all OC in O 

f(T, r, O) {
    $T_f$ <- T
    $T_s$ <- null

    while (true) {
        s <- find earliest point in $T_f$ after which the failure is guaranteed (IP)
        if (s == 0) return null;

        for (t_id : schedualable_threads) {
            t_temp <- next_event_on_thread(t_id, IP - 1)
            if (always_succeeds(t_temp)) {
                // identified IP, IIP Siblings
                return {s -> t_temp}
            }
        }

        $T_s$ <- find successful run following $T_f$ until IP - 1
        $t$ <- find earliest point in $T_s$ after which the success is guaranteed (IIP)

        $T_f$ <- find failing run following $T_s$ until IIP - 1

        for (t_id : schedualable_threads) {
            s_temp <- next_event_on_thread(t_id, IIP - 1)
            if (always_fails(s_temp)) {
                // identified IP, IIP Siblings
                return {s_temp -> t}
            }
        }
    }
    return {s -> t}; 
}
```
## Siblings - Proof
Definition: Given a trace prefix $`T`$ and an OC $`o = (a, b) \in OC`$ we say that $`o`$ is in either of the three states:
- satisfied if $`a \in T \land (b \in T \implies clock(a) < clock(b))`$, i.e. $`o\in S(T)`$,
- violated if $`b \in T \land (a \in T \implies clock(b) < clock(a))`$, i.e. $`o\in V(T)`$,
- unresolved if $`a \not\in T \land b \not\in T`$, i.e. $`o\in U(T)`$.

Lemma (D): $`S(T)`$, $`V(T)`$, and $`U(T)`$ are pairwise disjoint and complete w.r.t. $`OC`$.

Lemma (S): $`\forall x, T.\ \forall o_s \in S(Tx) \cap U(T).\ source(o_s)=x`$.

Lemma (V): $`\forall x, T.\ \forall o_v \in V(Tx) \cap U(T).\ target(o_v)=x`$.

Notation: We use subscripts to denote bug related OC sets, i.e., $`OC_B`$, $`S_B(T)`$, $`V_B(T)`$, and $`U_B(T)`$ for all bugs $`B`$ present in the program and $`OC_b`$, $`S_b(T)`$, $`V_b(T)`$, and $`U_b(T)`$ for some bug $`b \in B`$.

Lemma (IP): $`\forall IP.\ \exists b \in b.\ OC_b.\ OC_b=S_b(T \cdot IP)`$.

Lemma (IIP): $`\forall IIP.\ \forall b \in b.\ OC_b.\ V_b(T \cdot IIP) \neq \empty`$.

Statement: If some IP and IIP are siblings, then (IP -> IIP) is an OC for some bug.
- From Lemma (IP): $`\exists b \in b.\ OC_b=S_b(T \cdot IP)`$.
- From Lemma (D): $`U_b(T\cdot IP) = V_b(T\cdot IP) = \empty`$.
- From Lemma (S): $`\forall o \in U_b(T).\ source(o)=IP`$. (1)
- From Lemma (IIP): $`\exists o_b \in V(T \cdot IIP)`$.
- From Lemma (V): $`target(o_b)=IIP`$.
- From (1): $`o_b = (IP, IIP)`$.

## Oracle - Iterate and Check
```
Input: T, r, O
Output: Latest OC - c that in some buggy trace that also satisfies all OC in O 

f(T, r, O) {
    $T_f$ <- T
    $T_s$ <- null

    while (true) {
        s <- find earliest point in $T_f$ after which the failure is guaranteed (IP)
        if (s == 0) return null;

        $T_s$ <- find successful run following $T_f$ until IP - 1
        $t$ <- find earliest point in $T_s$ after which the success is guaranteed (IIP)

        $T_f$ <- find failing run following $T_s$ until IIP - 1

        if (always_fails({s -> t}, IP - 1) && compatible(s, t)) break;
    }
    return {s -> t}; 
}
```

## Iterate and Check - Proof  
IP - source of OC $`c^*`$ = (IP -> $`e^*`$)  
IIP - source of OC $`c^~`$ = ($`e^~`$ -> IIP)  
Statement:   
If at IP - 1 all the runs satisfying (IP -> IIP) fail,  
then $`c^*`$ = $`c^~`$  

Taking the contrapositive of this statement we have:  
If $`c^*`$ != $`c^~`$,  
then at IP - 1 all the runs satisfying (IP -> IIP) will either pass or fail  

This statement is true by the definition of IP - at the point IP - 1, when uncostrained aditionally some runs will fail and some will pass. Since $`c^*`$ != $`c^~`$, constraining the execution on (IP -> IIP) will have no result, thus all runs from IP - 1 will either fail or pass.


## Note
The word 'guaranteed' is used as guaranteed withing some probability. We consider r potential traces that satisfy the conditions of O. If all r traces are failing/succeeding we say that we have a guarantee.
