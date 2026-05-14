#ifndef AMBE_ACTION_INITIALIZATION_HH
#define AMBE_ACTION_INITIALIZATION_HH

#include "G4VUserActionInitialization.hh"

class ActionInitialization final : public G4VUserActionInitialization {
public:
  ActionInitialization() = default;
  ~ActionInitialization() override = default;

  void Build() const override;
};

#endif
