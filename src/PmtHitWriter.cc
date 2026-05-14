#include "PmtHitWriter.hh"

#include "G4Exception.hh"
#include "G4ExceptionSeverity.hh"
#include "G4ios.hh"
#include "TFile.h"
#include "TTree.h"

#include <memory>

namespace {
std::unique_ptr<TFile> outputFile;
std::unique_ptr<TTree> photonTree;

int evt = 0;
int process = 0;
double x = 0.0;
double y = 0.0;
double z = 0.0;
double t = 0.0;
double e = 0.0;

G4String WithRootExtension(G4String path)
{
  if (path.find(".") == G4String::npos) {
    path += ".root";
  }
  return path;
}
}

void PmtHitWriter::Open(const G4String& path)
{
  const auto outputPath = WithRootExtension(path);
  outputFile = std::make_unique<TFile>(outputPath.c_str(), "RECREATE");
  if (!outputFile || outputFile->IsZombie()) {
    G4ExceptionDescription description;
    description << "Could not create PMT hit ROOT file: " << outputPath;
    G4Exception("PmtHitWriter::Open", "AmBeTrigSim003", FatalException, description);
  }

  photonTree = std::make_unique<TTree>("ph", "photon data");
  photonTree->Branch("evt", &evt, "evt/I");
  photonTree->Branch("x", &x, "x/D");
  photonTree->Branch("y", &y, "y/D");
  photonTree->Branch("z", &z, "z/D");
  photonTree->Branch("t", &t, "t/D");
  photonTree->Branch("e", &e, "e/D");
  photonTree->Branch("process", &process, "process/I");

  G4cout << "Opened PMT optical-photon hit tree " << outputPath << G4endl;
}

void PmtHitWriter::Fill(int event, double xCm, double yCm, double zCm,
                        double timeNs, double energyEv, int processId)
{
  if (!photonTree) {
    return;
  }

  evt = event;
  x = xCm;
  y = yCm;
  z = zCm;
  t = timeNs;
  e = energyEv;
  process = processId;
  photonTree->Fill();
}

void PmtHitWriter::Close()
{
  if (!outputFile) {
    return;
  }

  outputFile->cd();
  if (photonTree) {
    photonTree->Write();
  }
  outputFile->Write();
  G4cout << "Wrote PMT optical-photon hit tree with "
         << (photonTree ? photonTree->GetEntries() : 0)
         << " rows to " << outputFile->GetName() << G4endl;
  photonTree.reset();
  outputFile->Close();
  outputFile.reset();
}
