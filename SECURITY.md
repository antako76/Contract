# Security Policy

If you believe you have found a security issue in CONTRACT, please report it
privately instead of opening a public issue.

## Reporting

Please report security issues privately.

Maintainer: Ilya Korolev

Contact:

- GitHub: [`antako76`](https://github.com/antako76)

Include:

- a short description of the issue;
- the affected file or API surface;
- a minimal reproduction if possible;
- the impact you expect;
- whether the issue affects parsing, serialization, or adapter behavior.

Examples of security issues include:

- memory corruption;
- undefined behavior triggered by untrusted input;
- out-of-bounds reads or writes;
- integer overflow leading to unsafe behavior;
- denial of service through malformed input;
- unexpected data disclosure;
- adapter-specific parsing vulnerabilities.

## What To Avoid

- Do not include secrets, credentials, or production data in the report.
- Please allow reasonable time for investigation and remediation before public
  disclosure.

Security fixes may be released before full public disclosure.
Compatibility may be broken when necessary to address a security issue.

## Scope

The primary security scope includes:

- public headers under [`include/contract/`](include/contract/);
- serialization and deserialization adapters;
- binary, console, and other supported format handlers;
- code generation and reflection mechanisms.

Documentation, examples, and tests are generally out of scope unless they
demonstrate a vulnerability in the library itself.

## Response Expectations

The maintainer will acknowledge confirmed reports as soon as practical and
will communicate the planned remediation path when possible.
