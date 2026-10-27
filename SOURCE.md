# Korean Dangi Calendar — source copy record

This repository now contains its own implementation of provider `dangi`, its required supporting C code and headers, a one-provider build, and copy-regression fixtures.

## Origin and scope

- **Source:** [Libraries/Common 1.19.78](https://github.com/Infiltrator-Projects/Libraries/tree/8d60662114e3052380f6f88ad3cacd9732ae7131).
- **Source revision:** `8d60662114e3052380f6f88ad3cacd9732ae7131`.
- **Original entry:** `resources/temporal/src/calendars/native-calendar.c`.
- **Local entry:** `src/provider.c`.
- **Output:** `Korean Dangi Calendar.so`.
- **Native resource path:** `/System/Resources/Calendars/Korean Dangi Calendar.so`.
- **Original Common code:** retained unchanged. This is a copy, not a removal or package-ownership transition.

Combined source files were selected for this provider and split by keeping its existing operations, data and required helper definitions. Inactive provider branches and unreachable definitions were removed. Shared arithmetic, formatting, localization and runtime-bridge helpers are intentionally copied into every repository that needs them. The local build uses only this repository's source and platform dependencies; it does not fetch or link a Common checkout/archive.

Public Common ABI headers retain compatibility declarations. Those declarations do not install or implement the other providers. The built module exports only its metadata entry and its `calendar` operation-table entry. Existing notices and the original GPL license are retained.

`SOURCE-MANIFEST.json` records original paths, original/copy hashes, selected definitions and the build configuration. Its hashes identify this initial extraction; later local edits legitimately change the copied hashes.

## Build and check

Use a C11 compiler, GNU Make, Python 3 and pkg-config. Development packages required by this copy: glib-2.0, icu-i18n.

```sh
make
make check
```

The build directly creates `build/Korean Dangi Calendar.so`. ICU-backed providers retain the copied runtime ABI bridge and use the host's ICU/CLDR data. No installer or package-ownership change is included in this copy.

## Recorded checks

- Independent build with strict C11 warnings treated as errors.
- Metadata, stable ID, canonical name, callback ABI and export checks.
- **883** observations matched the original Common-built provider, including applicable conversion, formatting, navigation, timing and invalid-input cases.
- **84** provider-specific regression fixtures are retained under `tests/`.
- Common's existing calendar and clock module suites passed against the complete set of 54 independently built copies: **6 calendar tests and 16 clock tests**.

See `VALIDATION-RESULTS.json` for the tested host and exact source revision. This verifies the copy against the existing implementation; it is not an additional historical-fidelity audit or a cross-platform release qualification.

## Owned lunar chronology requalification

Provider-owned Korean lunisolar year data for 1900–2100, independently checked against KASI-based dates from 1900-01-31 through 2050-11-18; later owned dates are predictions. ICU supplies locale text and the retained projection outside that year table. The current implementation and test counts are described in `docs/FORENSIC-AUDIT.md`. Initial extraction hashes and copy-comparison results remain historical provenance.
