# Public P6 regression corpus

The seven unmodified XER files and `LICENSE-MPXJ.txt` come from
[MPXJ](https://github.com/joniles/mpxj) at the revision pinned in `manifest.json`.
The upstream project distributes them under LGPL-2.1. They are test inputs,
not installed application assets. The manifest records exact source URLs,
encodings, hashes and expected JOBS import dispositions.

The two larger files described in `external-manifest.json` have no declared
upstream license. Their bytes are **not** vendored or included in artifacts.
`scripts/fetch-p6-external-samples.py --output build/p6-external` downloads the
pinned originals for local/CI analysis; only the resulting comparison matrix
and logs should be retained in published evidence.

`scripts/verify-p6-corpus.py` uses MPXJ 16.10.0 / JPype 1.6.0 independently of
the JOBS parser. It compares source identities/counts, planned duration,
effective weekly calendars and the supported exported projection. Rejected
inputs retain their diagnostic codes. Multiple rate-period assignments and
unsupported currencies are not silently merged or relabelled.

Upstream provenance and P6-format headers are recorded; original customer/P6
execution is not independently attested. Passing this corpus does not prove
Oracle P6 recalculation equivalence or complete XER compatibility.
