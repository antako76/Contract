# Attributes

> This page describes the **implemented** attribute model. Parts that are
> designed but not yet implemented (adapter projection vocabularies,
> fingerprints, validation modes, the debug/audit dump surface, future
> vocabularies) live in [`vision.md`](vision.md); sections below that depend on
> them are marked with a planned-status note.

Scope: generic attribute / metadata model for the C++ `CONTRACT` library.

Start here if the attribute system feels too abstract.

This document defines the shared attribute vocabulary, visibility model, and
attribute-mode terminology.
Adapter-specific behavior lives in `adapter_scopes.md`.
Implementation guidance lives in `adapters.md`.
Security-specific vocabulary lives in `security.md`.
Vocabulary-local tags may be used as optional selectors, but they do not
replace the vocabulary boundary or the adapter rule matrix.
Adapters may use `attribute_rules` as a convenience layer for groups of tags
and exact attrs, but explicit per-attribute rules still win.
For the short overview of the attribute processing pipeline, start with
`README.md` in this directory.

## 1. One Example

```cpp
CONTRACT(RequestEvent,
    (password, 1,
        contract::security::secret(),
        contract::security::no_log()),

    (tenant, 2,
        contract::schema::type(contract::schema::string),
        ch::low_cardinality())
)
```

This says:

- `password` is sensitive and must not be printed by log/debug adapters;
- `tenant` is ordinary business data, but a storage adapter may use
  `ch::low_cardinality()` as a projection hint;
- the same declaration is not a guarantee by itself;
- each adapter must still declare what it sees and what it enforces.

If this example makes sense, the next page explains adapter rules and modes.
If it does not, stay on this page and read the definitions below first.

Related documents:

- `adapter_scopes.md` — adapter visibility, rules, and modes.
- `adapters.md` — guide for adapter authors.
- `security.md` — security attribute vocabulary and behavior.

---

## 2. Mental Model

```text
Core stores.
Attributes declare.
Adapters guarantee.
Audit verifies expectations.
```

CONTRACT defines stable field identity and typed metadata.

Attributes do not perform behavior by themselves.

Adapters interpret attributes and provide concrete guarantees within their visible rule set.

---

## 3. Processing Model

```text
Attribute
    ↓
Vocabulary
    ↓
Optional tags
    ↓
Visibility
    ↓
Adapter Type
    ↓
Attribute Rules
    ↓
Attribute Mode
    ↓
Guarantee Surface
```

The order matters:

- vocabulary says who owns the meaning;
- optional tags may further group attributes inside the vocabulary;
- visibility says who may see it;
- adapter type is a descriptive family label only; it does not by itself change
  resolution (the resolver consults `visible_vocabularies` and `attribute_rules`,
  not the `adapter_type` value);
- visible vocabularies and attribute rules say how the adapter treats it;
- attribute mode says what the adapter does with it;
- guarantee surface is the final validated result.

A field may declare `contract::security::encrypt()`, but the actual guarantee depends on
which adapter sees it, how the adapter classifies it, and whether validation
succeeds.

---

## 4. Tags

Tags are optional vocabulary-local selectors.

They are used to group attributes for adapter rules without introducing a second
global classification layer.

Examples:

```cpp
contract::check::tag::decode_guard
contract::check::tag::presence
contract::check::tag::length
contract::check::tag::value
```

Tags are not required for every vocabulary.
They do not replace the vocabulary boundary.
They do not guarantee behavior by themselves.

---

## 5. Vocabulary Model

Every attribute belongs to exactly one vocabulary.

A vocabulary is the semantic namespace and ownership boundary.

Common vocabularies:

```cpp
contract::schema::*
contract::check::*
contract::unit::*
contract::security::*
contract::doc::*
```

