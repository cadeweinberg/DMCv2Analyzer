# DMC Saleae Analyzer

Custom Saleae Logic 2 Low Level Analyzer for the Dragonframe DMC v2 communications protocol.

The analyzer decodes the binary DMC packet format used by DMC-16/DMC+ controllers and compatible devices. It accepts asynchronous serial input, reconstructs DMC packets, validates their Fletcher-16 checksums, and exposes decoded fields through Saleae’s FrameV2 data table.

## Features

- Standard asynchronous UART decoding, LSB-first.
- Default baud rate: `115200` bits/s.
- Configurable input channel and baud rate.
- DMC packet framing using the `DF` marker.
- Little-endian ID, type, length, and payload decoding.
- Fletcher-16 checksum validation.
- Decoding for DMC motor, DMX, GIO, real-time, and virtual messages.
- ACK and response-code decoding.
- Error reporting for checksum, framing, truncation, and malformed packets.
- FrameV2 fields for packet type, ID, length, status, direction, and decoded values.
- Optional `Show Serial Bytes` diagnostic setting for displaying individual UART bytes.
- CSV/text export of packet metadata and decoded fields.

The protocol specification is in [docs/DMC-Protocol-2024-02-13.txt](docs/DMC-Protocol-2024-02-13.txt). The checked-in Saleae API notes are in [docs/Analyzer_API.md](docs/Analyzer_API.md).

## Using the analyzer

1. Build the analyzer or obtain a platform-specific plugin binary.
2. Configure Saleae Logic 2 to use its custom analyzer/developer directory.
3. Copy the built plugin into the analyzer directory.
4. Restart Logic 2 and add the `DMC` analyzer to the captured serial channel.
5. Set the baud rate to the rate used by the device. The default is `115200`.

### Output

Parsed packets appear in the analyzer data table as `DMC` FrameV2 entries. Packet fields include the message name, ID, length, checksum status, direction classification, and message-specific decoded values.

Enable `Show Serial Bytes` when diagnosing a capture. This adds `serial_byte` entries containing the sampled byte and framing status. It is disabled by default so the table contains only parsed DMC packets.

The packet bubbles and CSV export provide the message type, ID, length, status, and decoded fields. Raw bytes remain available through the optional `serial_byte` diagnostic entries.

## Building

# A note on downloading the MacOS Analyzer builds

This section only applies to downloaded pre-built protocol analyzer binaries on MacOS. If you build the protocol analyzer locally, or acquire it in a different way, this section does not apply.

Any time you download a binary from the internet on a Mac, wether it be an application or a shared library, MacOS will flag that binary for "quarantine". MacOS then requires any quarantined binary to be signed and notarized through the MacOS developer program before it will allow that binary to be executed.

Because of this, when you download a pre-compiled protocol analyzer plugin from the internet and try to load it in the Saleae software, you will most likely see an error message like this:

> "libSimpleSerialAnalyzer.so" cannot be opened because th developer cannot be verified.

Signing and notarizing of open source software can be rare, because it requires an active paid subscription to the MacOS developer program, and the signing and notarization process frequently changes and becomes more restrictive, requiring frequent updates to the build process.

The quickest solution to this is to simply remove the quarantine flag added by MacOS using a simple command line tool.

Note - the purpose of code signing and notarization is to help end users be sure that the binary they downloaded did indeed come from the original publisher and hasn't been modified. Saleae does not create, control, or review 3rd party analyzer plugins available on the internet, and thus you must trust the original author and the website where you are downloading the plugin. (This applies to all software you've ever downloaded, essentially.)

To remove the quarantine flag on MacOS, you can simply open the terminal and navigate to the directory containing the downloaded shared library.

This will show what flags are present on the binary:

```sh
xattr libSimpleSerialAnalyzer.so
# example output:
# com.apple.macl
# com.apple.quarantine
```

This command will remove the quarantine flag:

```sh
xattr -r -d com.apple.quarantine libSimpleSerialAnalyzer.so
```

To verify the flag was removed, run the first command again and verify the quarantine flag is no longer present.

## Building your Analyzer

CMake and a C++ compiler are required. Instructions for installing dependencies can be found here:
https://github.com/saleae/SampleAnalyzer

The fastest way to use this analyzer is to download a release from github. Local building should only be needed for making your own changes to the analyzer source.

### Windows

```bat
mkdir build
cd build
cmake .. -A x64
cmake --build .
:: built analyzer will be located at SampleAnalyzer\build\Analyzers\Debug\SimpleSerialAnalyzer.dll
```

### MacOS

```bash
mkdir build
cd build
cmake ..
cmake --build .
# built analyzer will be located at SampleAnalyzer/build/Analyzers/libSimpleSerialAnalyzer.so
```

### Linux

```bash
mkdir build
cd build
cmake ..
cmake --build .
# built analyzer will be located at SampleAnalyzer/build/Analyzers/libSimpleSerialAnalyzer.so
```

## Tests

The standalone protocol parser tests cover checksum calculation, packet decoding, malformed input recovery, truncation, and framing errors.

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

If the SDK is already populated locally and CMake attempts to update it without network access, configure with:

```bash
cmake -S . -B build -DFETCHCONTENT_FULLY_DISCONNECTED=ON
```

## Debugging

For useful debugging, start with an existing capture rather than recording while paused in a debugger. Add the analyzer to the capture, then attach the native debugger to the Logic 2 process before rerunning analysis.

On Windows, close Saleae Logic before rebuilding or replacing the plugin. Logic loads the analyzer DLL and can lock it, causing the linker to report that it cannot open `DMCAnalyzer.dll`.


## Repository layout

- `src/DMCAnalyzer.cpp` — Saleae analyzer lifecycle and UART sampling.
- `src/DMCProtocol.cpp` — DMC packet assembly, checksum validation, and field decoding.
- `src/DMCAnalyzerResults.cpp` — FrameV2, waveform, export, and diagnostic output.
- `src/DMCSimulationDataGenerator.cpp` — generated DMC traffic for analyzer simulation.
- `tests/DMCProtocolTests.cpp` — standalone parser tests.
- `docs/DMC-Protocol-2024-02-13.txt` — DMC v2 protocol specification.

## License

See [LICENSE](LICENSE).
