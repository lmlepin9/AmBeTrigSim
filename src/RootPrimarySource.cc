#include "RootPrimarySource.hh"

#include "AmBeRunStatistics.hh"
#include "G4Event.hh"
#include "G4Exception.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

#include "TFile.h"
#include "TTree.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
constexpr G4double gammaTagEnergyMeV = 4.4;
constexpr G4double gammaTagToleranceMeV = 0.2;
}

RootPrimarySource::RootPrimarySource(G4String path)
  : fPath(std::move(path))
{}

RootPrimarySource::~RootPrimarySource()
{
  Close();
}

void RootPrimarySource::SetPath(const G4String& path)
{
  if (path != fPath) {
    fPath = path;
    fCurrentEntry = 0;
    Close();
  }
}

G4int RootPrimarySource::GetEntries() const
{
  return fTree ? static_cast<G4int>(fTree->GetEntries()) : 0;
}

void RootPrimarySource::OpenIfNeeded()
{
  if (!fFile) {
    Open();
  }
}

void RootPrimarySource::Open()
{
  fFile.reset(TFile::Open(fPath.c_str(), "READ"));
  if (!fFile || fFile->IsZombie()) {
    G4ExceptionDescription description;
    description << "Could not open ROOT primary source file: " << fPath;
    G4Exception("RootPrimarySource::Open", "AmBeTrigSim003", FatalException, description);
  }

  fTree = dynamic_cast<TTree*>(fFile->Get("EmergingParticles"));
  if (!fTree) {
    G4Exception("RootPrimarySource::Open", "AmBeTrigSim004", FatalException,
                "Could not find TTree 'EmergingParticles'.");
  }

  fTree->SetBranchAddress("EventId", &fEventId);
  fTree->SetBranchAddress("PDG", &fPDG);
  fTree->SetBranchAddress("Vertex", &fVertex);
  fTree->SetBranchAddress("Momentum", &fMomentum);

  G4cout << "Loaded ROOT primary source " << fPath << " with "
         << fTree->GetEntries() << " entries." << G4endl;
}

void RootPrimarySource::Close()
{
  fTree = nullptr;
  fPDG = nullptr;
  fVertex = nullptr;
  fMomentum = nullptr;
  fFile.reset();
}

void RootPrimarySource::GenerateEvent(G4Event* event, G4ParticleGun& gun)
{
  OpenIfNeeded();

  const auto entries = fTree->GetEntries();
  if (entries <= 0) {
    G4Exception("RootPrimarySource::GenerateEvent", "AmBeTrigSim005", FatalException,
                "ROOT primary source tree has no entries.");
  }

  if (fCurrentEntry >= entries) {
    if (!fLoop) {
      G4Exception("RootPrimarySource::GenerateEvent", "AmBeTrigSim006", FatalException,
                  "ROOT primary source reached end of file and looping is disabled.");
    }
    fCurrentEntry = 0;
  }

  fTree->GetEntry(fCurrentEntry);
  if (!fPDG || !fVertex || !fMomentum ||
      fPDG->size() != fVertex->size() || fPDG->size() != fMomentum->size()) {
    G4Exception("RootPrimarySource::GenerateEvent", "AmBeTrigSim007", FatalException,
                "ROOT primary source entry has inconsistent particle arrays.");
  }

  if (fVerbose) {
    G4cout << "ROOT primary entry " << fCurrentEntry
           << " EventId " << fEventId
           << " with " << fPDG->size() << " particles" << G4endl;
  }

  auto* particleTable = G4ParticleTable::GetParticleTable();
  auto hasFourPointFourGamma = false;
  for (std::size_t i = 0; i < fPDG->size(); ++i) {
    auto* particle = particleTable->FindParticle(fPDG->at(i));
    if (!particle) {
      G4ExceptionDescription description;
      description << "Unknown PDG code " << fPDG->at(i)
                  << " in ROOT source entry " << fCurrentEntry << ".";
      G4Exception("RootPrimarySource::GenerateEvent", "AmBeTrigSim008", FatalException, description);
    }

    const auto& vertex = fVertex->at(i);
    const auto& momentum = fMomentum->at(i);
    const G4ThreeVector position =
      (fVertexOffset + G4ThreeVector(vertex.X(), vertex.Y(), vertex.Z()) * fVertexScale) * mm;
    const G4ThreeVector p(momentum.Px(), momentum.Py(), momentum.Pz());
    const auto pMag = p.mag();
    if (pMag <= 0.) {
      continue;
    }

    const auto totalEnergyMeV = momentum.E();
    const auto massMeV = particle->GetPDGMass() / MeV;
    const auto kineticEnergyMeV = std::max(0., totalEnergyMeV - massMeV);
    if (fPDG->at(i) == 22 &&
        std::abs(kineticEnergyMeV - gammaTagEnergyMeV) <= gammaTagToleranceMeV) {
      hasFourPointFourGamma = true;
    }

    gun.SetParticleDefinition(particle);
    gun.SetParticlePosition(position);
    gun.SetParticleMomentumDirection(p.unit());
    gun.SetParticleEnergy(kineticEnergyMeV * MeV);
    gun.GeneratePrimaryVertex(event);

    if (fVerbose) {
      G4cout << "  PDG " << fPDG->at(i)
             << " kinetic " << kineticEnergyMeV << " MeV"
             << " position " << position / mm << " mm"
             << " direction " << p.unit() << G4endl;
    }
  }

  AmBeRunStatistics::RegisterRootEvent(event->GetEventID(), hasFourPointFourGamma);
  ++fCurrentEntry;
}
