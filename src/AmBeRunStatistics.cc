#include "AmBeRunStatistics.hh"

#include "G4ios.hh"

#include <iomanip>
#include <ios>
#include <algorithm>
#include <iterator>
#include <set>
#include <vector>

namespace {
std::set<G4int> rootEvents;
std::set<G4int> gammaEvents;
std::set<G4int> bgoCrossingEvents;
std::set<G4int> bgoScintillationEvents;
std::set<G4int> anyBgoScintillationEvents;
std::set<G4int> neutronBgoScintillationEvents;

G4double Fraction(G4int numerator, G4int denominator)
{
  return denominator > 0 ? static_cast<G4double>(numerator) / denominator : 0.;
}

G4int IntersectionSize(const std::set<G4int>& left, const std::set<G4int>& right)
{
  std::vector<G4int> intersection;
  std::set_intersection(left.begin(), left.end(),
                        right.begin(), right.end(),
                        std::back_inserter(intersection));
  return static_cast<G4int>(intersection.size());
}
}

void AmBeRunStatistics::Reset()
{
  rootEvents.clear();
  gammaEvents.clear();
  bgoCrossingEvents.clear();
  bgoScintillationEvents.clear();
  anyBgoScintillationEvents.clear();
  neutronBgoScintillationEvents.clear();
}

void AmBeRunStatistics::RegisterRootEvent(G4int eventId, G4bool hasFourPointFourGamma)
{
  rootEvents.insert(eventId);
  if (hasFourPointFourGamma) {
    gammaEvents.insert(eventId);
  }
}

G4bool AmBeRunStatistics::EventIsRootSource(G4int eventId)
{
  return rootEvents.find(eventId) != rootEvents.end();
}

G4bool AmBeRunStatistics::EventHasFourPointFourGamma(G4int eventId)
{
  return gammaEvents.find(eventId) != gammaEvents.end();
}

void AmBeRunStatistics::MarkBgoCrossing(G4int eventId)
{
  if (EventHasFourPointFourGamma(eventId)) {
    bgoCrossingEvents.insert(eventId);
  }
}

void AmBeRunStatistics::MarkBgoScintillation(G4int eventId)
{
  if (EventHasFourPointFourGamma(eventId)) {
    bgoScintillationEvents.insert(eventId);
  }
}

void AmBeRunStatistics::MarkAnyBgoScintillation(G4int eventId)
{
  if (EventIsRootSource(eventId)) {
    anyBgoScintillationEvents.insert(eventId);
  }
}

void AmBeRunStatistics::MarkNeutronBgoScintillation(G4int eventId)
{
  if (EventIsRootSource(eventId)) {
    neutronBgoScintillationEvents.insert(eventId);
  }
}

void AmBeRunStatistics::PrintSummary()
{
  const auto rootTotal = static_cast<G4int>(rootEvents.size());
  const auto total = static_cast<G4int>(gammaEvents.size());
  const auto crossed = static_cast<G4int>(bgoCrossingEvents.size());
  const auto scintillated = static_cast<G4int>(bgoScintillationEvents.size());
  const auto anyScintillated = static_cast<G4int>(anyBgoScintillationEvents.size());
  const auto neutronScintillated = static_cast<G4int>(neutronBgoScintillationEvents.size());
  const auto gammaAndNeutronScintillated =
    IntersectionSize(bgoScintillationEvents, neutronBgoScintillationEvents);

  const auto oldPrecision = G4cout.precision();
  G4cout << "AmBe ROOT event summary:" << G4endl
         << "  ROOT source events: " << rootTotal << G4endl
         << "  tagged ~4.4 MeV gamma events: " << total << G4endl
         << "  crossed BGO crystal: " << crossed
         << " / " << total
         << " = " << std::fixed << std::setprecision(4)
         << Fraction(crossed, total)
         << " (" << 100. * Fraction(crossed, total) << "%)" << G4endl
         << "  crossed BGO and produced scintillation: " << scintillated
         << " / " << total
         << " = " << Fraction(scintillated, total)
         << " (" << 100. * Fraction(scintillated, total) << "%)" << G4endl
         << "  any BGO scintillation: " << anyScintillated
         << " / " << rootTotal
         << " = " << Fraction(anyScintillated, rootTotal)
         << " (" << 100. * Fraction(anyScintillated, rootTotal) << "%)" << G4endl
         << "  primary-neutron lineage produced BGO scintillation: " << neutronScintillated
         << " / " << rootTotal
         << " = " << Fraction(neutronScintillated, rootTotal)
         << " (" << 100. * Fraction(neutronScintillated, rootTotal) << "%)" << G4endl
         << "  both tagged gamma and primary-neutron lineages produced BGO scintillation: "
         << gammaAndNeutronScintillated
         << " / " << rootTotal
         << " = " << Fraction(gammaAndNeutronScintillated, rootTotal)
         << " (" << 100. * Fraction(gammaAndNeutronScintillated, rootTotal) << "%)" << G4endl;
  G4cout.unsetf(std::ios_base::floatfield);
  G4cout.precision(oldPrecision);
}
