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

Concretely, the first step - `attribute` declared in a `CONTRACT(...)` entry -
looks like this
([`tests/unit/adapters/console/console_security_test.cpp`](../../tests/unit/adapters/console/console_security_test.cpp)):

```cpp
CONTRACT(SecretRecord,
    (password, 1, contract::security::secret()),
    (tokens, 2, contract::security::no_log()),
    (profile, 3, contract::security::sensitive()),
    (note, 4)) // no attribute - the console adapter renders it normally
```

`contract::security` is the vocabulary here; `secret`/`no_log`/`sensitive` are
the attributes it defines. [`security.md`](security.md) covers what each one
means and how adapters are expected to react to it.

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

## Where To Go Next

Read in this order:

1. [`attributes.md`](attributes.md) - "what does this attribute mean?" - the
   canonical model, DSL, query helpers, and composition rules (implemented).
2. [`adapter_scopes.md`](adapter_scopes.md) - "can this adapter see it, and
   what does it do with it?" - adapter type, visibility, and rule matrix.
3. [`security.md`](security.md) - "how do secrets/redaction actually behave?"
   - practical security vocabulary examples.
4. [`adapters.md`](adapters.md) - "how do I implement an adapter against
   this?" - adapter author guidance.
5. [`vision.md`](vision.md) - designed but not-yet-implemented parts of the
   model (projection vocabularies, fingerprints, validation modes).
