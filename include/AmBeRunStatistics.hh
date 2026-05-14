#ifndef AMBE_RUN_STATISTICS_HH
#define AMBE_RUN_STATISTICS_HH

#include "globals.hh"

namespace AmBeRunStatistics {
void Reset();

void RegisterRootEvent(G4int eventId, G4bool hasFourPointFourGamma);
G4bool EventIsRootSource(G4int eventId);
G4bool EventHasFourPointFourGamma(G4int eventId);

void MarkBgoCrossing(G4int eventId);
void MarkBgoScintillation(G4int eventId);
void MarkAnyBgoScintillation(G4int eventId);
void MarkNeutronBgoScintillation(G4int eventId);

void PrintSummary();
}

#endif
