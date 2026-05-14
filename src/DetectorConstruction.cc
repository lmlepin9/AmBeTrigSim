#include "DetectorConstruction.hh"

#include "G4Colour.hh"
#include "G4GDMLParser.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VisAttributes.hh"
#include "G4ios.hh"

#include <utility>

namespace {
void ApplyVisualizationStyle(G4LogicalVolume* logical)
{
  if (!logical) {
    return;
  }

  const auto name = logical->GetName();
  const auto* material = logical->GetMaterial();
  const auto materialName = material ? material->GetName() : G4String();
  G4Colour colour(0.35, 0.65, 1.0, 0.18);

  if (name == "World") {
    logical->SetVisAttributes(G4VisAttributes::GetInvisible());
  } else {
    if (name.find("logicMarker") != std::string::npos) {
      colour = G4Colour(1.0, 0.0, 0.0, 1.0);
    } else if (name.find("BGO_crystal") != std::string::npos) {
      colour = G4Colour(0.55, 0.55, 0.60, 0.35);
    } else if (name.find("BGO_teflon_wrapping") != std::string::npos) {
      colour = G4Colour(0.85, 0.85, 1.0, 0.25);
    } else if (name.find("stainless_steel") != std::string::npos) {
      colour = G4Colour(0.62, 0.64, 0.66, 0.35);
    } else if (name.find("black_foam") != std::string::npos) {
      colour = G4Colour(0.03, 0.03, 0.035, 0.28);
    } else if (name.find("PMTLV") != std::string::npos) {
      colour = G4Colour(0.95, 0.25, 0.18, 0.45);
    } else if (materialName.find("WhitePVC") != std::string::npos) {
      colour = G4Colour(0.96, 0.96, 0.90, 0.28);
    } else if (name.find("capsule") != std::string::npos ||
               name.find("source") != std::string::npos) {
      colour = G4Colour(0.0, 1.0, 0.0, 0.35);
    }

    G4VisAttributes attributes(colour);
    attributes.SetVisibility(true);
    attributes.SetForceWireframe(true);
    attributes.SetForceAuxEdgeVisible(true);
    logical->SetVisAttributes(attributes);
  }

  for (auto i = 0; i < logical->GetNoDaughters(); ++i) {
    ApplyVisualizationStyle(logical->GetDaughter(i)->GetLogicalVolume());
  }
}
}

DetectorConstruction::DetectorConstruction(G4String gdmlPath)
  : fGdmlPath(std::move(gdmlPath))
{}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
  G4GDMLParser parser;
  parser.Read(fGdmlPath, false);

  auto* gdmlWorld = parser.GetWorldVolume();
  if (!gdmlWorld) {
    G4Exception("DetectorConstruction::Construct", "AmBeTrigSim001",
                FatalException, "GDML parser did not return a world volume.");
  }

  G4cout << "Loaded detector geometry from " << fGdmlPath << G4endl;
  ApplyVisualizationStyle(gdmlWorld->GetLogicalVolume());
  return gdmlWorld;
}
