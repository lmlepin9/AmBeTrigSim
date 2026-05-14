#ifndef AMBE_RUN_ACTION_HH
#define AMBE_RUN_ACTION_HH

#include "G4String.hh"
#include "G4UserRunAction.hh"

#include <memory>

class G4GenericMessenger;
class G4Run;

class RunAction final : public G4UserRunAction {
public:
  RunAction();
  ~RunAction() override = default;

  void BeginOfRunAction(const G4Run* run) override;
  void EndOfRunAction(const G4Run* run) override;

private:
  G4String fOutputFile = "OutPut.root";
  std::unique_ptr<G4GenericMessenger> fMessenger;
};

#endif
