# CONTRACT Adapter Author Guide

Scope: implementation guide for CONTRACT adapters.
Canonical terminology lives in `attributes.md` and `adapter_scopes.md`.
Adapter-internal field/codec rules live in [`../adapters/README.md`](../adapters/README.md).

Related documents:

- `attributes.md`
- `adapter_scopes.md`
- `security.md`

---

## 1. Adapter Responsibility

Adapters convert CONTRACT metadata and values into a target representation.

Examples:

```text
JSON
Binary
Protobuf
ClickHouse
Logs
Debug output
YAML config
Validation
```

An adapter must declare:

```text
adapter type;
visible vocabularies;
attribute rules;
strict-mode behavior;
fingerprint inputs where relevant.
```

`adapter type` classifies the adapter family.
`attribute rules` state the public behavior matrix for that adapter.

---

## 2. Recommended Architecture

```text
Contract Traversal
        ↓
Field Policy Resolution
        ↓
Primitive Writers
```

Primitive writers should not contain scattered checks for all attributes.
For codec and field dispatch organization, see [`../adapters/README.md`](../adapters/README.md).

Security and policy handling should be centralized at field-policy level.
Vocabulary-local tags may help group rules, but explicit per-attribute rules
remain authoritative.
Adapters may declare `attribute_rules` to map a tag like
`contract::check::tag::decode_guard` or an exact attribute to a shared
attribute mode instead of writing repeated per-attribute overloads.

---

## 3. Contract Traversal

Responsible for:

```text
visiting fields;
visiting nested structures;
visiting arrays/containers;
preserving field ids;
preserving type-level attributes;
using effective ids after BASE offsets.
```

Adapters should use CONTRACT-provided descriptors instead of manually accessing fields where possible.

---

## 4. Field Policy Resolution

Responsible for:

```text
visible vocabulary filtering;
attribute mode lookup;
security policy handling;
adapter-specific projection decisions;
capability validation;
conflict handling.
```

Example:

```cpp
// resolve_attribute_mode takes a single attribute (not a field) and returns an
// attribute_resolution carrying { kind, mode }; resolve per visible attribute.
const auto resolution = resolve_attribute_mode<AdapterTraits>(attr);
if (resolution.kind == attribute_resolution_kind::visible) {
    apply_mode(resolution.mode, field, ctx);
}
```

Avoid:

```cpp
write_string_secret(...)
write_string_no_log(...)
write_string_encrypt(...)
```

Do not explode function signatures with attribute-specific overloads.

Attributes are metadata consumed by the adapter policy layer.

---

## 5. Primitive Writers

Responsible only for rendering values that have already passed policy resolution.
Field/codec organization details are in [`../adapters/README.md`](../adapters/README.md).

Examples:

```cpp
write_int(...)
write_string(...)
write_object(...)
write_array(...)
write_null(...)
```

Primitive writers should not decide whether a field is secret, encrypted, omitted, or redacted.

---

## 6. Adapter Traits

Example:

```cpp
struct log_adapter_traits
{
    static constexpr adapter_type type = adapter_type::log;

    using visible_vocabularies = contract::vocabularies<
        contract::schema::vocabulary,
        contract::check::vocabulary,
        contract::unit::vocabulary,
        contract::security::vocabulary,
        log::vocabulary
    >;

    using attribute_rules = contract::attribute_rules<
        contract::default_for<contract::schema::vocabulary>::display,
        contract::default_for<contract::check::vocabulary>::ignore,
        contract::default_for<contract::unit::vocabulary>::display,
        contract::default_for<contract::security::vocabulary>::ignore,

        contract::for_tag<contract::check::tag::decode_guard>::enforce,
        contract::for_attr<contract::security::no_log>::out_of_scope,
        contract::for_attr<contract::security::secret>::enforce>;
};
```

`type` tells readers what family this adapter belongs to.
`attribute_rules` declare exact-attr rules, tag rules, and vocabulary defaults.
An applicable visible attribute without an explicit rule resolves to `error`
unless the adapter is in read-only visibility mode.

---

## 7. Visibility Rules

