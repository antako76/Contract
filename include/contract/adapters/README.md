# contract/adapters

Public optional adapters built on top of the core contract traversal.

Expected first adapters:

- console audit/debug dump;
- JSON event builder/serializer;
- compact log/debug dump;
- schema dump;
- id/name validation helpers;
- simple binary writer/reader prototype.

Adapters must not add behavior to user data classes. They consume contract
metadata through the public core API.

Shared adapter traits live in `contract::adapters::base`:

- type normalization;
- dependent `static_assert` helpers;
- contract-definition detection.

Reusable formatting pieces live next to adapters in
`contract::adapters::debug`:

- readable type names;
- field metadata comments;
- string escaping and truncation;
- byte previews.

Adapters own presentation and runtime policy:

- tree vs single-line output;
- comments and field metadata visibility;
- container limits;
- redaction;
- color;
- deterministic formatting for golden tests.

The console adapter is an audit/debug view, not a serialization format. It
prints values as a YAML-style tree and keeps contract metadata next to fields in
comments.

The console adapter follows the same split as binary:

- `contract/adapters/console.hpp` is the lean core writer and primitive codecs;
- `contract/adapters/console/all.hpp` pulls in the full container codec set.

The JSON adapter currently starts as a lean builder in
`contract/adapters/json.hpp`, with `contract/adapters/json/all.hpp` as the full
umbrella include.

The YAML adapter is a strict streaming config reader in
`contract/adapters/yaml.hpp`, with `contract/adapters/yaml/all.hpp` as the full
umbrella include.
