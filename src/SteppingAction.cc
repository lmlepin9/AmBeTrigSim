#include "SteppingAction.hh"

#include "G4Colour.hh"
#include "G4Event.hh"
#include "G4GenericMessenger.hh"
#include "G4Polyline.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4VisManager.hh"
#include "G4VVisManager.hh"
#include "G4VisAttributes.hh"

#include <set>
#include <string>

namespace {
G4int currentEvent = -1;
G4double bgoEdep = 0.;
G4int scintillationPhotons = 0;
G4int visualizedScintillationPhotons = 0;
G4int maxScintillationPhotonsToDraw = 300;
std::set<G4int> scintillationPhotonTracksToDraw;

void PrintAndResetCounters(G4int nextEvent)
{
  if (currentEvent >= 0) {
    G4cout << "Event " << currentEvent
           << ": BGO edep = " << bgoEdep / MeV << " MeV, "
           << "scintillation optical photons = " << scintillationPhotons
           << ", visualized = " << visualizedScintillationPhotons
           << G4endl;
  }
  currentEvent = nextEvent;
  bgoEdep = 0.;
  scintillationPhotons = 0;
  visualizedScintillationPhotons = 0;
  scintillationPhotonTracksToDraw.clear();
}
}

SteppingAction::SteppingAction()
{
  fMessenger = std::make_unique<G4GenericMessenger>(
    this, "/AmBe/vis/", "Visualization controls for AmBeTrigSim");

  auto& maxOpticalCommand =
    fMessenger->DeclareProperty("maxScintillationPhotons",
                                maxScintillationPhotonsToDraw,
                                "Maximum number of scintillation optical photons to draw per event. Use -1 for no limit.");
  maxOpticalCommand.SetParameterName("count", false);
  maxOpticalCommand.SetDefaultValue("300");
}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
  const auto* event = G4RunManager::GetRunManager()->GetCurrentEvent();
  if (event && event->GetEventID() != currentEvent) {
    PrintAndResetCounters(event->GetEventID());
  }

  const auto* track = step->GetTrack();
  const auto* preVolume = step->GetPreStepPoint()->GetPhysicalVolume();
  if (preVolume && preVolume->GetLogicalVolume()->GetName().find("BGO_crystal") != std::string::npos) {
    bgoEdep += step->GetTotalEnergyDeposit();
  }

  if (track->GetDefinition()->GetParticleName() == "opticalphoton" &&
      track->GetCurrentStepNumber() == 1 &&
      track->GetCreatorProcess() &&
      track->GetCreatorProcess()->GetProcessName() == "Scintillation") {
    ++scintillationPhotons;
    if (maxScintillationPhotonsToDraw < 0 ||
        visualizedScintillationPhotons < maxScintillationPhotonsToDraw) {
      scintillationPhotonTracksToDraw.insert(track->GetTrackID());
      ++visualizedScintillationPhotons;
    }
  }

  auto* visManager = G4VVisManager::GetConcreteInstance();
  if (!visManager) {
    return;
  }
  auto* fullVisManager = G4VisManager::GetInstance();
  if (!fullVisManager || !fullVisManager->GetCurrentViewer()) {
    return;
  }

  const auto particleName = track->GetDefinition()->GetParticleName();
  if (particleName == "opticalphoton" &&
      scintillationPhotonTracksToDraw.find(track->GetTrackID()) == scintillationPhotonTracksToDraw.end()) {
    return;
  }

  G4Polyline line;
  line.push_back(step->GetPreStepPoint()->GetPosition());
  line.push_back(step->GetPostStepPoint()->GetPosition());

  G4Colour colour(1.0, 1.0, 0.0);
  if (particleName == "opticalphoton") {
    colour = G4Colour(0.0, 1.0, 0.0);
  } else if (track->GetDefinition()->GetPDGEncoding() == 2112) {
    colour = G4Colour(0.0, 1.0, 0.75);
  } else if (track->GetDefinition()->GetPDGCharge() > 0.) {
    colour = G4Colour(1.0, 0.2, 0.2);
  } else if (track->GetDefinition()->GetPDGCharge() < 0.) {
    colour = G4Colour(0.2, 0.6, 1.0);
  }

  G4VisAttributes attributes(colour);
  attributes.SetLineWidth(4.0);
  line.SetVisAttributes(&attributes);

  visManager->Draw(line);
}
