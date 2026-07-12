# Adapter Rules and Capability Matrix

> **Status.** The resolution mechanism (§2, §6) and the wire-adapter behavior
> are implemented. The adapter *profiles* (§8: Log/Debug/Storage/Schema/Config/
> Validation) and most of the capability matrix (§9) are a **design projection**
> across intended adapter families — in code today only wire adapters exist
> (`binary`, `compact`). CONTRACT does not impose requirements on adapters: an
> adapter simply declares what it handles through its `adapter_traits`
> (`visible_vocabularies` + `attribute_rules`). The "REQUIRED/OPTIONAL" wording
> below is design intent for those planned families, not an enforced contract.

Scope: adapter visibility, rule matrices, and behavior for CONTRACT attributes.

Related documents:

- `attributes.md` - canonical attribute model.
- `adapters.md` - adapter author guide.
- `security.md` - practical security behavior.

---

## 1. Motivation

CONTRACT supports extensible attributes and an open adapter ecosystem.

Not every adapter has the same guarantee surface.

Examples:

```text
A debug adapter is not responsible for database encryption.
A protobuf adapter is not responsible for log redaction.
A storage adapter must not silently ignore storage-related encryption requirements.
```

Adapter rules define which attributes are relevant, required, optional, ignored,
out of scope, or rejected.

---

## 2. Core Rule

```text
Visible attributes resolve through exact rules, then tag rules, then vocabulary defaults.
```

## 3. Adapter Classification

Adapter type is the adapter family or classification.

Canonical adapter types:

```cpp
enum class adapter_type
{
    log,
    debug,
    wire,
    storage,
    schema,
    config,
    validation
};
```

| Type | Primary behavior |
|---|---|
| `log` | human-readable operational logs |
| `debug` | diagnostics and audit |
| `wire` | transport representation |
| `storage` | persistent representation |
| `schema` | metadata generation |
| `config` | configuration loading |
| `validation` | rule enforcement |

Type classifies the adapter family.

---

## 4. Baseline Rules

`adapter_type` can be used as a naming anchor for a baseline rule list, but
the baseline is declared directly in the adapter traits.

Example wire adapter declaration:

```cpp
struct adapter_traits {
    static constexpr adapter_type type = adapter_type::wire;

    using visible_vocabularies = contract::vocabularies<
        contract::check::vocabulary,
        contract::security::vocabulary>;

    using attribute_rules = contract::attribute_rules<
        contract::default_for<contract::check::vocabulary>::ignore,
        contract::default_for<contract::security::vocabulary>::ignore,
        contract::for_tag<contract::check::tag::decode_guard>::enforce,
        contract::for_attr<contract::security::no_log>::out_of_scope,
        contract::for_attr<contract::security::encrypt>::enforce>;
};
```

The important point is that the baseline stays visible in the adapter and is
not hidden behind a second API layer.

---

## 5. One Field, Two Adapters

```cpp
CONTRACT(Event,
    (password, 1,
        contract::security::secret(),
        contract::security::no_log()),

    (tenant, 2,
        contract::schema::type(contract::schema::string),
        ch::low_cardinality())
)
```

For a log adapter:

```text
password -> required to omit raw value
tenant   -> visible, but low_cardinality is out of scope
```

For a storage adapter:

```text
password -> security-sensitive, may require encryption or rejection
tenant   -> low_cardinality may be hint or enforce
```

The same field declaration can lead to different outcomes depending on adapter
type and rule set.

---

## 6. Attribute Mode

Every visible attribute must result in one of:

```text
enforce
hint
display
ignore
out_of_scope
reject
error
```

`enforce` means the adapter consumes the attribute and guarantees it.
`hint` means the adapter may use the attribute, but does not guarantee it.
`display` means the adapter only shows the attribute in diagnostics or audit.
`ignore` means the adapter sees the attribute and intentionally ignores it.
`out_of_scope` means the attribute is meaningful, but outside adapter responsibility.
`reject` means the attribute requirement is valid, but the adapter cannot or will not satisfy it.
`error` means the attribute usage or combination is invalid or unsupported for this adapter.

