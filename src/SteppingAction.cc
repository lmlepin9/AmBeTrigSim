#include "SteppingAction.hh"

#include "AmBeRunStatistics.hh"
#include "G4Colour.hh"
#include "G4DynamicParticle.hh"
#include "G4Event.hh"
#include "G4GenericMessenger.hh"
#include "G4OpticalPhoton.hh"
#include "G4Polyline.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4VisManager.hh"
#include "G4VVisManager.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VisAttributes.hh"
#include "PmtHitWriter.hh"

#include <cmath>
#include <set>
#include <string>

namespace {
constexpr G4double gammaTagEnergy = 4.4 * MeV;
constexpr G4double gammaTagTolerance = 0.2 * MeV;

G4int currentEvent = -1;
G4double bgoEdep = 0.;
G4int scintillationPhotons = 0;
G4int visualizedScintillationPhotons = 0;
G4int maxScintillationPhotonsToDraw = 300;
G4int verboseOpticalSteps = 0;
std::set<G4int> scintillationPhotonTracksToDraw;
std::set<G4int> fourPointFourGammaLineageTracks;
std::set<G4int> primaryNeutronLineageTracks;

G4bool IsBgoVolume(const G4VPhysicalVolume* volume)
{
  return volume &&
         volume->GetLogicalVolume()->GetName().find("BGO_crystal") != std::string::npos;
}

G4bool IsPrimaryFourPointFourGamma(const G4Track* track)
{
  return track->GetParentID() == 0 &&
         track->GetDefinition()->GetParticleName() == "gamma" &&
         std::abs(track->GetKineticEnergy() - gammaTagEnergy) <= gammaTagTolerance;
}

G4bool IsPrimaryNeutron(const G4Track* track)
{
  return track->GetParentID() == 0 &&
         track->GetDefinition()->GetPDGEncoding() == 2112;
}

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
  fourPointFourGammaLineageTracks.clear();
  primaryNeutronLineageTracks.clear();
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

