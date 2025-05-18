# djson v3.0

## Dumb and lightweight JSON parsing library.

[![Build Status](https://github.com/karnkaul/djson/actions/workflows/ci.yml/badge.svg)](https://github.com/karnkaul/djson/actions/workflows/ci.yml)

## Features

- Copiable `Json` objects
- Default construction (representing `null`) is "free"
- `as_string_view()`
- Heterogenous arrays
- Escaped text (`\\`, `\"`, `\b`, `\t`, `\n`)
- Implicit construction for nulls, booleans, numbers, strings
- Serialization, pretty-print (default)
- Customization points for `from_json` and `to_json`
- Build tree from scratch

### Limitations

- Escaped unicode (`\u1F604`) not currently supported

## Documentation

Documentation (with examples) is hosted [here](https://karnkaul.github.io/djson/).

## Contributing

Pull/merge requests are welcome.

**[Original repository](https://github.com/karnkaul/djson)**
