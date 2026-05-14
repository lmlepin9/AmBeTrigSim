# AmBeTrigSim

Minimal Geant4 simulation for the AmBe housing GDML geometry.

## What it does

- Loads the repo-local `AmBeHousing.gdml` by default.
- The GDML includes the larger air world and direct active BGO/Teflon placements used by the simulation.
- Uses the `FTFP_BERT_HP` physics list.
- Registers Geant4 optical physics; the BGO scintillation material properties are stored in `AmBeHousing.gdml`.
- Uses the GDML material assignments for BGO, Teflon, stainless steel, and polyurethane foam; the Teflon optical skin is also defined in GDML.
- Records optical photons entering `PMTLV` into a ROOT tree named `ph`, matching the `BGOg4Sim` branch layout.
- Includes a separate C++ PMT electronics-response builder in `pmt_response/`.
- The default visualization source is a `4.4 MeV` gamma from `(0, 0, -168.64) mm` pointed off-axis into the BGO crystal, avoiding the central bore.
- With `--ambe`, the visualization source uses `/home/lmlepin/Check_AmBeSimulation/AmBe-EmergingParticles-N0.root`.
- In ROOT-source mode, it generates all particles stored in one `EmergingParticles` tree entry per Geant4 event, then advances to the next entry.
- Places the default source at `(0, 0, -168.64) mm`, the world-coordinate position of the GDML `logicMarker`, which is at the center of the source capsule/case.
- Starts an interactive visualization session when run without arguments.

## Build

```bash
setup_env AmBe-sim
cmake -S . -B build
cmake --build build
```

This builds both the Geant4 simulation and the PMT response executable:

```text
build/ambe_sim
build/pmt_response
```

## Run interactively

```bash
./build/ambe_sim
```

The interactive startup macro draws the GDML geometry and generates one default gamma so its trajectory is visible.

To use the ROOT AmBe emerging-particle source in visualization mode:

```bash
./build/ambe_sim --ambe
```

## Run a macro

```bash
./build/ambe_sim build/macros/run.mac
```

To load a different GDML file:

```bash
./build/ambe_sim build/macros/run.mac /path/to/geometry.gdml
```

Useful source commands:

```text
/AmBe/source/useRootFile true
/AmBe/source/rootFile /home/lmlepin/Check_AmBeSimulation/AmBe-EmergingParticles-N0.root
/AmBe/source/rootVertexScale 1
/AmBe/source/rootVertexOffset 0 0 -168.64 mm
/AmBe/source/setRootEntry 0
/AmBe/source/particle gamma
/AmBe/source/energy 4.4 MeV
/AmBe/source/position 0 0 -168.64 mm
/AmBe/source/direction 10 0 41.73
/AmBe/source/update
```

The ROOT vertex coordinates are treated as millimeters relative to the AmBe source center by default, so they are offset by `(0, 0, -168.64) mm`.

## PMT optical-photon hits and response

During a run, optical photons entering the GDML `PMTLV` volume are written to `OutPut.root` in a tree named `ph` with branches:

```text
evt, x, y, z, t, e, process
```

To convert those hits into PMT waveforms:

```bash
./build/pmt_response OutPut.root waveforms.root
```

See `pmt_response/README.md` for the response model and output details.
