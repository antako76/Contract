# Attributes - Design Vision (Not Yet Implemented)

This page collects the parts of the attribute model that are **designed but not
yet implemented** in the current code. They describe the intended direction, not
behavior you can rely on today. For what is actually implemented, see
[`attributes.md`](attributes.md).

Status at a glance:

- **Implemented today:** the five common vocabularies (`schema`, `check`,
  `unit`, `security`, `doc`), the visibility / attribute-rules resolution model,
  per-adapter `adapter_traits`, contract- and field-level `ATTRS(...)`, core
  validation, and adapter-side enforcement in the `binary`/`compact` wire
  adapters (`check::*` guards, `security::encrypt`).
- **Not yet implemented (this page):** adapter projection vocabularies
  (`proto`/`ch`/`log`/`wire`), fingerprints, validation modes, the debug/audit
  dump surface, and the future vocabulary candidates. Adapter consumption of
  `schema::reserved`/`deprecated`/`alias` is also not implemented (the
  attributes exist and are stored, but no adapter acts on them yet).

---

## Adapter Projection Vocabularies

### proto

```cpp
proto::type(proto::type_id::fixed64)
proto::type(proto::type_id::bytes)
proto::type(proto::type_id::string)
proto::encoding(proto::encoding_id::packed)
proto::name("path_hash")
```

Rules:

```text
field id is protobuf tag number by default;
contract::schema::type(...) helps type inference;
proto::type(...) is explicit override;
proto adapter owns proto-specific validation.
```

### ch

```cpp
ch::type("DateTime")
ch::type("LowCardinality(String)")
ch::column("bucket_start")
ch::low_cardinality()
```

Rules:

```text
ClickHouse type string is acceptable because CH type system is rich and extensible;
ch::type is a concrete projection override;
schema/check/unit/security remain common attrs and may affect validation/audit/policy.
```

### log

P0 log vocabulary should stay small.

```cpp
log::name("tenant")
```

Security behavior should use common `contract::security::*` attributes.

Do not duplicate meanings as `log::redact` or `log::no_log`.

### wire

```cpp
wire::omit()
wire::message_encrypted()
```

`contract::security::encrypt()` is the common requirement.

`wire::*` attributes may describe concrete transport/message projection details.

---

## Fingerprints

There is not one universal fingerprint.

Adapters may need different fingerprints.

Examples:

```text
core schema fingerprint:
  field ids/names/types;
  BASE effective ids;
  reserved ids/names/ranges;
  explicit contract::schema::type attrs.

protobuf schema fingerprint:
  field ids;
  proto field names;
  effective proto types;
  proto encodings;
  reserved/deprecated.

ClickHouse schema fingerprint:
  field path;
  column name;
  ClickHouse type;
  nullable/default policy;
  relevant schema/security/storage attrs.

wire fingerprint:
  field ids/order;
  wire codec/projection;
  message/field omit/encryption policy;
  schema version.
```

Rule:

```text
Adapter documentation must state which attrs enter its fingerprint.
```

---

## Validation Modes

Adapters may expose multiple validation modes.

### Strict Mode

```text
Unknown visible attrs may be errors.
Unsupported visible policy attrs are errors.
Security policy gaps are errors.
Projection ambiguity is an error.
```

Strict mode is recommended for:

```text
production;
CI;
schema generation;
storage adapters;
wire adapters.
```

### Permissive Mode

```text
Unknown visible attrs may be displayed or ignored.
Hints may be ignored.
Semantic attrs may be displayed only.
```

Rules:

```text
Permissive mode must not silently violate security policy.
```

### Unsafe Mode

Unsafe mode is an explicit escape hatch.

Examples:

```text
raw secret output;
unencrypted storage despite contract::security::encrypt();
unenforced security policy.
```

Rules:

```text
Unsafe mode must be explicit.
Unsafe mode must be visible in audit output.
Unsafe mode should not be enabled in production profiles.
```

---

## Debug and Audit

Audit output must prevent false expectations.

It should show both declarations and effective adapter guarantees.

Default audit view:

```yaml
AuthEvent:
  user_email: "<redacted>"       # #1 string, sensitive, max_length=320
  access_token: <omitted>        # #2 string, secret, no_log, encrypt
  duration_ns: 991827364         # #3 u64, unit=ns
```

Target-adapter audit view:

```cpp
console_audit::dump(obj, out);
console_audit::dump_for<log_adapter_traits>(obj, out);
console_audit::dump_for<clickhouse_adapter_traits>(obj, out);
contract::print_adapter_support<AuthEvent, log_adapter_traits>(out);
```

Example:

```yaml
AuthEvent: # target_adapter=log
  user_email: "<redacted>"
    # contract::schema::type(string)       [log: display]
    # contract::check::max_length(320)     [log: ignore]
    # contract::security::sensitive()      [log: enforce: redact_by_policy]

  access_token: <omitted>
    # contract::schema::type(string)       [log: display]
    # contract::security::secret()         [log: enforce: never_raw]
    # contract::security::no_log()         [log: enforce: omit]
    # contract::security::encrypt()        [log: out_of_scope]
```

For storage:

```yaml
AuthEvent: # target_adapter=clickhouse
  access_token:
    # contract::schema::type(string)       [clickhouse: hint]
    # contract::security::secret()         [clickhouse: error in strict mode unless storage policy exists]
    # contract::security::no_log()         [clickhouse: out_of_scope]
    # contract::security::encrypt()        [clickhouse: enforce or error]
```

---

## Future Vocabulary Candidates

The following vocabularies are intentionally excluded from P0.

### format

Examples:

```cpp
format::uuid()
format::email()
format::uri()
format::date_time()
format::base64()
format::hex()
```

Purpose:

```text
Logical representation hints.
Validation helpers.
Schema generation metadata.
```

### repr

Examples:

```cpp
repr::hex()
repr::base64()
repr::compressed()
```

Purpose:

```text
Logical representation metadata.
```

### analytics

Examples:

```cpp
analytics::dimension()
analytics::measure()
analytics::counter()
```

Purpose:

```text
Reporting and analytics metadata.
```

### cardinality

Examples:

```cpp
cardinality::low()
cardinality::high()
```

Purpose:

```text
Storage and indexing hints.
```

These vocabularies remain P1 candidates until practical adapter requirements emerge.

---

