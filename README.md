
```markdown
# BEJ Minimal Decoder (C, GCC)

A minimal BEJ decoder (DSP0218) for the sample task:

* Reads the binary **schema dictionary** (DMTF/Redfish, Table 31).
* Decodes a **bejEncoding** stream: a root `Set` and nested `S–F–L–V` tuples.
* Supports **Set**, **Int**, **String** (and, pragmatically, **Array** of simple types and **Enum** → string).
* **Annotation dictionary** is ignored: annotations are detected (`S & 1`) and cleanly skipped.

## Project Layout

src/
bej.h # Public API (types, constants, prototypes)
bej_reader.{c,h} # Byte reader + nnint
bej_json.{c,h} # Simple pretty JSON writer
bej_dict.{c,h} # Dictionary parser (Table 31)
bej_decode.{c,h} # BEJ decoder (bejEncoding + SFLV) bound to the schema dictionary
main.c # CLI: file loading, decoder invocation`
````
## Build Instructions

### Unix (Linux/macOS)
````
# 1) configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 2) build
cmake --build build -j

# 3) run
./build/bej_tool -s Memory_v1.bin -a annotation.bin -b example.bin -o out.json
````

### Windows (MinGW-w64)

```
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc ^
      -DBUILD_TESTS=OFF -DBUILD_MIN_C_TESTS=ON
cmake --build build -j
```

### Tests (C-only)

```
ctest --test-dir build --output-on-failure
```

## Usage

```
bej_tool -s <schema.bin> -a <annotation.bin> -b <data.bej> -o <out.json>
```

Arguments:

* `-s <schema.bin>` – schema dictionary (e.g., `Memory_v1.bin`).
* `-a <annotation.bin>` – annotation dictionary file (must exist; content ignored).
* `-b <data.bej>` – BEJ stream (e.g., `example.bin` produced by the reference Python script).
* `-o <out.json>` – output JSON path.

## Example

```
./bej_tool -s Memory_v1.bin -a annotation.bin -b example.bin -o out.json
```

### Expected JSON (demo set)

```json
{
  "CapacityMiB": 65536,
  "DataWidthBits": 64,
  "AllowedSpeedsMHz": [2400, 3200],
  "ErrorCorrection": "NoECC",
  "MemoryLocation": { "Channel": 0, "Slot": 0 }
}
```

