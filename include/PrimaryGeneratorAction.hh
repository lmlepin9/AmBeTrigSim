#ifndef AMBE_PRIMARY_GENERATOR_ACTION_HH
#define AMBE_PRIMARY_GENERATOR_ACTION_HH

#include "G4ThreeVector.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"

#include <memory>

class G4Event;
class G4GenericMessenger;
class G4ParticleGun;
class RootPrimarySource;

class PrimaryGeneratorAction final : public G4VUserPrimaryGeneratorAction {
public:
  PrimaryGeneratorAction();
  ~PrimaryGeneratorAction() override;

  void GeneratePrimaries(G4Event* event) override;

private:
  void DefineCommands();
  void UpdateGunDefinition();
  void SetRootFile(const G4String& path);
  void SetRootEntry(G4int entry);

  std::unique_ptr<G4ParticleGun> fParticleGun;
  std::unique_ptr<G4GenericMessenger> fMessenger;
  std::unique_ptr<RootPrimarySource> fRootSource;

  G4String fParticleName = "gamma";
  G4double fEnergy = 4.4;
  G4ThreeVector fPosition = G4ThreeVector(0., 0., -168.64);
  G4ThreeVector fDirection = G4ThreeVector(10., 0., 41.73);
  G4String fRootFile = "/home/lmlepin/Check_AmBeSimulation/AmBe-EmergingParticles-N0.root";
  G4bool fUseRootFile = false;
  G4bool fRootLoop = true;
  G4bool fRootVerbose = true;
  G4double fRootVertexScale = 1.0;
  G4ThreeVector fRootVertexOffset = G4ThreeVector(0., 0., -168.64);
};

#endif