The common entry point is:

```cpp
auto result = contract::resolve_attribute_mode<AdapterTraits>(attr);
```

It returns `attribute_resolution`, whose `kind` is `invisible` or `visible`.
Validation and adapter execution must consume this result instead of
reimplementing the decision rules.

Resolution order is deterministic:

```text
invisible vocabulary
  -> invisible

visible attribute with exact attr rule
  -> exact attribute rule

visible attribute with tag rule
  -> tag rule

visible attribute with vocabulary default
  -> vocabulary default

visible attribute without a matching rule
  -> error
```

Exact attr rules override tag rules.
Tag rules override vocabulary defaults.
Conflicting rules at the same specificity are a model error.

---

## 8. Adapter Profiles

### 8.1 Log Adapter

Produces human-readable operational logs.

Examples:

```text
text logs
structured JSON logs
audit logs
application diagnostic messages
```

### REQUIRED

```text
contract::security::no_log()
contract::security::secret()
```

### OPTIONAL

```text
contract::security::sensitive()
contract::doc::comment()
log::name()
```

### OUT OF SCOPE

```text
contract::security::encrypt()
```

### Required Behavior

```text
contract::security::no_log() -> omit field/value from ordinary logs
contract::security::secret() -> never output raw value
contract::security::sensitive() -> adapter policy decides redact/hash/allow
```

Log adapters must build output through CONTRACT field traversal and field policy resolution.

---

### 8.2 Debug Adapter

Produces human-readable diagnostic output and adapter inspection.

Examples:

```text
debug dump
console rendering
target-adapter handling report
troubleshooting view
```

### REQUIRED

```text
contract::security::no_log()
contract::security::secret()
attribute visibility
capability visibility
adapter decisions
```

### OPTIONAL

```text
contract::security::sensitive()
contract::doc::comment()
```

### OUT OF SCOPE

```text
contract::security::encrypt()
```

### Required Behavior

Debug output must prevent accidental raw disclosure.

It may show metadata without value:

```text
password:
  attributes: contract::security::secret, contract::security::no_log
  value: <omitted>
```

Debug adapter is also the main verification tool for:

```text
visible attrs
invisible attrs
attribute modes
rejected policies
ignored attrs
```

---

### 8.3 Wire Adapter

Produces transport representation.

Examples:

```text
protobuf
binary protocol
JSON RPC payload
network snapshot batch
```

### REQUIRED

```text
field ids
schema compatibility
versioning support
```

### REQUIRED

```text
contract::security::encrypt()
```

### OPTIONAL

```text
contract::schema::type()
contract::check::* as bounds/limits
contract::unit::* as metadata
wire::*
proto::*
json::*
binary::*
```

### OUT OF SCOPE

```text
contract::security::no_log()
```

### Required Behavior

Wire adapters are not loggers.

`contract::security::no_log()` does not require omitting a field from a wire payload.

`contract::security::encrypt()` requires one of:

```text
field encryption
message/envelope encryption
transport encryption
approved external layer
rejection
```

---

### 8.4 Storage Adapter

Produces persistent representation.

Examples:

```text
ClickHouse
PostgreSQL
backup files
snapshots
local disk cache
```

### REQUIRED

```text
schema mapping
type mapping
field identity mapping
```

### REQUIRED

```text
contract::security::encrypt()
```

### OPTIONAL

```text
contract::schema::type()
contract::check::* as constraints/hints
contract::unit::* as comments/metadata
ch::*
storage hints
index hints
compression hints
```

### OUT OF SCOPE

```text
contract::security::no_log()
```

### Required Behavior

Storage adapters must not silently store raw fields that require encryption.

Valid outcomes:

```text
enforce by adapter encryption;
out_of_scope or reject when no approved external policy exists;
reject in strict mode.
```

---

### 8.5 Schema Adapter

Produces metadata and documentation.

Examples:

```text
schema export
documentation generation
reflection metadata
protobuf schema generation
OpenAPI-like metadata
```

