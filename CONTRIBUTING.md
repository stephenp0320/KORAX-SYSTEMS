# Contributing to Korax

Thanks for your interest. Korax is early-stage; issues and pull requests are
welcome.

## Developer Certificate of Origin (DCO)

Contributions are accepted under the [Developer Certificate of Origin](https://developercertificate.org/).
Sign off every commit to certify you wrote the code or have the right to submit
it under the project's licence:

```sh
git commit -s -m "raven: fix location timestamp scaling"
```

The `-s` adds a `Signed-off-by: Your Name <you@example.com>` line. Commits
without a sign-off cannot be merged.

## Ground rules

- **Build clean.** `cmake -S . -B build -DKORAX_WERROR=ON && cmake --build build`
  must pass with no warnings under both GCC and Clang.
- **Tests pass.** `ctest --test-dir build` must be green. New behaviour needs a
  test; the decoder is held to the ODID wire format, not to itself.
- **Style.** C11, 4-space indent, 100-column soft limit. A `.clang-format` and
  `.editorconfig` are provided — run `clang-format` before committing.
- **Small, focused commits** with a `module: summary` subject line.

## Security-sensitive code

Every byte entering the decoder is attacker-controllable radio input. Decode
functions must be total over arbitrary input (no crashes, no undefined
behaviour) and are expected to be fuzzed. See [SECURITY.md](SECURITY.md).
