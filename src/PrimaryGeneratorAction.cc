#include "PrimaryGeneratorAction.hh"

#include "RootPrimarySource.hh"

#include "G4Event.hh"
#include "G4GenericMessenger.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

#include <utility>

PrimaryGeneratorAction::PrimaryGeneratorAction()
  : fParticleGun(std::make_unique<G4ParticleGun>(1))
{
  fRootSource = std::make_unique<RootPrimarySource>(fRootFile);
  DefineCommands();
  UpdateGunDefinition();
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() = default;

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
  if (fUseRootFile) {
    fRootSource->SetPath(fRootFile);
    fRootSource->SetEnabled(true);
    fRootSource->SetLoop(fRootLoop);
    fRootSource->SetVerbose(fRootVerbose);
    fRootSource->SetVertexScale(fRootVertexScale);
    fRootSource->SetVertexOffset(fRootVertexOffset);
    fRootSource->GenerateEvent(event, *fParticleGun);
    return;
  }

  fParticleGun->SetParticlePosition(fPosition * mm);
  fParticleGun->SetParticleMomentumDirection(fDirection.unit());
  fParticleGun->GeneratePrimaryVertex(event);
}

void PrimaryGeneratorAction::DefineCommands()
{
  fMessenger = std::make_unique<G4GenericMessenger>(
    this, "/AmBe/source/", "AmBeTrigSim primary generator controls");

  auto& particleCmd =
    fMessenger->DeclareProperty("particle", fParticleName,
                                "Primary particle name. Default: gamma");
  particleCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& energyCmd =
    fMessenger->DeclarePropertyWithUnit("energy", "MeV", fEnergy,
                                        "Primary kinetic energy. Default: 4.4 MeV");
  energyCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& positionCmd =
    fMessenger->DeclarePropertyWithUnit("position", "mm", fPosition,
                                        "Primary position in GDML world coordinates. "
                                        "Default: 0 0 -168.64 mm, the GDML logicMarker/source center.");
  positionCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& directionCmd =
    fMessenger->DeclareProperty("direction", fDirection,
                                "Primary momentum direction. Default: 10 0 41.73, "
                                "from the source center to an off-axis point inside the BGO crystal.");
  directionCmd.SetStates(G4State_PreInit, G4State_Idle);

  fMessenger->DeclareMethod("update", &PrimaryGeneratorAction::UpdateGunDefinition,
                            "Apply changed particle or energy settings to the particle gun.")
    .SetStates(G4State_PreInit, G4State_Idle);

  auto& useRootCmd =
    fMessenger->DeclareProperty("useRootFile", fUseRootFile,
                                "Use particles from the ROOT EmergingParticles tree. Default: false");
  useRootCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& rootFileCmd =
    fMessenger->DeclareProperty("rootFile", fRootFile,
                                "ROOT file with EmergingParticles tree.");
  rootFileCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& rootLoopCmd =
    fMessenger->DeclareProperty("rootLoop", fRootLoop,
                                "Loop back to the first ROOT entry after reaching the end.");
  rootLoopCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& rootVerboseCmd =
    fMessenger->DeclareProperty("rootVerbose", fRootVerbose,
                                "Print ROOT entry and generated particle details.");
  rootVerboseCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& rootScaleCmd =
    fMessenger->DeclareProperty("rootVertexScale", fRootVertexScale,
                                "Scale applied to ROOT vertex coordinates before mm units. "
                                "Default: 1, treating ROOT vertices as mm.");
  rootScaleCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& rootOffsetCmd =
    fMessenger->DeclarePropertyWithUnit("rootVertexOffset", "mm", fRootVertexOffset,
                                        "Offset added to ROOT vertices after scaling. "
                                        "Default: AmBe source center.");
  rootOffsetCmd.SetStates(G4State_PreInit, G4State_Idle);

  fMessenger->DeclareMethod("setRootFile", &PrimaryGeneratorAction::SetRootFile,
                            "Set ROOT file path and reset the ROOT entry cursor.")
    .SetStates(G4State_PreInit, G4State_Idle);

  fMessenger->DeclareMethod("setRootEntry", &PrimaryGeneratorAction::SetRootEntry,
                            "Set the next ROOT tree entry to generate.")
    .SetStates(G4State_PreInit, G4State_Idle);
}

void PrimaryGeneratorAction::UpdateGunDefinition()
{
  auto* particle = G4ParticleTable::GetParticleTable()->FindParticle(fParticleName);
  if (!particle) {
    G4ExceptionDescription description;
    description << "Unknown particle '" << fParticleName << "'.";
    G4Exception("PrimaryGeneratorAction::UpdateGunDefinition", "AmBeTrigSim002",
                FatalException, description);
  }

  fParticleGun->SetParticleDefinition(particle);
  fParticleGun->SetParticleEnergy(fEnergy * MeV);

  G4cout << "Primary generator: " << fParticleName << ", "
         << fEnergy << " MeV, position "
         << fPosition << " mm, direction "
         << fDirection.unit() << G4endl;

  fRootSource->SetPath(fRootFile);
  fRootSource->SetLoop(fRootLoop);
  fRootSource->SetVerbose(fRootVerbose);
  fRootSource->SetVertexScale(fRootVertexScale);
  fRootSource->SetVertexOffset(fRootVertexOffset);
  if (fUseRootFile) {
    G4cout << "ROOT primary source enabled: " << fRootFile << G4endl;
  }
}

void PrimaryGeneratorAction::SetRootFile(const G4String& path)
{
  fRootFile = path;
  fRootSource->SetPath(fRootFile);
}

void PrimaryGeneratorAction::SetRootEntry(G4int entry)
{
  fRootSource->SetCurrentEntry(entry);
}
