# Subsystem reading template

## Identity and scope

- Native framework and owning image:
- OS version/build, architecture and UUID:
- Canonical image hash and scratch database:
- Native functions/methods, addresses and original source paths when recovered:
- Confidence per conclusion: verified / inferred / unread:
- What remains unread and therefore unbuilt:

## Native implementation

Record layouts with offsets and sizes, field ownership, language/compilation
boundaries, registration, constants, branch conditions, call order, callbacks,
error/exception paths, thread state and object lifetime. Keep the native function
boundaries. Separate implementation evidence from confirming measurements.

## Dependencies and naming

Record the owning image and ordinary/weak/reexported/soft-link boundary for every
dependency; whether it exchanges raw values or CF/NS objects; its restoration
prerequisites; native versus prefixed declarations; and any process-shared names.
State the precise approved divergence for coexistence or a native defect.

## iOS Device comparison

Give the independently pinned iOS Device image, function/address and confidence.
Record code/layout/constants/ownership differences or explicitly state unread.
Do not infer equivalence from the simulator or from a common source version.

## Verification and limits

List isolated/differential/internal cases, independent fixture construction,
owning-image guards, exact comparison policy and reproducible commands. State
setup failures separately from compatibility failures. Record memory/threading
checks where applicable and every untested boundary. Add complete definitions
to `Configuration/Restoration.json` only after their dependency group is restored.
