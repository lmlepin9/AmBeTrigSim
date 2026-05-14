#include "DetectorConstruction.hh"

#include "G4Box.hh"
#include "G4Colour.hh"
#include "G4GDMLParser.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4NistManager.hh"
#include "G4OpticalSurface.hh"
#include "G4PVPlacement.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VisAttributes.hh"
#include "G4ios.hh"

#include <utility>

namespace {
void ConfigureOpticalMaterials()
{
  auto* nist = G4NistManager::Instance();
  auto* bgo = nist->FindOrBuildMaterial("G4_BGO");
  auto* teflon = nist->FindOrBuildMaterial("G4_TEFLON");
  auto* air = nist->FindOrBuildMaterial("G4_AIR");

  constexpr G4int n = 13;
  G4double energy[n] = {
    1.771 * eV, 1.922 * eV, 2.084 * eV, 2.275 * eV, 2.505 * eV,
    2.755 * eV, 2.987 * eV, 3.220 * eV, 3.492 * eV, 3.874 * eV,
    4.350 * eV, 5.060 * eV, 6.199 * eV
  };

  // Sampled from the BGOg4Sim BGO spectrum; normalized shape is enough for Geant4.
  G4double bgoScint[n] = {
    0.00, 0.08, 0.28, 0.62, 0.95, 0.87, 0.42,
    0.08, 0.00, 0.00, 0.00, 0.00, 0.00
  };
  G4double bgoRIndex[n] = {
    2.084, 2.095, 2.107, 2.124, 2.146, 2.174, 2.216,
    2.241, 2.289, 2.387, 2.473, 2.589, 2.718
  };
  G4double bgoAbs[n] = {
    10.0 * cm, 9.9 * cm, 9.7 * cm, 9.6 * cm, 9.4 * cm, 9.1 * cm,
    8.6 * cm, 8.0 * cm, 7.3 * cm, 2.6 * cm, 1.0e-6 * cm,
    1.0e-6 * cm, 1.0e-6 * cm
  };

  auto* bgoMpt = new G4MaterialPropertiesTable();
  bgoMpt->AddProperty("SCINTILLATIONCOMPONENT1", energy, bgoScint, n);
  bgoMpt->AddProperty("SCINTILLATIONCOMPONENT2", energy, bgoScint, n);
  bgoMpt->AddProperty("RINDEX", energy, bgoRIndex, n);
  bgoMpt->AddProperty("ABSLENGTH", energy, bgoAbs, n);
  bgoMpt->AddConstProperty("SCINTILLATIONYIELD", 10000. / MeV);
  bgoMpt->AddConstProperty("RESOLUTIONSCALE", 2.0);
  bgoMpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 1.0 * ns);
  bgoMpt->AddConstProperty("SCINTILLATIONTIMECONSTANT2", 300.0 * ns);
  bgoMpt->AddConstProperty("SCINTILLATIONYIELD1", 0.0);
  bgoMpt->AddConstProperty("SCINTILLATIONYIELD2", 1.0);
  bgo->SetMaterialPropertiesTable(bgoMpt);

  G4double airRIndex[n];
  G4double airAbs[n];
  G4double teflonRIndex[n];
  G4double teflonReflectivity[n];
  G4double teflonEfficiency[n];
  for (G4int i = 0; i < n; ++i) {
    airRIndex[i] = 1.00029;
    airAbs[i] = 1.0 * m;
    teflonRIndex[i] = 1.35;
    teflonReflectivity[i] = 0.90;
    teflonEfficiency[i] = 0.0;
  }

  auto* airMpt = new G4MaterialPropertiesTable();
  airMpt->AddProperty("RINDEX", energy, airRIndex, n);
  airMpt->AddProperty("ABSLENGTH", energy, airAbs, n);
  air->SetMaterialPropertiesTable(airMpt);

  auto* teflonMpt = new G4MaterialPropertiesTable();
  teflonMpt->AddProperty("RINDEX", energy, teflonRIndex, n);
  teflonMpt->AddProperty("REFLECTIVITY", energy, teflonReflectivity, n);
  teflonMpt->AddProperty("EFFICIENCY", energy, teflonEfficiency, n);
  teflon->SetMaterialPropertiesTable(teflonMpt);
}

void ApplyTransparentWireframe(G4LogicalVolume* logical)
{
  if (!logical) {
    return;
  }

  G4Colour colour(0.35, 0.65, 1.0, 0.18);
  const auto name = logical->GetName();
  if (name.find("logicMarker") != std::string::npos) {
    colour = G4Colour(1.0, 0.0, 0.0, 1.0);
  } else if (name.find("capsule") != std::string::npos ||
             name.find("source") != std::string::npos) {
    colour = G4Colour(0.0, 1.0, 0.0, 0.35);
  }

  G4VisAttributes attributes(colour);
  attributes.SetVisibility(true);
  attributes.SetForceWireframe(true);
  attributes.SetForceAuxEdgeVisible(true);
  logical->SetVisAttributes(attributes);

  for (auto i = 0; i < logical->GetNoDaughters(); ++i) {
    ApplyTransparentWireframe(logical->GetDaughter(i)->GetLogicalVolume());
  }
}

