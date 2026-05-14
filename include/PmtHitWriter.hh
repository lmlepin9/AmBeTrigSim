#ifndef AMBE_PMT_HIT_WRITER_HH
#define AMBE_PMT_HIT_WRITER_HH

#include "G4String.hh"

namespace PmtHitWriter {
void Open(const G4String& path);
void Fill(int event, double xCm, double yCm, double zCm, double timeNs,
          double energyEv, int process);
void Close();
}

#endif