### REQUIRED

```text
field ids
field metadata
comments
attributes
schema lifecycle attrs
```

### OPTIONAL

```text
security classification metadata
check constraints
unit metadata
```

### OUT OF SCOPE

```text
runtime enforcement
runtime encryption
runtime redaction
```

Schema adapters may display security attributes but do not provide runtime guarantees.

---

### 8.6 Config Adapter

Loads configuration into contracts.

Examples:

```text
YAML
JSON config
TOML
environment-expanded config
```

### REQUIRED

```text
field mapping
defaults
optional handling
validation hooks
error reporting
```

### OPTIONAL

```text
contract::doc::comment()
contract::check::* validation
security classification for config secrets
```

### Required Behavior

Config adapters should avoid logging raw values for fields marked `contract::security::secret()` or `contract::security::no_log()` in parse errors and diagnostics.

---

### 8.7 Validation Adapter

Validates values against contract declarations.

### REQUIRED

```text
contract::schema::type() where applicable
contract::check::* supported constraints
field presence/shape rules supported by the adapter
```

### OPTIONAL

```text
unit validation
security policy validation
```

### OUT OF SCOPE

```text
storage/wire encryption implementation
```

---

## 9. Capability Matrix

| Attribute | Log | Debug | Wire | Storage | Schema | Config | Validation |
|---|---|---|---|---|---|---|---|
| `contract::security::no_log` | enforce | enforce | out_of_scope | N/A | N/A | should avoid diagnostic leaks | N/A |
| `contract::security::secret` | enforce | enforce | ignore | hint | metadata only | should avoid diagnostic leaks | hint |
| `contract::security::sensitive` | hint | hint | ignore | hint | metadata only | hint | hint |
| `contract::security::encrypt` | out_of_scope | out_of_scope | enforce | enforce | metadata only | N/A | policy check only |
| `contract::schema::type` | display/ignore | display | hint | hint | enforce | hint | enforce if validating shape |
| `contract::schema::reserved` | N/A | display | enforce if schema-generating wire | N/A | enforce | N/A | enforce for schema validation |
| `contract::check::*` | display/ignore | display | hint | hint | expose | validate | enforce if supported |
| `contract::unit::*` | display/ignore | display | metadata/hint | metadata/hint | expose | N/A | hint |
| `contract::doc::comment` | display | display | N/A | N/A | enforce | display | N/A |
| `ch::*` | invisible | display | invisible | ClickHouse only | N/A | N/A | N/A |
| `proto::*` | invisible | display | Protobuf only | invisible | Protobuf schema only | N/A | N/A |

Only the **Wire** column reflects implemented behavior (as declared by the
`binary`/`compact` `adapter_traits`); the other columns are the planned
projection described in the status note above. `ch::*`/`proto::*` rows refer to
the planned projection vocabularies, which do not exist yet.

---

## 10. Debug and Capability Reporting

Debug and capability reporting should follow the same handling matrix used by
validation.

The callback entry contains the target, field name, declared and effective
field ids, BASE offset, typed attribute payload, resolution, adapter type, and
validation issue.

`error` produces `unsupported_attribute`.
`reject` produces `rejected_requirement`.
Both make the summary invalid. Invisible, ignore, display, hint, and
out-of-scope decisions remain visible in the report but are not validation
errors.

This is the main rule users should rely on.

---

## 11. Unsafe Modes

Unsafe mode rules are explicit.

If an adapter supports unsafe escape hatches, they must be visible in audit or
debug output and test-covered.

---

## 12. Adapter-Specific Hooks

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

## 13. Adapter Implementation Checklist

Before publishing an adapter, document:

```text
adapter type;
visible vocabularies;
baseline rules;
rule overrides;
handling matrix;
strict/permissive behavior;
security behavior;
policy outcomes;
fingerprint inputs;
debug report support;
unsafe modes;
known unsupported attrs.
```

---

## 14. Final Rule

```text
Adapters should be easy to write,
but their guarantees must be explicit.
```
