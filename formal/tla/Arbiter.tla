--------------------------- MODULE Arbiter ---------------------------
(* Safety model of one bus in modules/edge_core/src/arbiter.c and
   modules/edge_io/src/engines/master_engine.c. Line numbers are anchors only
   and drift when the product changes; function names and invariant names are
   the stable references:
     arbiter.c:181  _dispatch_next_locked   recovery-window guard
     arbiter.c:313  arbiter_on_io_done      engine-timeout settlement window
     arbiter.c:403  arbiter_check_timeouts  timer-scan settlement window
     master_engine.c:237 _master_run_command send, deadline reset, current_cmd

   Invariants: NoDoubleSettle, NoDispatchWhileOwner, NoDispatchInRecovery,
   LateResponseNotAssignedToNext. *)
EXTENDS Naturals, FiniteSets

CONSTANTS
    Cmds,          \* command ids
    Priorities,    \* effective priority values
    Timeout,       \* cmd->timeout_ms in ticks; same for every command
    LateBound,     \* late-response latency after a timeout settlement
    MaxQueue,      \* max_queue_len per bus
    MaxTime,       \* model time bound; keeps TLC finite
    RecoveryGate   \* TRUE = dispatch honours recovery_until_ns

\* The claim is bounded to responses that arrive inside the recovery window.
ASSUME LateBound < Timeout

VARIABLES
    queue,        \* pending commands; arbiter_submit_cmd keeps them sorted
    owner,        \* bus_slot_t.owner; 0 = free
    priority,     \* cmd->priority with aging folded in at submit (arbiter.c:44)
    deadline,     \* cmd->deadline_ns; 0 until submitted
    now,          \* uv_hrtime() in ticks
    recoveryUntil,\* bus_slot_t.recovery_until_ns; 0 = no window
    submitted,
    settleCount,  \* settlements per command; NoDoubleSettle bounds it
    timeoutAt,    \* settlement time when the command timed out; 0 = no
    lateArrived,  \* its late response has reached the wire
    assignedTo,   \* command whose engine current consumed it; 0 = dropped
    engineSent,   \* frames master_engine handed to write_async
    engineRead,
    engineCurrent,\* master_priv_t.current_cmd; 0 = none
    dispatchedWithoutOwner, \* history: last dispatch saw owner = 0
    dispatchedInRecovery    \* history: last dispatch was inside the window

vars == << queue, owner, priority, deadline, now, recoveryUntil,
          submitted, settleCount, timeoutAt, lateArrived, assignedTo,
          engineSent, engineRead, engineCurrent,
          dispatchedWithoutOwner, dispatchedInRecovery >>

Init ==
    /\ queue = {}
    /\ owner = 0
    /\ priority = [c \in Cmds |-> CHOOSE p \in Priorities : TRUE]
    /\ deadline = [c \in Cmds |-> 0]
    /\ now = 0
    /\ recoveryUntil = 0
    /\ submitted = {}
    /\ settleCount = [c \in Cmds |-> 0]
    /\ timeoutAt = [c \in Cmds |-> 0]
    /\ lateArrived = [c \in Cmds |-> FALSE]
    /\ assignedTo = [c \in Cmds |-> 0]
    /\ engineSent = {}
    /\ engineRead = {}
    /\ engineCurrent = 0
    /\ dispatchedWithoutOwner = TRUE
    /\ dispatchedInRecovery = FALSE

\* arbiter_submit_cmd (arbiter.c:234): admission, deadline, priority order.
Submit(c, p) ==
    /\ c \in Cmds \ submitted
    /\ Cardinality(queue) < MaxQueue
    /\ submitted' = submitted \cup {c}
    /\ queue' = queue \cup {c}
    /\ priority' = [priority EXCEPT ![c] = p]
    /\ deadline' = [deadline EXCEPT ![c] = now + Timeout]
    /\ UNCHANGED << owner, now, recoveryUntil, settleCount, timeoutAt,
                   lateArrived, assignedTo, engineSent, engineRead,
                   engineCurrent, dispatchedWithoutOwner, dispatchedInRecovery >>

