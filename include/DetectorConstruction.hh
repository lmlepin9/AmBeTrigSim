#ifndef AMBE_DETECTOR_CONSTRUCTION_HH
#define AMBE_DETECTOR_CONSTRUCTION_HH

#include "G4String.hh"
#include "G4VUserDetectorConstruction.hh"

class G4VPhysicalVolume;

class DetectorConstruction final : public G4VUserDetectorConstruction {
public:
  explicit DetectorConstruction(G4String gdmlPath);
  ~DetectorConstruction() override = default;

  G4VPhysicalVolume* Construct() override;

private:
  G4String fGdmlPath;
};

#endif
