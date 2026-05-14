#ifndef AMBE_ROOT_PRIMARY_SOURCE_HH
#define AMBE_ROOT_PRIMARY_SOURCE_HH

#include "G4String.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"

#include <Math/Vector4D.h>

#include <memory>
#include <vector>

class G4Event;
class G4ParticleGun;
class TFile;
class TTree;

class RootPrimarySource final {
public:
  using LorentzVector = ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double>>;

  explicit RootPrimarySource(G4String path);
  ~RootPrimarySource();

  void SetPath(const G4String& path);
  void SetEnabled(G4bool enabled) { fEnabled = enabled; }
  void SetLoop(G4bool loop) { fLoop = loop; }
  void SetVertexOffset(const G4ThreeVector& offset) { fVertexOffset = offset; }
  void SetVertexScale(G4double scale) { fVertexScale = scale; }
  void SetVerbose(G4bool verbose) { fVerbose = verbose; }
  void SetCurrentEntry(G4int entry) { fCurrentEntry = entry; }

  G4bool IsEnabled() const { return fEnabled; }
  const G4String& GetPath() const { return fPath; }
  G4int GetCurrentEntry() const { return fCurrentEntry; }
  G4int GetEntries() const;

  void OpenIfNeeded();
  void GenerateEvent(G4Event* event, G4ParticleGun& gun);

private:
  void Open();
  void Close();

  G4String fPath;
  G4bool fEnabled = true;
  G4bool fLoop = true;
  G4bool fVerbose = true;
  G4int fCurrentEntry = 0;
  G4double fVertexScale = 1.0;
  G4ThreeVector fVertexOffset = G4ThreeVector(0., 0., -168.64);

  std::unique_ptr<TFile> fFile;
  TTree* fTree = nullptr;

  G4int fEventId = 0;
  std::vector<int>* fPDG = nullptr;
  std::vector<LorentzVector>* fVertex = nullptr;
  std::vector<LorentzVector>* fMomentum = nullptr;
};

#endif
