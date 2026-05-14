#include "DetectorConstruction.hh"

#include "G4Box.hh"
#include "G4Colour.hh"
#include "G4GDMLParser.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
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

void ApplyDetectorVisualizationAndSurfaces(G4LogicalVolume* logical)
{
  if (!logical) {
    return;
  }

  const auto name = logical->GetName();

  if (name.find("BGO_crystal") != std::string::npos) {
    G4VisAttributes attributes(G4Colour(0.55, 0.55, 0.60, 0.35));
    attributes.SetForceWireframe(true);
    attributes.SetForceAuxEdgeVisible(true);
    logical->SetVisAttributes(attributes);
    G4cout << name << " uses GDML material " << logical->GetMaterial()->GetName()
           << G4endl;
  } else if (name.find("BGO_teflon_wrapping") != std::string::npos) {
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
    G4cout << name << " uses GDML material " << logical->GetMaterial()->GetName()
           << " with reflective optical skin" << G4endl;
  } else if (name.find("stainless_steel") != std::string::npos) {
    G4VisAttributes attributes(G4Colour(0.62, 0.64, 0.66, 0.35));
    attributes.SetForceWireframe(true);
    attributes.SetForceAuxEdgeVisible(true);
    logical->SetVisAttributes(attributes);
    G4cout << name << " uses GDML material " << logical->GetMaterial()->GetName()
           << G4endl;
  }

  for (auto i = 0; i < logical->GetNoDaughters(); ++i) {
    ApplyDetectorVisualizationAndSurfaces(logical->GetDaughter(i)->GetLogicalVolume());
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
  ApplyTransparentWireframe(gdmlWorld->GetLogicalVolume());
  ApplyDetectorVisualizationAndSurfaces(gdmlWorld->GetLogicalVolume());

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