void ApplyDetectorMaterials(G4LogicalVolume* logical)
{
  if (!logical) {
    return;
  }

  const auto name = logical->GetName();
  auto* nist = G4NistManager::Instance();

  if (name.find("BGO_crystal") != std::string::npos) {
    logical->SetMaterial(nist->FindOrBuildMaterial("G4_BGO"));
    G4VisAttributes attributes(G4Colour(0.55, 0.55, 0.60, 0.35));
    attributes.SetForceWireframe(true);
    attributes.SetForceAuxEdgeVisible(true);
    logical->SetVisAttributes(attributes);
    G4cout << "Assigned G4_BGO with scintillation properties to " << name << G4endl;
  } else if (name.find("BGO_teflon_wrapping") != std::string::npos) {
    logical->SetMaterial(nist->FindOrBuildMaterial("G4_TEFLON"));
    G4VisAttributes attributes(G4Colour(0.85, 0.85, 1.0, 0.25));
    attributes.SetForceWireframe(true);
    attributes.SetForceAuxEdgeVisible(true);
    logical->SetVisAttributes(attributes);

    auto* surface = new G4OpticalSurface("BGO_Teflon_OpticalSurface");
    surface->SetType(dielectric_metal);
    surface->SetModel(glisur);
    surface->SetFinish(ground);
    surface->SetPolish(0.9);
    surface->SetMaterialPropertiesTable(logical->GetMaterial()->GetMaterialPropertiesTable());
    new G4LogicalSkinSurface("BGO_Teflon_SkinSurface", logical, surface);
    G4cout << "Assigned reflective G4_TEFLON optical skin to " << name << G4endl;
  } else if (name.find("stainless_steel") != std::string::npos) {
    logical->SetMaterial(nist->FindOrBuildMaterial("G4_STAINLESS-STEEL"));
    G4VisAttributes attributes(G4Colour(0.62, 0.64, 0.66, 0.35));
    attributes.SetForceWireframe(true);
    attributes.SetForceAuxEdgeVisible(true);
    logical->SetVisAttributes(attributes);
    G4cout << "Assigned G4_STAINLESS-STEEL to " << name << G4endl;
  }

  for (auto i = 0; i < logical->GetNoDaughters(); ++i) {
    ApplyDetectorMaterials(logical->GetDaughter(i)->GetLogicalVolume());
  }
}

G4LogicalVolume* FindLogicalVolume(G4LogicalVolume* logical, const G4String& pattern)
{
  if (!logical) {
    return nullptr;
  }
  if (logical->GetName().find(pattern) != std::string::npos) {
    return logical;
  }
  for (auto i = 0; i < logical->GetNoDaughters(); ++i) {
    if (auto* found = FindLogicalVolume(logical->GetDaughter(i)->GetLogicalVolume(), pattern)) {
      return found;
    }
  }
  return nullptr;
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
  ConfigureOpticalMaterials();
  ApplyTransparentWireframe(gdmlWorld->GetLogicalVolume());
  ApplyDetectorMaterials(gdmlWorld->GetLogicalVolume());

  auto* air = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");
  auto* worldSolid = new G4Box("World", 1.0 * m, 1.0 * m, 1.0 * m);
  auto* worldLogical = new G4LogicalVolume(worldSolid, air, "World");
  worldLogical->SetVisAttributes(G4VisAttributes::GetInvisible());

  new G4PVPlacement(nullptr, {}, gdmlWorld->GetLogicalVolume(), "AmBeHousing",
                    worldLogical, false, 0, true);

  auto* bgoLogical = FindLogicalVolume(gdmlWorld->GetLogicalVolume(), "BGO_crystal");
  if (bgoLogical) {
    new G4PVPlacement(nullptr, G4ThreeVector(0., 0., -126.91 * mm), bgoLogical,
                      "BGO_crystal_active", worldLogical, false, 0, true);
    G4cout << "Placed active BGO crystal directly in outer world at (0,0,-126.91) mm"
           << G4endl;
  }

  auto* teflonLogical = FindLogicalVolume(gdmlWorld->GetLogicalVolume(), "BGO_teflon_wrapping");
  if (teflonLogical) {
    new G4PVPlacement(nullptr, G4ThreeVector(0., 0., -152.01 * mm), teflonLogical,
                      "BGO_teflon_wrapping_active", worldLogical, false, 0, true);
    G4cout << "Placed active BGO Teflon wrapper directly in outer world at (0,0,-152.01) mm"
           << G4endl;
  }

  return new G4PVPlacement(nullptr, {}, worldLogical, "World", nullptr, false, 0, true);
}
