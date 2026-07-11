# CONTRACT Attributes

This directory is the entry point for the attribute system.

It explains how attributes flow from a contract declaration to an adapter
guarantee, and then points to the files that define each layer in detail.

This directory intentionally splits the model into one canonical attribute
specification and one adapter-rules spec. Keep that split:

- `attributes.md` defines the attribute model itself (implemented surface).
- `vision.md` collects the designed-but-not-yet-implemented parts of the model
  (projection vocabularies, fingerprints, validation modes, debug/audit,
  future vocabularies).
- `adapter_scopes.md` defines how adapter families turn visibility into
  `attribute_mode` decisions.
- `security.md` defines the security vocabulary and behavior examples.
- `adapters.md` explains how adapter authors should structure their code.

The common vocabularies are implemented as dedicated headers under
[`include/contract/attributes/vocabularies/`](../../include/contract/attributes/vocabularies/).
The canonical sources for `contract::schema::*`, `contract::check::*`,
`contract::unit::*`, `contract::doc::*`, and `contract::security::*`
declarations live there alongside their `attr_traits` specializations.

## Attribute Processing Pipeline

```text
attribute
  -> vocabulary
  -> optional vocabulary-local tags
  -> adapter visibility
  -> adapter type
  -> attribute rules
  -> attribute mode
  -> guarantee surface
```

Read the pipeline left to right:

- `attribute` is the declared payload in a `CONTRACT(...)` entry.
- `vocabulary` owns the meaning of that payload.
- `tags` are optional vocabulary-local selectors that may help adapters reduce
  explicit per-attribute repetition.
- adapters may declare `attribute_rules` to group their handling of selected
  tags and exact attrs without hardcoding per-attribute scans.
- `visibility` decides whether the adapter can observe that vocabulary.
- `adapter type` classifies the adapter family.
- `adapter type` selects a baseline rule set when one is provided.
- `attribute mode` turns the visible attribute into a concrete adapter
  decision.
- `guarantee surface` is the final validated set of promises the adapter can
  make.

Explicit attribute rules remain authoritative over tag-based grouping.

The first three layers are static metadata. The last four are about adapter
behavior.

## Mental Model

```text
Contract declaration
    -> descriptor storage
    -> core traversal
    -> adapter visibility and rule set
    -> attribute handling resolution
    -> validation
    -> guarantee surface
```

The important boundaries are:

- `core` stores descriptors and preserves metadata.
- `adapters` decide how to use visible metadata.
- `validation` checks whether the adapter can keep the promises it claims.

Important distinctions:

- `invisible` does not mean `ignored`.
- `out_of_scope` does not mean `ignored`.
- `ignore` means the adapter sees the attribute but intentionally does
  nothing.
- `error` means the adapter sees the attribute, it is relevant to the adapter
  rules, but it cannot fulfill it.
- `reject` means the adapter sees the attribute, it is valid, but the adapter
  cannot or will not satisfy it.

## Key Terms

| Term | Meaning |
|---|---|
| `attribute` | typed metadata attached to a contract, field, or future overlay |
| `vocabulary` | namespace and ownership boundary for attribute meaning |
| `visibility` | whether a given adapter may observe a vocabulary |
| `adapter type` | adapter family: `log`, `debug`, `wire`, `storage`, `schema`, `config`, `validation` |
| `attribute rules` | adapter-declared rules applied to visible attributes |
| `attribute mode` | how a visible attribute is handled |
| `guarantee surface` | the validated behavior set the adapter can promise |

## What Lives Where

Use the documents this way:

- Read [`attributes.md`](attributes.md) to understand the canonical attribute
  model, DSL, query helpers, and composition rules.
- Read [`adapter_scopes.md`](adapter_scopes.md) to understand how adapter type,
  visibility, and adapter rules relate.
- Read [`security.md`](security.md) when the attribute vocabulary involves
  secrets, redaction, or disclosure control.
- Read [`adapters.md`](adapters.md) when implementing an adapter.

## Core Documents

- [`attributes.md`](attributes.md) - the canonical attribute model (implemented).
- [`vision.md`](vision.md) - designed but not-yet-implemented parts of the model.
- [`adapter_scopes.md`](adapter_scopes.md) - adapter visibility and rule matrix.
- [`security.md`](security.md) - security attribute behavior.
- [`adapters.md`](adapters.md) - adapter author guidance.

## Reading Order

1. Start with [`attributes.md`](attributes.md) for the canonical model.
2. Read [`adapter_scopes.md`](adapter_scopes.md) for adapter visibility and rule matrix.
3. Read [`security.md`](security.md) for practical security behavior.
4. Use [`adapters.md`](adapters.md) when you are implementing an adapter.

## Practical Rule

If you need to answer a question about:

- "what does this attribute mean?" go to `attributes.md`;
- "can this adapter see it?" go to `adapter_scopes.md`;
- "what does the adapter do with it?" go to `adapter_scopes.md`;
- "how should I implement it?" go to `adapters.md`;
