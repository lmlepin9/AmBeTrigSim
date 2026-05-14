#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"

#include "FTFP_BERT_HP.hh"
#include "G4OpticalParameters.hh"
#include "G4OpticalPhysics.hh"
#include "G4RunManagerFactory.hh"
#include "G4SteppingVerbose.hh"
#include "G4UImanager.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"

#include <memory>
#include <vector>

namespace {
G4String DefaultGdmlPath()
{
  return AMBE_DEFAULT_GDML_PATH;
}
}

int main(int argc, char** argv)
{
  G4bool useAmBeRootPrimaries = false;
  std::vector<G4String> positionalArgs;
  for (auto i = 1; i < argc; ++i) {
    const G4String arg = argv[i];
    if (arg == "--ambe") {
      useAmBeRootPrimaries = true;
    } else {
      positionalArgs.push_back(arg);
    }
  }

  auto uiArgc = 1;
  char* uiArgv[] = {argv[0], nullptr};
  auto ui = positionalArgs.empty() ? std::make_unique<G4UIExecutive>(uiArgc, uiArgv) : nullptr;

  auto runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);

  const G4String gdmlPath = positionalArgs.size() > 1 ? positionalArgs[1] : DefaultGdmlPath();
  runManager->SetUserInitialization(new DetectorConstruction(gdmlPath));

  auto* physicsList = new FTFP_BERT_HP;
  auto* opticalParameters = G4OpticalParameters::Instance();
  opticalParameters->SetCerenkovTrackSecondariesFirst(true);
  opticalParameters->SetScintTrackSecondariesFirst(true);
  auto* opticalPhysics = new G4OpticalPhysics;
  physicsList->RegisterPhysics(opticalPhysics);
  runManager->SetUserInitialization(physicsList);
  runManager->SetUserInitialization(new ActionInitialization);

  auto visManager = std::make_unique<G4VisExecutive>();
  visManager->Initialize();

  auto* uiManager = G4UImanager::GetUIpointer();
  if (ui) {
    uiManager->ApplyCommand(useAmBeRootPrimaries
                              ? "/control/execute macros/vis_ambe.mac"
                              : "/control/execute macros/vis.mac");
    ui->SessionStart();
  } else {
    const G4String macro = positionalArgs[0];
    uiManager->ApplyCommand("/control/execute " + macro);
  }

  delete runManager;
  return 0;
}
