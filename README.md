# AmBeTrigSim

Minimal Geant4 simulation for the AmBe housing GDML geometry.

## What it does

- Loads the repo-local `AmBeHousing.gdml` by default.
- Places the GDML geometry inside a larger invisible air world so particles can emerge from the housing and remain visible.
- Uses the `FTFP_BERT_HP` physics list.
- Registers Geant4 optical physics and assigns BGO scintillation properties to the GDML `BGO_crystal` volume.
- Assigns a reflective Teflon optical skin to the GDML `BGO_teflon_wrapping` volume.
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
