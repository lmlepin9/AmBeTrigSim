# PMT Response

This directory contains the C++ post-processing step for PMT optical-photon hits.

The Geant4 simulation writes one row per detected optical photon into `OutPut.root`:

```text
tree: ph
branches: evt, x, y, z, t, e, process
```

The branch layout follows the reference implementation in `/home/lmlepin/BGOg4Sim`:

- `evt`: Geant4 event id.
- `x`, `y`, `z`: photon hit position at the PMT, in cm.
- `t`: photon global time, in ns.
- `e`: photon kinetic energy, in eV.
- `process`: `0` for scintillation, `1` for Cerenkov, `2` for other/unknown.

## Build

From the repository root:

```bash
setup_env AmBe-sim
cmake -S . -B build
cmake --build build
```

This builds both:

```text
build/ambe_sim
build/pmt_response
```

## Run

First produce PMT optical-photon hits:

```bash
./build/ambe_sim build/macros/run.mac
```

By default this writes:

```text
OutPut.root
```

Then build the PMT electronics response:

```bash
./build/pmt_response OutPut.root waveforms.root
```

If no arguments are given, `pmt_response` reads `OutPut.root` and writes `waveforms.root`.

## Output

The waveform file contains:

- `events`: summary tree with `event`, `triggered`, `n_cherenkov`, and `n_scintillation`.
- `WVF_<event>_<timestamp>`: PMT waveform histogram.
- `SCI_<event>_<timestamp>`: accepted scintillation photon time histogram.
- `CHE_<event>_<timestamp>`: accepted Cerenkov photon time histogram.
- `BRF_<event>_<timestamp>`: triggered waveform aligned to the trigger time, written only for triggered events.

The response model mirrors the Python script in `BGOg4Sim/scripts/photon_histogram_builder.py`: QE filtering, gain smearing, single-photon response convolution, Gaussian electronic noise, and threshold triggering.
