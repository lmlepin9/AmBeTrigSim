#ifndef AMBE_STEPPING_ACTION_HH
#define AMBE_STEPPING_ACTION_HH

#include "G4UserSteppingAction.hh"

#include <memory>

class G4Step;
class G4GenericMessenger;

class SteppingAction final : public G4UserSteppingAction {
public:
  SteppingAction();
  ~SteppingAction() override = default;

  void UserSteppingAction(const G4Step* step) override;

private:
  std::unique_ptr<G4GenericMessenger> fMessenger;
};

#endif