Visibility and rule matrix live in `adapter_scopes.md`.
Ordinary adapters should expose only declared vocabularies in their public
matrix. Debug/audit adapters may show read-only metadata for broader reporting.
Adapter type does not change visibility by itself; visible vocabularies do.

---

## 8. Mode Matrix

Every adapter must publish a mode matrix.
The rule vocabulary and examples live in `adapter_scopes.md`.
This guide only says: do not hide attribute behavior inside leaf writers.
The matrix is where the adapter type and rule list become concrete behavior.

---

## 9. Validation

Adapters should require static validation before values are rendered or
persisted:

```cpp
contract::require_adapter_mode<T, adapter_traits>();
```

The check must run at the adapter's guaranteed dispatch point for every
CONTRACT type it processes, including nested contract values. Do not place it
only in a top-level convenience API if nested values can bypass that API.

The exact integration point depends on the adapter:

```text
binary adapter  -> contract-specific operator or contract codec;
console adapter -> contract codec or common contract renderer;
other adapters  -> the single dispatch path that recognizes CONTRACT types.
```

Container, optional, variant, and custom codecs already own recursive value
dispatch. When they encounter a nested CONTRACT type, that normal dispatch
must reach the validation point. Core validation intentionally validates one
contract descriptor at a time and does not duplicate the adapter's type
recursion.

`require_adapter_mode` contains a compile-time assertion and adds no runtime
work.

For audit and diagnostics, use the callback overload:

```cpp
auto summary = contract::validate_adapter_mode<T, adapter_traits>(
    [](const auto& entry) {
        // Render or collect the typed decision.
    });
```

The callback and strict check consume the same `attribute_resolution` values.
Adapter initialization separately validates keys, TLS, KMS access, and other
runtime configuration.

### Binary Adapter

Binary is the first structural wire adapter used to prove the attribute model
against a real recursive serializer.

Binary requirements are:

```text
validate each CONTRACT type at the adapter's dispatch boundary;
keep primitive codecs free of attribute policy;
preserve the existing wire format unless an attribute explicitly changes it;
call validation before nested CONTRACT values are traversed;
do not hide policy decisions in leaf codecs.
```

In P0, binary is a wire codec, not a full security policy engine.

That means:

```text
contract::check::max_length(), contract::check::max_bytes(), and contract::check::max_items()
are enforced on both the write side (before emitting) and the read side (as decode guards);
other contract::check::* attrs are ignored for now unless a future binary policy chooses to
handle them explicitly;
contract::security::sensitive() and contract::security::secret() are ignored by the binary codec;
contract::security::no_log() is out_of_scope for the current binary rule set;
contract::security::encrypt() is enforced via a simple streaming XOR transform in the current
binary implementation;
schema metadata remains separate from raw binary serialization and may be added later as an
explicit projection layer.
```

Binary should stay compatible with existing payloads by default. If an
attribute changes the on-wire layout, that must be explicit, versioned, and
covered by round-trip tests.

---

## 10. Debug and Capability Reporting

Debug and capability reporting should follow the same mode matrix used by
validation. See `adapter_scopes.md` for the canonical report fields and
mode vocabulary.

---

## 11. Schema and Fingerprints

Adapters that generate schemas or persistent/wire formats should document
their fingerprint inputs. The canonical fingerprint categories are described
in `attributes.md`; this guide only reminds adapter authors to surface them.

---

## 12. Unsafe Modes

Unsafe mode rules live in `adapter_scopes.md`.
If an adapter supports unsafe escape hatches, they must be explicit and visible
in audit/debug output.

---

## 13. Adapter-Specific Hooks

Adapter-specific hooks are allowed only for exceptional cases.

```cpp
template<class Writer>
void contract_wire_write(
    const PayloadEvent::contract_fields::payload&,
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

## 14. Adapter Implementation Checklist

Before publishing an adapter, document:

```text
visible vocabularies;
mode matrix;
strict/permissive behavior;
security behavior;
mode outcomes;
fingerprint inputs;
debug report support;
unsafe modes;
known unsupported attrs.
```

---

## 15. Final Rule

```text
Adapters should be easy to write,
but their guarantees must be explicit.
```