  auto& verboseOpticalCommand =
    fMessenger->DeclareProperty("verboseOpticalSteps",
                                verboseOpticalSteps,
                                "Print the first N optical-photon steps for geometry debugging.");
  verboseOpticalCommand.SetParameterName("count", false);
  verboseOpticalCommand.SetDefaultValue("0");
}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
  const auto* event = G4RunManager::GetRunManager()->GetCurrentEvent();
  if (event && event->GetEventID() != currentEvent) {
    PrintAndResetCounters(event->GetEventID());
  }

  auto* track = step->GetTrack();
  const auto* preVolume = step->GetPreStepPoint()->GetPhysicalVolume();
  const auto* postVolume = step->GetPostStepPoint()->GetPhysicalVolume();
  const auto eventId = event ? event->GetEventID() : -1;
  if (eventId >= 0 && AmBeRunStatistics::EventIsRootSource(eventId)) {
    if (IsPrimaryFourPointFourGamma(track) ||
        fourPointFourGammaLineageTracks.find(track->GetParentID()) != fourPointFourGammaLineageTracks.end()) {
      fourPointFourGammaLineageTracks.insert(track->GetTrackID());
    }
    if (IsPrimaryNeutron(track) ||
        primaryNeutronLineageTracks.find(track->GetParentID()) != primaryNeutronLineageTracks.end()) {
      primaryNeutronLineageTracks.insert(track->GetTrackID());
    }
  }

  if (IsBgoVolume(preVolume)) {
    bgoEdep += step->GetTotalEnergyDeposit();
  }

  if (eventId >= 0 &&
      (IsBgoVolume(preVolume) || IsBgoVolume(postVolume)) &&
      IsPrimaryFourPointFourGamma(track)) {
    AmBeRunStatistics::MarkBgoCrossing(eventId);
  }

  if (track->GetDefinition()->GetParticleName() == "opticalphoton" &&
      track->GetCurrentStepNumber() == 1 &&
      track->GetCreatorProcess() &&
      track->GetCreatorProcess()->GetProcessName() == "Scintillation") {
    ++scintillationPhotons;
    if (eventId >= 0 && IsBgoVolume(preVolume)) {
      AmBeRunStatistics::MarkAnyBgoScintillation(eventId);
    }
    if (eventId >= 0 &&
        IsBgoVolume(preVolume) &&
        fourPointFourGammaLineageTracks.find(track->GetTrackID()) != fourPointFourGammaLineageTracks.end()) {
      AmBeRunStatistics::MarkBgoScintillation(eventId);
    }
    if (eventId >= 0 &&
        IsBgoVolume(preVolume) &&
        primaryNeutronLineageTracks.find(track->GetTrackID()) != primaryNeutronLineageTracks.end()) {
      AmBeRunStatistics::MarkNeutronBgoScintillation(eventId);
    }
    if (maxScintillationPhotonsToDraw < 0 ||
        visualizedScintillationPhotons < maxScintillationPhotonsToDraw) {
      scintillationPhotonTracksToDraw.insert(track->GetTrackID());
      ++visualizedScintillationPhotons;
    }
  }

  const auto preIsPmt =
    preVolume &&
    (preVolume->GetName() == "PMTLV" ||
     preVolume->GetLogicalVolume()->GetName().find("PMTLV") != std::string::npos);
  const auto postIsPmt =
    postVolume &&
    (postVolume->GetName() == "PMTLV" ||
     postVolume->GetLogicalVolume()->GetName().find("PMTLV") != std::string::npos);

  const auto isOpticalPhoton =
    track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition();

  if (isOpticalPhoton && verboseOpticalSteps > 0) {
    const auto preName = preVolume ? preVolume->GetName() : G4String("<none>");
    const auto postName = postVolume ? postVolume->GetName() : G4String("<none>");
    const auto preLogicalName =
      preVolume ? preVolume->GetLogicalVolume()->GetName() : G4String("<none>");
    const auto postLogicalName =
      postVolume ? postVolume->GetLogicalVolume()->GetName() : G4String("<none>");
    const auto prePosition = step->GetPreStepPoint()->GetPosition();
    const auto postPosition = step->GetPostStepPoint()->GetPosition();
    G4cout << "Optical step " << track->GetTrackID()
           << "." << track->GetCurrentStepNumber()
           << ": pre " << preName << " / " << preLogicalName
           << " at " << prePosition / mm << " mm"
           << " -> post " << postName << " / " << postLogicalName
           << " at " << postPosition / mm << " mm"
           << G4endl;
    --verboseOpticalSteps;
  }

  if (isOpticalPhoton && (preIsPmt || postIsPmt)) {
    G4int processId = 2;
    if (track->GetCreatorProcess()) {
      const auto processName = track->GetCreatorProcess()->GetProcessName();
      if (processName == "Scintillation") {
        processId = 0;
      } else if (processName == "Cerenkov") {
        processId = 1;
      }
    }

    const auto position = track->GetPosition();
    PmtHitWriter::Fill(event ? event->GetEventID() : -1,
                       position.x() / cm,
                       position.y() / cm,
                       position.z() / cm,
                       track->GetGlobalTime() / ns,
                       track->GetKineticEnergy() / eV,
                       processId);

    track->SetTrackStatus(fStopAndKill);
  }

  if (fourPointFourGammaLineageTracks.find(track->GetTrackID()) != fourPointFourGammaLineageTracks.end()) {
    const auto* secondaries = step->GetSecondaryInCurrentStep();
    for (const auto* secondary : *secondaries) {
      fourPointFourGammaLineageTracks.insert(secondary->GetTrackID());
    }
  }
  if (primaryNeutronLineageTracks.find(track->GetTrackID()) != primaryNeutronLineageTracks.end()) {
    const auto* secondaries = step->GetSecondaryInCurrentStep();
    for (const auto* secondary : *secondaries) {
      primaryNeutronLineageTracks.insert(secondary->GetTrackID());
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