Adapter projection vocabularies (planned — not yet implemented; see
[Design Vision](vision.md#adapter-projection-vocabularies)):

```cpp
proto::*
ch::*
json::*
binary::*
log::*
wire::*
```

Only the five common vocabularies above exist in code today. The
`proto::`/`ch::`/`log::`/`wire::` names used in examples on this page are
illustrative of the projection model, not implemented vocabularies.
Project-specific vocabularies are allowed but should not leak into the core model.

Rules:

```text
Common attrs describe the value independently of a concrete backend.
Adapter attrs describe a concrete projection.
Ordinary adapters must not consume foreign adapter vocabularies.
Audit/debug adapters may see all attrs read-only.
```

Example:

```cpp
CONTRACT(Event,
    (tenant, 1,
        contract::schema::type(contract::schema::string),
        contract::security::sensitive(),
        ch::low_cardinality())
)
```

A ClickHouse adapter may consume `ch::low_cardinality()`.

A log adapter should not treat `ch::low_cardinality()` as a supported or unsupported log feature. It is outside its visible vocabulary.
See `adapter_scopes.md` for the rules that turn visibility into concrete adapter behavior.

---

## 6. Visibility Model

Adapters declare which vocabularies are visible to them.

Example:

```cpp
struct clickhouse_adapter_traits {
    using visible_vocabularies = contract::vocabularies<
        contract::schema::vocabulary,
        contract::check::vocabulary,
        contract::unit::vocabulary,
        contract::security::vocabulary,
        ch::vocabulary
    >;
};
```

Audit/debug adapters may use all-attrs read-only visibility:

```cpp
struct debug_adapter_traits {
    static constexpr attribute_visibility visibility =
        attribute_visibility::all_attrs_read_only;
};
```

Core visibility queries are:

```cpp
contract::visible_vocabularies_t<AdapterTraits>
contract::is_vocabulary_visible_v<AdapterTraits, Vocabulary>
contract::is_attribute_visible_v<AdapterTraits, Attr>
contract::has_all_attributes_read_only_v<AdapterTraits>
```

Ordinary adapters must declare a duplicate-free
`contract::vocabularies<...>` set. An adapter using
`all_attrs_read_only` may omit that set.

Visibility answers only whether metadata is observable. It does not imply that
an attribute is supported, ignored, enforced, or part of a guarantee.

Important distinction:

```text
Invisible is not ignored.
```

If a vocabulary is not visible to an adapter, attributes from that vocabulary are not part of that adapter's guarantee surface.

Example:

```cpp
ch::low_cardinality()
```

For `LogAdapter`:

```text
visibility: invisible
behavior: not applicable
```

For `ClickHouseAdapter`:

```text
visibility: visible
behavior: hint or enforce
```

---

## 7. Attribute Mode

Attribute mode describes what an adapter does with a visible attribute.
Adapter-specific mode matrices are defined in `adapter_scopes.md`.

```cpp
enum class attribute_mode
{
    enforce,
    hint,
    display,
    ignore,
    out_of_scope,
    reject,
    error
};
```

### enforce

The adapter implements the declared behavior.

Example:

```text
LogAdapter + contract::security::no_log()
```

### hint

The adapter uses the attribute to improve projection or performance, but correctness does not depend on it.

Example:

```text
ClickHouseAdapter + ch::low_cardinality()
```

### display

The adapter displays metadata but does not enforce behavior.

Example:

```text
Schema documentation adapter showing security classification.
```

### ignore

The adapter sees the attribute and intentionally ignores it.

### out_of_scope

The attribute is meaningful, but outside adapter responsibility.

### reject

The attribute requirement is valid, but the adapter cannot satisfy it.

Validation should fail.

### error

The visible attribute is relevant but unsupported.

Validation should fail.

Example:

```text
StorageAdapter + contract::security::encrypt() without an approved encryption policy.
```

---

## 8. Adapter Scopes

Adapter rules are the adapter behavior boundary.
The canonical mode matrix and examples live in `adapter_scopes.md`.

---

## 9. Validation Surface

Attribute modes summarize how an adapter treats a visible attribute.
The canonical mode matrix and examples live in `adapter_scopes.md`.

---

## 10. Guarantee Surface

A CONTRACT attribute is a declaration, not a global guarantee.

A guarantee exists only when a concrete adapter:

1. sees the attribute vocabulary;
2. recognizes the concrete attribute;
3. marks it as `enforce`, `hint`, `display`, `ignore`, `out_of_scope`, or `reject`;
4. passes validation;
5. runs in a mode where enforcement is enabled.

The adapter handling matrix is the public guarantee surface of that adapter.

The static validation API is:

```cpp
constexpr auto summary =
    contract::validate_adapter_mode<T, adapter_traits>();

contract::validate_adapter_mode<T, adapter_traits>(
    [](const auto& entry) {
        // entry keeps the concrete attribute type and payload.
    });

static_assert(contract::adapter_mode_is_valid_v<T, adapter_traits>);
contract::require_adapter_mode<T, adapter_traits>();
```

The callback entry contains the target, field name, declared and effective
field ids, BASE offset, typed attribute payload, resolution, adapter type,
and validation issue.

`error` produces `unsupported_attribute`.
`reject` produces `rejected_requirement`.
Both make the summary invalid. Invisible, ignore, display, hint, and
out_of_scope decisions remain visible in the report but are not validation
errors.

This is the main rule users should rely on.

---

## 11. Public DSL

Preferred public style:

```cpp
CONTRACT(Type,
    ATTRS(type_attr1, type_attr2, ...),

    BASE(BaseType, offset),

    (field_name, field_id),
    (field_name, field_id, attr1, attr2, ...)
)
```

No `FIELD(...)` wrapper is required in the public DSL.

The DSL normalizes every attribute expression to one descriptor:

```cpp
attribute_descriptor<Attr> {
    Attr attribute;
    std::string_view source;
};
```

`source` preserves the declaration text, for example
`contract::check::max_length(64)`. The semantic object and its metadata are
stored together; consumers must not maintain a parallel source array.

---

## 12. Field Id Rules

The second element in `(field, id, attrs...)` is mandatory.

Field id is not an optional attribute.

Field id is core contract metadata.

Rules:

```text
id must be positive;
id must be unique in the flattened contract after BASE offsets;
id is stable across versions;
removed ids must not be reused;
field id participates in schema/protobuf/wire compatibility;
field id may be used as protobuf tag number;
auto-generated ids are not allowed for stable export/wire/storage contracts.
```

Example:

```cpp
(field_name, 7)
```

means:

```text
field name = field_name
field id   = 7
attrs      = empty
```

---

## 13. Contract-Level Attributes

`ATTRS(...)` attaches metadata to the whole contract/message/type.

Use cases:

```text
reserved ids/names/ranges;
message-level versioning;
message-level schema name overrides;
stable adapter/schema metadata that belongs to the type as a whole.
```

Example:

```cpp
CONTRACT(Header,
    ATTRS(
        contract::schema::reserved("old_source", 7),
        contract::schema::reserved_range(20, 29)
    ),
    (service, 1),
    (operation, 2)
)
```

The `ATTRS(...)` mechanism and the `schema::reserved*` attributes are
implemented and stored on the contract. Adapter *consumption* of them is
planned but not yet implemented — the interpretations below are the intended
direction (see [Design Vision](vision.md)), not current behavior:

```text
protobuf adapter generates reserved tags/names;
tagged wire adapter validates that active fields do not reuse reserved ids;
debug adapter prints reserved ids/names;
storage adapters may use reserved names as tombstones for removed columns.
```

---

## 14. Field-Level Attributes

Field-level attributes are attached after field id:

```cpp
(field_name, id, attr1, attr2, ...)
```

Example:

```cpp
(email, 3,
    contract::schema::type(contract::schema::string),
    contract::check::max_length(320),
    contract::security::sensitive(),
    contract::security::no_log()
)
```

Attributes must be typed values, not stringly-typed key/value pairs.

Each attribute must define:

```text
vocabulary;
targets;
repeatability;
```

The CONTRACT DSL captures the source spelling independently of `attr_traits`.
Programmatically constructed attribute packs may leave that source empty.

---

## 15. Attribute Queries and Composition

Contract-level and field-level attributes are separate scopes. Contract
attributes are not implicitly copied to every field.

`BASE` preserves the declared attributes of imported fields. It changes only
the effective field id.

Core queries operate on a descriptor:

```cpp
contract::attributes_of_t<Descriptor>
contract::has_attribute_v<Descriptor, Attr>
contract::attribute_count_v<Descriptor, Attr>
contract::attributes_in_vocabulary_t<Descriptor, Vocabulary>
```

Payloads are available through:

```cpp
contract::get_attributes(descriptor)
contract::get_attributes_in_vocabulary<Vocabulary>(descriptor)
descriptor.attributes.find<Attr>() // Attr* or nullptr
descriptor.attributes.get<Attr>()  // Attr&, requires presence
```

`attributes<Attr...>::entries` is a tuple of `attribute_descriptor<Attr>`.
Tuple traversal exposes both `entry.attribute` and `entry.source`; semantic
lookup returns the attribute object itself rather than assuming a particular
payload shape.

Explicit attribute-pack composition uses:

```cpp
contract::compose_attributes(left, right)
```

Composition rules:

```text
left-hand attributes precede right-hand attributes;
repeatable attributes preserve declaration order;
duplicate non-repeatable attributes are rejected;
composition does not imply inheritance between contract and field scopes;
vocabulary filtering preserves relative declaration order and payloads.
```

These rules provide the merge primitive for future overlays without adding
overlay precedence or override semantics in P0.

---

## 16. Attribute Traits

Every attribute type should have traits.

```cpp
struct schema_vocabulary {};
struct check_vocabulary {};
struct unit_vocabulary {};
struct security_vocabulary {};

struct attr_targets {
    bool type;
    bool field;
    bool enum_type;
    bool enum_value;
    bool overlay;
};

template<class Attr>
struct attr_traits {
    using vocabulary = ...;
    static constexpr attr_targets targets = ...;
    static constexpr bool repeatable = false;

    // Optional. Attributes that participate in tag-based rules declare the
    // tags they carry here (e.g. check attributes tag themselves as
    // decode guards). Omission means the attribute matches only vocabulary-
    // and exact-level rules.
    using tags = contract::tags<...>;
};
```

Note: `attribute_rules` is declared on the *adapter* traits
(`adapter_traits::attribute_rules`), not on `attr_traits`. `attr_traits`
describes the attribute; the adapter's traits describe how that adapter treats
each vocabulary/tag/attribute.

Traits are used for:

```text
target validation;
duplicate detection;
repeatability rules;
debug/audit rendering;
adapter visibility filtering;
policy applicability;
adapter handling matrix matching;
conflict validation;
overlay merge rules;
schema fingerprints.
```

---

## 17. Allowed Attribute Payloads

Allowed P0 attribute payloads:

```text
empty marker structs;
small integral constants;
string literals / fixed string wrappers;
enums;
type tags;
small constexpr structs composed of the above.
```

Avoid in P0:

```text
owning std::string;
runtime heap containers;
function pointers/lambdas;
arbitrary runtime callbacks;
polymorphic objects.
```

Rationale:

```text
attrs should be constexpr-friendly;
adapter validation should be compile-time where possible;
audit/fingerprint generation should not depend on hidden runtime state.
```

---

## 18. Common Vocabularies

### 18.1 schema

Schema lifecycle attributes:

```cpp
contract::schema::reserved("old_source", 7)
contract::schema::reserved_id(7)
contract::schema::reserved_name("old_source")
contract::schema::reserved_range(20, 29)
contract::schema::deprecated()
contract::schema::alias("old_name")
```

Schema type attributes:

```cpp
contract::schema::type(contract::schema::string)
contract::schema::type(contract::schema::bytes)
contract::schema::type(contract::schema::fixed32)
contract::schema::type(contract::schema::fixed64)
```

Meaning:

```text
contract::schema::type(...) describes the schema/logical type of the field value.
It refines C++ storage/value type when C++ alone is ambiguous.
It is not a C++ type and not an adapter-specific projection type.
```

Examples:

```cpp
(service, 1,
    contract::schema::type(contract::schema::string),
    contract::check::max_length(16)
)

(payload, 2,
    contract::schema::type(contract::schema::bytes),
    contract::check::max_bytes(4096)
)

(path_hash, 3,
    contract::schema::type(contract::schema::fixed64)
)
```

No P0 `format::binary()` is needed:

```text
contract::schema::type(contract::schema::bytes) means opaque bytes.
contract::schema::type(contract::schema::string) + future format::base64() means a string containing base64 text.
```


#### Schema Lifecycle Attributes

##### contract::schema::deprecated()

Category:

```text
Schema lifecycle attribute.
```

Meaning:

```text
The field is still part of the contract but should not be used by new producers.
Readers should continue supporting it for compatibility.
```

Example:

```cpp
(old_path, 17,
    contract::schema::deprecated()
)
```

Rules:

```text
Deprecated fields remain valid.
Deprecated fields keep their ids.
Deprecated fields may later transition to reserved state.
Deprecated fields participate in compatibility checks.
```

##### contract::schema::alias(...)

Category:

```text
Schema evolution attribute.
```

Meaning:

```text
Alternative historical name for the same field.
```

Example:

```cpp
(user_id, 5,
    contract::schema::alias("uid"),
    contract::schema::alias("user")
)
```

Rules:

```text
Alias is repeatable.
Alias does not create a new field id.
Alias participates in schema evolution and migration.
Alias names must not collide with active field names.
```

### 18.2 check

Portable constraints:

```cpp
contract::check::min_value(...)
contract::check::max_value(...)
contract::check::min_length(...)
contract::check::max_length(...)
contract::check::max_bytes(...)
contract::check::max_items(...)
contract::check::not_empty()
```

Constraints are declarations.

A validation adapter may enforce them.

Other adapters may consume them as hints, display them, ignore them, or reject unsupported combinations.

### 18.3 unit

Canonical unit attribute:

```cpp
contract::unit::ucum("...")
```

Examples:

```cpp
contract::unit::ucum("ns")
contract::unit::ucum("ms")
contract::unit::ucum("s")
contract::unit::ucum("m")
contract::unit::ucum("km")
contract::unit::ucum("kg")
contract::unit::ucum("By")
contract::unit::ucum("1/s")
```

Convenience helpers:

```cpp
contract::unit::seconds()
contract::unit::milliseconds()
contract::unit::microseconds()
contract::unit::nanoseconds()
contract::unit::unix_seconds()
contract::unit::unix_milliseconds()
```

Rules:

```text
unit attrs explain quantity interpretation;
unit attrs do not mandate wire/storage representation;
CONTRACT P0 is not a unit conversion framework.
```

### 18.4 security

Security is the official P0 Security / Data Protection vocabulary.

P0 attributes:

```cpp
contract::security::sensitive()
contract::security::secret()
contract::security::no_log()
contract::security::encrypt()
```

`contract::security::redact()` is intentionally not part of the initial vocabulary.

Reason:

```text
redact is an exposure strategy.
secret/no_log/encrypt express stable data protection intent.
Adapters may choose redact/hash/omit as their policy for sensitive/secret fields.
```

#### contract::security::sensitive()

Classification attribute.

Meaning:

```text
field contains sensitive data;
field should not be dumped or exposed casually;
storage/transmission may be allowed under platform policy.
```

Examples:

```text
email;
phone;
tenant-sensitive id;
internal user id;
business identifier with restricted visibility.
```

#### contract::security::secret()

Classification attribute stronger than `sensitive`.

Meaning:

```text
field contains credential/token/key/password/session-cookie/bearer-like value;
log/console/debug sinks must never output raw value by default;
storage/wire adapters need explicit safe policy or strict-mode rejection when applicable.
```

`secret` implies sensitive-level handling.

`secret` does not automatically imply `encrypt()`.

#### contract::security::no_log()

Exposure policy attribute.

Meaning:

```text
field must be omitted from ordinary logs and diagnostic output.
```

Debug adapters may show metadata without value.

#### contract::security::encrypt()

Storage/wire policy attribute.

Meaning:

```text
field requires encryption in scopes where the raw value is persisted or transmitted.
```

Scope behavior:

```text
storage adapter -> enforce / hint / reject depending on storage policy;
wire adapter    -> enforce / hint / reject depending on transport policy;
log adapter     -> out_of_scope;
debug adapter   -> out_of_scope;
schema adapter  -> metadata only.
```

Encryption may be field-level, message-level, envelope-level, storage-layer, or transport-layer,
depending on adapter rules and approved project policy.

The current binary adapter enforces `contract::security::encrypt()` as a
simple streaming XOR transform keyed by adapter options.

Rules:

```text
contract::security::secret() implies never_raw for log/console/debug sinks;
contract::security::secret() does not imply contract::security::encrypt();
contract::security::encrypt() must be explicit when encryption is required;
contract::security::no_log() and raw human-readable output are incompatible;
security policy attributes must not be silently ignored in relevant scopes;
unsafe raw display/storage/transmission requires explicit unsafe mode and must be visible in audit output.
```

---

## 19. Adapter Projection Vocabularies

> **Status: planned — not yet implemented.** The design detail lives in [Design Vision](vision.md#adapter-projection-vocabularies); it is not part of current behavior.

## 20. Inline Attributes vs Overlays

Inline attributes:

```cpp
CONTRACT(Event,
    (tenant, 1,
        contract::schema::type(contract::schema::string),
        contract::security::sensitive(),
        ch::low_cardinality()
    )
)
```

Use inline attributes when:

```text
metadata is part of the canonical data path;
auditability in one place is valuable;
the type is project-owned;
projection is stable and not environment-specific.
```

Overlay attributes:

```cpp
CONTRACT(Event,
    (tenant, 1,
        contract::schema::type(contract::schema::string),
        contract::security::sensitive()
    )
)

CONTRACT_OVERLAY(Event, clickhouse_profile,
    (tenant,
        ch::low_cardinality()
    )
)
```

Use overlays when:

```text
type is reusable or external;
adapter projection is optional;
metadata belongs to a separate module;
teams prefer separating storage/wire/log policy;
multiple profiles need different projections.
```

Overlay status:

```text
P1 / future.
```

Overlay rules:

```text
overlay may add attrs only;
overlay must not change field id/name/type/access;
overlay must not remove fields;
duplicate attr conflicts between base and overlay are errors unless explicit override is supported;
adapters using overlays must validate them;
audit output should show effective attrs and source.
```

---

## 21. External Contracts and Access Hooks

For classes that cannot or should not be modified, define the contract externally.

```cpp
struct MetricsHeader {
    char service[32];
    char tenant[64];
    std::uint32_t bucket_start;
    std::uint64_t path_hash;
};

constexpr auto contract_definition(contract::tag<MetricsHeader>) {
    using Self = MetricsHeader;

    return contract::make_contract<Self>(
        "MetricsHeader",
        contract::attrs(),

        contract::field<&Self::service, field::service, 1>(
            "service",
            contract::schema::type(contract::schema::string),
            contract::check::max_length(32)
        ),
        contract::field<&Self::tenant, field::tenant, 2>(
            "tenant",
            contract::schema::type(contract::schema::string),
            contract::check::max_length(64),
            contract::security::sensitive(),
            contract::security::no_log()
        )
    );
}
```

External `contract_get` / `contract_set` hooks are adapter-independent logical access hooks.

They are not serialization hooks.

```cpp
std::string_view contract_get(contract::tag<field::tenant>, const Header& h);
void contract_set(contract::tag<field::tenant>, Header& h, std::string_view value);
```

Adapter-specific hooks are allowed only as escape hatches.

```cpp
template<class Writer>
void contract_wire_write(
    contract::tag<field::payload>,
    Writer& out,
    const PayloadEvent& obj
);
```

Rules:

```text
hook is field-specific and adapter-specific;
hook is not represented as inline lambda attr;
hook must be testable;
hook must participate in adapter fingerprint where relevant;
audit output must show custom_hook marker;
hook should require review justification.
```

---

## 22. Fingerprints

> **Status: planned — not yet implemented.** The design detail lives in [Design Vision](vision.md#fingerprints); it is not part of current behavior.

## 23. Validation

Core validation:

```text
field id present;
field id positive;
field ids unique after BASE offsets;
reserved ids/ranges not reused;
reserved names not reused where applicable;
attr target valid;
non-repeatable attrs not duplicated;
common vocabulary conflicts detected;
BASE contract exists and is accessible.
```

Common vocabulary validation:

```text
contract::schema::type duplicated/conflicting -> error;
contract::security::secret + contract::security::sensitive -> allowed/redundant warning;
contract::security::no_log + human-readable raw output -> adapter validation error;
check min/max contradictions -> error where statically known;
unit duplicate/conflict -> error unless explicitly repeatable.
```




## 24. Conflict Ownership

Conflict validation is layered.

### 24.1 Core Ownership

Core validates:

```text
duplicate field ids;
reserved id violations;
reserved name violations;
duplicate non-repeatable attrs;
invalid attr targets;
invalid or inaccessible BASE;
invalid field descriptor structure.
```

### 24.2 Vocabulary Ownership

Vocabulary modules validate semantic conflicts.

Examples:

```text
contract::schema::type(string) + contract::schema::type(bytes);
contract::security::secret + hypothetical contract::security::public;
unit conflicts;
check min/max contradictions.
```

### 24.3 Adapter Ownership

Adapters validate projection conflicts.

Examples:

```text
proto::type(fixed64) + proto::type(sint64);
ch::type(UInt64) + ch::type(DateTime);
json::omit_empty() on a required projection where omission is forbidden.
```

Rule:

```text
Core validates structure.
Vocabulary validates meaning.
Adapter validates projection.
```

---

## 25. Validation Modes

> **Status: planned — not yet implemented.** The design detail lives in [Design Vision](vision.md#validation-modes); it is not part of current behavior.

## 26. Adapter Validation

Core descriptor validation rejects invalid targets, duplicate non-repeatable
attrs, invalid ids, and invalid BASE composition while descriptors are built.

Adapter handling validation then traverses contract attrs and flattened field
attrs through the same resolver used by adapters. It reports:

```text
unsupported visible semantic/hint attrs;
rejected policy attrs;
ignored attrs;
out-of-scope policies;
validated guarantees;
declared and effective BASE field ids.
```

Formatting or printing a handling report belongs to debug/audit adapters. The
core validation API returns typed entries and a summary; it does not format
text.

Unsafe overrides are not part of the current core model. They require a
separate explicit override API before validation can report them.

---

## 27. Debug and Audit

> **Status: planned — not yet implemented.** The design detail lives in [Design Vision](vision.md#debug-and-audit); it is not part of current behavior.

## 28. P0 Scope

Core P0:

```text
ATTRS(...);
(field, id, attrs...);
typed attr storage;
attr_traits with vocabulary;
common P0 vocabularies: schema/check/unit/security/doc;
visibility filtering;
handling matrix declarations;
contract validation;
adapter validation;
console audit output;
target-adapter support visualization.
```

P0 attributes:

```text
schema:
  reserved, reserved_id, reserved_name, reserved_range, deprecated, alias, type(string/bytes/fixed32/fixed64)

check:
  min_value, max_value, min_length, max_length, max_bytes, max_items, not_empty

unit:
  ucum, seconds, milliseconds, microseconds, nanoseconds, unix_seconds, unix_milliseconds

security:
  sensitive, secret, no_log, encrypt

doc:
  comment
```

P0 adapters:

```text
console/debug audit adapter;
target-adapter handling report;
minimal log adapter with security handling;
one projection adapter prototype: protobuf or ClickHouse.
```

P1 candidates:

```text
overlays;
format vocabulary: date_time, uuid, email, uri, base64;
strict/permissive mode coverage in all adapters;
generated adapter handling matrix docs;
advanced hooks/codecs;
rich adapter projection attrs;
contract::security::redact if a stable use case remains;
more schema generators.
```

---


## 29. Future Vocabulary Candidates

> **Status: planned — not yet implemented.** The design detail lives in [Design Vision](vision.md#future-vocabulary-candidates); it is not part of current behavior.

## 30. Non-Goals

P0 is not:

```text
runtime reflection framework;
universal ORM;
universal serializer;
full protobuf replacement;
full migration system;
unit conversion framework;
legal PII/GDPR taxonomy;
place for inline behavior lambdas.
```

---

## 31. Final Summary

```text
CONTRACT defines stable field identity.
Attributes declare schema type, constraints, units and security policy.
Adapters declare what they see and what they guarantee.
Adapter rules define behavior.
The handling matrix is the guarantee surface.
Audit prevents false expectations.
```