\* Queue head: highest effective priority, then earliest deadline
\* (arbiter.c:288); the id tie-break is model-only.
IsNext(c) ==
    /\ c \in queue
    /\ \A d \in queue :
         \/ priority[c] > priority[d]
         \/ /\ priority[c] = priority[d]
            /\ \/ deadline[c] < deadline[d]
               \/ /\ deadline[c] = deadline[d]
                  /\ c <= d

RecoveryReady ==
    ~RecoveryGate \/ recoveryUntil = 0 \/ now >= recoveryUntil

\* _dispatch_next_locked (arbiter.c:170) plus _master_run_command send
\* (master_engine.c:237): owner, queue pop, deadline reset, current_cmd.
Dispatch ==
    \E c \in queue :
        /\ IsNext(c)
        /\ owner = 0
        /\ RecoveryReady
        /\ owner' = c
        /\ queue' = queue \ {c}
        /\ engineCurrent' = c
        /\ engineSent' = engineSent \cup {c}
        /\ deadline' = [deadline EXCEPT ![c] = now + Timeout]
        /\ recoveryUntil' = IF recoveryUntil # 0 /\ now >= recoveryUntil
                            THEN 0 ELSE recoveryUntil
        /\ dispatchedWithoutOwner' = (owner = 0)
        /\ dispatchedInRecovery' = (recoveryUntil # 0 /\ now < recoveryUntil)
        /\ UNCHANGED << priority, now, submitted, settleCount, timeoutAt,
                       lateArrived, assignedTo, engineRead >>

Tick ==
    /\ now < MaxTime
    /\ now' = now + 1
    /\ UNCHANGED << queue, owner, priority, deadline, recoveryUntil,
                   submitted, settleCount, timeoutAt, lateArrived, assignedTo,
                   engineSent, engineRead, engineCurrent,
                   dispatchedWithoutOwner, dispatchedInRecovery >>

\* arbiter_check_timeouts (arbiter.c:377): owner cleared, window written
\* before _dispatch_next_locked, settlement callback runs after the lock.
ArbiterTimeout(c) ==
    /\ c \in Cmds
    /\ owner = c
    /\ deadline[c] # 0
    /\ now >= deadline[c]
    /\ owner' = 0
    /\ engineCurrent' = 0
    /\ timeoutAt' = [timeoutAt EXCEPT ![c] = now]
    /\ recoveryUntil' = now + Timeout
    /\ settleCount' = [settleCount EXCEPT ![c] = settleCount[c] + 1]
    /\ UNCHANGED << queue, priority, deadline, now, submitted, lateArrived,
                   assignedTo, engineSent, engineRead,
                   dispatchedWithoutOwner, dispatchedInRecovery >>

\* _master_on_timeout (master_engine.c:65): the engine timer path settles
\* through arbiter_on_io_done (arbiter.c:313, CMD_STATUS_TIMEOUT).
EngineTimeout(c) ==
    /\ c \in Cmds
    /\ owner = c
    /\ deadline[c] # 0
    /\ now >= deadline[c]
    /\ owner' = 0
    /\ engineCurrent' = 0
    /\ timeoutAt' = [timeoutAt EXCEPT ![c] = now]
    /\ recoveryUntil' = now + Timeout
    /\ settleCount' = [settleCount EXCEPT ![c] = settleCount[c] + 1]
    /\ UNCHANGED << queue, priority, deadline, now, submitted, lateArrived,
                   assignedTo, engineSent, engineRead,
                   dispatchedWithoutOwner, dispatchedInRecovery >>

\* _process_req_result (master_engine.c:101) success: on_finish then
\* arbiter_on_io_done(SUCCESS). Ownership is cleared before settlement.
ResponseArrives(c) ==
    /\ c \in Cmds
    /\ owner = c
    /\ engineCurrent = c
    /\ deadline[c] # 0
    /\ now < deadline[c]
    /\ owner' = 0
    /\ engineCurrent' = 0
    /\ engineRead' = engineRead \cup {c}
    /\ settleCount' = [settleCount EXCEPT ![c] = settleCount[c] + 1]
    /\ UNCHANGED << queue, priority, deadline, now, recoveryUntil,
                   submitted, timeoutAt, lateArrived, assignedTo, engineSent,
                   dispatchedWithoutOwner, dispatchedInRecovery >>

