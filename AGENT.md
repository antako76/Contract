# AGENT.md

This repository is a clean rewrite of the CONTRACT layer. The main failure mode
to avoid is implementing a solution before its contract and boundaries are
clear.

## Terms

- **T-1 Contract**: the guarantees, inputs, outputs, constraints, and failure modes
  of a component.
- **T-2 Boundary**: the division of responsibility between layers or modules.
- **T-3 Public API**: the supported interface exposed to users of the library.
- **T-4 Internal API**: an implementation interface contained within its owning
  module and free to change with that module.
- **T-5 State**: data with a lifetime and owner. Configuration and computed
  runtime/session state are different kinds of state.
- **T-6 Local change**: an implementation change that preserves contracts,
  boundaries, and public APIs.
- **T-7 Architectural change**: a change to a contract, boundary, public API,
  extension mechanism, state ownership, or module responsibility.

## Working Model

Work is a joint engineering loop. Use the user's feedback as design input and
choose tradeoffs together. Optimize for reaching the correct shared result, not
for replying immediately, writing code immediately, or producing locally clever
code.

The fastest and most effective path is usually to discuss the actual problem
first, then implement the smallest correct solution. Do not burn tokens and
time generating a large amount of plausible but wrong code when the missing
constraint or boundary can be clarified directly.

Before implementing non-trivial behavior, explicitly state the implementation
problem you are solving when any of these are true:

- the current API does not obviously support the requested behavior;
- the change adds a helper, wrapper, fallback, sentinel, compatibility path, or
  parallel path;
- the write/read path needs a different payload shape than the current codec
  exposes;
- the requested behavior can be implemented in more than one defensible way.

If the problem is not clear, stop and surface it to the user before coding.
Do not smuggle an unresolved implementation problem into code.

Use this order:

1. `WM-1` Observe the current code and state the gap or conflict.
2. `WM-2` Classify the work as a local or architectural change.
3. `WM-3` For an architectural change, present the simplest clean options and wait for
   explicit agreement.
4. `WM-4` Implement the agreed contract directly.
5. `WM-5` Verify behavior and confirm that the resulting API remains clear.

If something blocks effective work, stop and discuss it with the user. Do not
silently work around a blocker that the user can help resolve.

## Rule Triggers

- `RT-1` At the start of every task, apply `WM-1` and `WM-2`: inspect the
  affected code and classify the task as `T-6` or `T-7`.
- `RT-2` Before an architectural change, apply `WM-3`, `AR-*`, and `API-*`.
  State the gap, review boundaries and APIs, present the clean options, and wait
  for explicit agreement.
- `RT-3` When a feature introduces or changes state, apply `ST-*` before
  implementation.
- `RT-4` Before adding a helper, wrapper, fallback, sentinel, compatibility
  path, parallel API, or new abstraction, apply `AR-3` through `AR-8` and
  `API-2` through `API-3`. If one concept starts spreading across multiple
  APIs, traits, wrappers, or synchronized representations, stop and re-evaluate
  the underlying contract before adding more code.
- `RT-5` Before editing, apply `CH-*`: confirm the requested scope and preserve
  readable direct code unless the agreed change requires otherwise.
- `RT-6` After implementation, apply `WM-5` and `VR-*`.
- `RT-7` Before staging or committing, apply `CH-5`.

For a local change, the minimum path is:

```text
WM-1 -> WM-2 (T-6) -> CH-3 -> implement -> WM-5 / VR-*
```

For an architectural change, the minimum path is:

```text
WM-1 -> WM-2 (T-7) -> WM-3 / AR-* / API-* / ST-* as needed
     -> explicit agreement -> implement -> WM-5 / VR-*
```

## Architecture Rules

- `AR-1` A layer or module must have one clear responsibility.
- `AR-2` A responsibility belongs to the layer whose contract defines its guarantee.
  Declaration and execution may belong to different layers, but that boundary
  must be explicit.
- `AR-3` Before assigning a responsibility, identify what the dependencies
  already guarantee and what guarantee is still missing.
- `AR-4` If responsibilities overlap or a boundary is unclear, treat that as a design
  problem and resolve it before coding.
- `AR-5` State a type, signature, naming, ownership, or layer conflict directly before
  proposing a workaround.
- `AR-6` Do not hide an unclear contract behind a helper, wrapper, fallback, sentinel,
  compatibility path, or parallel API.
- `AR-7` Add an abstraction only when it represents a real responsibility, removes
  meaningful complexity, or follows an established project pattern.
- `AR-8` Prefer one clear entry point for one responsibility.
- `AR-9` Nested work follows the owning layer's normal dispatch unless the contract
  explicitly defines another path.
- `AR-10` Keep failure ownership and user-facing error formatting in the layer
  that owns the behavior. Leaf helpers may signal failure, but they should not
  take over the final contract unless they own the whole failure path.
- `AR-11` Do not introduce one-off forwarding helpers or nested functions that
  only shuttle state between existing methods and mostly make the code harder
  to follow.

## API Rules

- `API-1` Keep the public surface small and intentional.
- `API-2` Classify every new API as public or internal before implementing it.
- `API-3` Prefer internal APIs for implementation details local to a module. A
  helper used by one file stays local or internal to that file.
- `API-4` Do not add a public API while the current public contract can express the
  requirement clearly.
- `API-5` Before adding a core/public customization point, trait, metadata API, or
  extension mechanism, describe the gap and contract options to the user and
  wait for explicit agreement.
- `API-6` Do not preserve names or compatibility paths that make the contract
  misleading unless compatibility is an explicit requirement.

## State Rules

- `ST-1` Decide whether the feature needs runtime state before designing state.
- `ST-2` Define the owner and lifetime of state before implementing a stateful feature.
- `ST-3` Keep configuration separate from computed runtime/session state.
- `ST-4` The outer run creates session state; nested traversal reuses it.
- `ST-5` If code must guess whether it is root/child or outer/inner, the session
  contract is incomplete and must be clarified.

## Change Rules

- `CH-1` Local changes may be implemented directly after reading the affected code.
- `CH-2` Architectural changes require discussion and explicit agreement before code.
- `CH-3` Keep changes scoped to the requested responsibility.
- `CH-4` Do not refactor readable direct code merely to remove a few repeated lines.
- `CH-5` Do not stage or commit changes unless the user explicitly requests it.

## Project Sources Of Truth

The stable adapter-facing public contracts live in
[`docs/contract/adapters/README.md`](docs/contract/adapters/README.md).

Before changing adapter contracts, entry points, configuration views, session
state, or facade boundaries, read that document and keep it synchronized with
the implementation.

## Verification

After a non-trivial change:

- `VR-1` Build the project.
- `VR-2` Run relevant tests for a local change; run the full suite for shared,
  core, or architectural changes.
- `VR-3` Run an affected example or benchmark when behavior or performance changed.
- `VR-4` Inspect the resulting public and internal APIs for clarity.

Compilation is necessary, but it does not validate an unclear design.
