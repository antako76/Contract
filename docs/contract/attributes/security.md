# Security Attributes

Scope: practical security behavior for CONTRACT-aware adapters.

Related documents:

- `attributes.md`
- `adapter_scopes.md`
- `adapters.md`

Implementation note:

- the canonical `contract::security::*` declarations and `attr_traits` specializations
  live in [`include/contract/attributes/vocabularies/security.hpp`](../../../include/contract/attributes/vocabularies/security.hpp).

---

## 1. Security Model

CONTRACT is not a process-wide security framework.

Security attributes are declarative metadata.

Behavior is implemented by CONTRACT-aware adapters.

Security guarantees exist only when:

1. the adapter sees the security vocabulary;
2. the adapter recognizes the attribute;
3. validation succeeds;
4. the adapter enforces, ignores, displays, or rejects the declared behavior
   according to its rules.

Raw C++ code can still print or store a field directly if it bypasses CONTRACT.

---

## 2. P0 Security Vocabulary

```cpp
contract::security::sensitive()
contract::security::secret()
contract::security::no_log()
contract::security::encrypt()
```

`contract::security::redact()` is not part of the initial vocabulary.

Redaction is treated as an adapter policy choice for `sensitive()` or `secret()` fields.

---


## 3. Why contract::security::redact() Is Not in the Core Vocabulary

Earlier drafts included:

```cpp
contract::security::redact()
```

Reason:

```text
sensitive / secret / no_log / encrypt describe stable data protection intent.
redact / mask / hash / omit describe exposure strategies.
Exposure strategy belongs to adapter policy.
```

Examples:

```text
LogAdapter may render contract::security::sensitive() as <redacted>.
DebugAdapter may render contract::security::secret() as <omitted>.
AuditAdapter may show only metadata for contract::security::no_log().
```

This keeps the common security vocabulary small and avoids duplicating adapter policy as global contract metadata.

Projects may still add `contract::security::redact()` as an extension or P1 candidate if a stable cross-adapter need appears.

Adapter-specific outcomes, matrices, and strict/unsafe mode rules live in
`adapter_scopes.md`.

---

## 4. contract::security::sensitive()

Category: semantic attribute.

Meaning:

```text
field contains sensitive data;
field should not be exposed casually;
adapters may apply safer defaults.
```

Examples:

```text
email
phone
internal user id
tenant-specific identifier
business-sensitive identifier
```

Expected behavior:

```text
LogAdapter   -> may redact/hash/allow by policy
DebugAdapter -> may redact/hash/allow by policy
WireAdapter  -> optional policy use
Storage      -> optional policy use
Schema       -> may expose classification
```

---

## 5. contract::security::secret()

Category: semantic attribute with strong security meaning.

Meaning:

```text
field contains credential/token/key/password/session-cookie/bearer-like value.
```

Examples:

```text
password
API key
access token
refresh token
private credential
```

Intended behavior:

```text
An adapter that renders values to human-readable output should not output raw secret values.
```

Current adapter defaults differ and are opt-in per adapter:

- the console adapter redacts `secret`/`no_log`/`sensitive` by default;
- the JSON adapter defaults to `security_mode::ignore` (it prints raw values
  unless configured with `security_mode::omit`/`redact` via `json::options`).

So this is the recommended policy, not a guarantee enforced uniformly across
every adapter today.

Examples:

```text
api_key: "***"
api_key: <redacted>
api_key: <omitted>
```

`secret()` implies sensitive-level handling.

`secret()` does not automatically imply `encrypt()`.

---

## 6. contract::security::no_log()

Category: policy attribute.

Meaning:

```text
field must not appear in ordinary logs or log-like diagnostic output.
```

Required for:

```text
LogAdapter
DebugAdapter
ConfigAdapter diagnostics where applicable
```

Example:

```cpp
(password, 1001,
    contract::security::secret(),
    contract::security::no_log())
```

Expected log output:

```text
// password field absent
```

Allowed debug metadata output:

```text
password:
  attributes: contract::security::secret, contract::security::no_log
  value: <omitted>
```

`no_log()` is enforced only by CONTRACT-aware log/debug adapters.

It does not prevent direct manual printing outside CONTRACT.

---

## 7. contract::security::encrypt()

Category: policy attribute.

Meaning:

```text
field requires encryption in scopes where the raw value is persisted or transmitted.
```

Relevant scopes:

```text
storage
wire
```

Out of scope:

```text
log
debug
schema runtime enforcement
```

Valid outcomes:

```text
enforce
hint
reject
out_of_scope
```

Examples:

```text
BackupAdapter:
  contract::security::encrypt -> enforce by envelope encryption

WireAdapter:
  contract::security::encrypt -> enforce by simple message encryption or transport encryption

ClickHouseAdapter:
  contract::security::encrypt -> enforce via encrypted storage backend, otherwise reject

LogAdapter:
  contract::security::encrypt -> out_of_scope
```

Important distinction:

```text
contract::security::encrypt() expresses data protection intent.
It does not prescribe field-level encryption.
```

The adapter/profile decides whether encryption is field-level, message-level, transport-level, storage-layer, envelope-level, or rejected.

The binary adapter currently enforces `contract::security::encrypt()` using a
simple streaming XOR transform keyed by adapter options; it is a transport
mechanism, not a full cryptographic policy engine.

---

## 8. Security Outcome Summary

The practical security matrix, strict/unsafe mode rules, and adapter defaults
are defined in `adapter_scopes.md`.

This page keeps the vocabulary and meaning of the security attrs:

```text
contract::security::sensitive() classifies.
contract::security::secret() forbids raw human-readable exposure.
contract::security::no_log() omits from logs/debug values.
contract::security::encrypt() requires encryption for persistence/transmission scopes.
```