\* _process_req_result error paths: transport/protocol failure settles with
\* no timeout window, same ownership-clear order.
EngineError(c) ==
    /\ c \in Cmds
    /\ owner = c
    /\ engineCurrent = c
    /\ owner' = 0
    /\ engineCurrent' = 0
    /\ settleCount' = [settleCount EXCEPT ![c] = settleCount[c] + 1]
    /\ UNCHANGED << queue, priority, deadline, now, recoveryUntil,
                   submitted, timeoutAt, lateArrived, assignedTo, engineSent,
                   engineRead, dispatchedWithoutOwner, dispatchedInRecovery >>

\* A response from a timed-out transaction reaches RX. If the engine still
\* has a current command, _master_io_cb (:180) decodes the bytes as that
\* command's response; otherwise run_command's DISCARD_RX (:218) drops them.
LateResponseArrives(c) ==
    /\ c \in Cmds
    /\ timeoutAt[c] # 0
    /\ ~lateArrived[c]
    /\ now <= timeoutAt[c] + LateBound
    /\ lateArrived' = [lateArrived EXCEPT ![c] = TRUE]
    /\ assignedTo' = [assignedTo EXCEPT ![c] =
                         IF engineCurrent = 0 THEN 0 ELSE engineCurrent]
    /\ engineRead' = IF engineCurrent = 0 THEN engineRead
                     ELSE engineRead \cup {engineCurrent}
    /\ UNCHANGED << queue, owner, priority, deadline, now, recoveryUntil,
                   submitted, settleCount, timeoutAt, engineSent,
                   engineCurrent, dispatchedWithoutOwner, dispatchedInRecovery >>

\* Safety only: no termination or fairness claim, so stuttering is allowed.
Stutter ==
    UNCHANGED vars

Next ==
    \/ \E c \in Cmds, p \in Priorities : Submit(c, p)
    \/ Dispatch
    \/ \E c \in Cmds : ArbiterTimeout(c)
    \/ \E c \in Cmds : EngineTimeout(c)
    \/ \E c \in Cmds : ResponseArrives(c)
    \/ \E c \in Cmds : EngineError(c)
    \/ \E c \in Cmds : LateResponseArrives(c)
    \/ Tick
    \/ Stutter

Spec == Init /\ [][Next]_vars

TypeOK ==
    /\ queue \subseteq Cmds
    /\ owner \in Cmds \cup {0}
    /\ priority \in [Cmds -> Priorities]
    /\ deadline \in [Cmds -> 0..MaxTime + Timeout]
    /\ now \in 0..MaxTime
    /\ recoveryUntil \in 0..MaxTime + Timeout
    /\ submitted \subseteq Cmds
    /\ settleCount \in [Cmds -> 0..2]
    /\ timeoutAt \in [Cmds -> 0..MaxTime]
    /\ lateArrived \in [Cmds -> BOOLEAN]
    /\ assignedTo \in [Cmds -> Cmds \cup {0}]
    /\ engineSent \subseteq Cmds
    /\ engineRead \subseteq Cmds
    /\ engineCurrent \in Cmds \cup {0}
    /\ dispatchedWithoutOwner \in BOOLEAN
    /\ dispatchedInRecovery \in BOOLEAN

NoDoubleSettle ==
    \A c \in Cmds : settleCount[c] <= 1

NoDispatchWhileOwner ==
    dispatchedWithoutOwner

NoDispatchInRecovery ==
    ~dispatchedInRecovery

\* A late response may be dropped or consumed by its own transaction; it must
\* never be consumed by the command dispatched after it.
LateResponseNotAssignedToNext ==
    \A c \in Cmds :
        (timeoutAt[c] # 0 /\ lateArrived[c])
            => (assignedTo[c] = 0 \/ assignedTo[c] = c)

=======================================================================
