#include "RunAction.hh"

#include "AmBeRunStatistics.hh"
#include "G4GenericMessenger.hh"
#include "G4Run.hh"
#include "G4Timer.hh"
#include "G4ios.hh"
#include "PmtHitWriter.hh"

RunAction::RunAction()
{
  fMessenger = std::make_unique<G4GenericMessenger>(
    this, "/AmBe/output/", "ROOT output controls for PMT optical-photon hits");

  auto& fileCommand =
    fMessenger->DeclareProperty("file", fOutputFile,
                                "Output ROOT file base name for PMT optical-photon hits.");
  fileCommand.SetParameterName("path", false);
  fileCommand.SetDefaultValue("OutPut.root");
}

void RunAction::BeginOfRunAction(const G4Run* run)
{
  G4cout << "### Run " << run->GetRunID() << " start." << G4endl;
  AmBeRunStatistics::Reset();
  auto outputFile = fOutputFile;
  if (outputFile.find(".") == G4String::npos) {
    outputFile += ".root";
  }
  PmtHitWriter::Open(outputFile);
}

void RunAction::EndOfRunAction(const G4Run*)
{
  PmtHitWriter::Close();
  AmBeRunStatistics::PrintSummary();
}
