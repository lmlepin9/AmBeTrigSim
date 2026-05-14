#include "TFile.h"
#include "TH1D.h"
#include "TRandom3.h"
#include "TTree.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {
constexpr int kNEntries = 106;
constexpr double kBinWidthNs = 2.0;
constexpr int kNBins = 1500;
constexpr double kTimeMaxNs = kNBins * kBinWidthNs;
constexpr double kTriggerThreshold = 0.01;
constexpr double kTriggerTimeNs = 700.0;
constexpr double kNoiseStd = 0.001;
constexpr double kGainMean = 5.0 / 1000.0;
constexpr double kGainSigma = 0.6 / 1000.0;

const double kEnergyEv[kNEntries] = {
  1.771, 1.784, 1.797, 1.810, 1.823, 1.837, 1.850, 1.864, 1.878, 1.893,
  1.907, 1.922, 1.937, 1.952, 1.968, 1.984, 2.000, 2.016, 2.032, 2.049,
  2.066, 2.084, 2.101, 2.119, 2.138, 2.156, 2.175, 2.194, 2.214, 2.234,
  2.254, 2.275, 2.296, 2.317, 2.339, 2.362, 2.384, 2.407, 2.431, 2.455,
  2.480, 2.505, 2.530, 2.556, 2.583, 2.610, 2.638, 2.666, 2.695, 2.725,
  2.755, 2.786, 2.818, 2.850, 2.883, 2.917, 2.952, 2.987, 3.024, 3.061,
  3.100, 3.139, 3.179, 3.220, 3.263, 3.306, 3.351, 3.397, 3.444, 3.492,
  3.542, 3.594, 3.646, 3.701, 3.757, 3.815, 3.874, 3.936, 3.999, 4.065,
  4.133, 4.203, 4.275, 4.350, 4.428, 4.508, 4.592, 4.678, 4.768, 4.862,
  4.959, 5.060, 5.166, 5.276, 5.390, 5.510, 5.635, 5.767, 5.904, 6.048,
  6.199, 6.358, 6.525, 6.702, 6.888, 7.085
};

const double kQePercent[kNEntries] = {
  0.000, 0.000, 0.000, 0.042, 0.057, 0.083, 0.116, 0.153, 0.197, 0.256,
  0.336, 0.441, 0.568, 0.716, 0.886, 1.075, 1.283, 1.512, 1.764, 2.040,
  2.338, 2.658, 3.001, 3.370, 3.770, 4.204, 4.676, 5.189, 5.741, 6.322,
  6.923, 7.533, 8.144, 8.763, 9.380, 9.997, 10.634, 11.302, 12.025, 12.699,
  13.401, 14.094, 14.800, 15.489, 16.190, 16.911, 17.656, 18.365, 19.010, 19.542,
  20.040, 20.541, 21.102, 21.692, 22.310, 22.883, 23.360, 23.673, 23.903, 24.082,
  24.276, 24.408, 24.363, 24.240, 24.074, 23.900, 23.718, 23.505, 23.170, 22.827,
  22.414, 21.974, 21.498, 20.970, 20.491, 20.076, 19.691, 19.317, 18.954, 18.584,
  18.180, 17.704, 17.156, 16.541, 15.937, 15.321, 14.699, 14.044, 13.355, 12.648,
  11.913, 11.153, 10.384, 9.636, 8.906, 8.192, 7.494, 6.788, 6.053, 5.289,
  4.577, 4.003, 3.408, 0.000, 0.000, 0.000
};

double QuantumEfficiency(double energyEv)
{
  if (energyEv <= kEnergyEv[0]) {
    return kQePercent[0] / 100.0;
  }
  if (energyEv >= kEnergyEv[kNEntries - 1]) {
    return kQePercent[kNEntries - 1] / 100.0;
  }

  const auto* upper = std::upper_bound(kEnergyEv, kEnergyEv + kNEntries, energyEv);
  const int hi = static_cast<int>(upper - kEnergyEv);
  const int lo = hi - 1;
  const double fraction = (energyEv - kEnergyEv[lo]) / (kEnergyEv[hi] - kEnergyEv[lo]);
  return ((1.0 - fraction) * kQePercent[lo] + fraction * kQePercent[hi]) / 100.0;
}

std::vector<double> SinglePhotonResponse()
{
  std::vector<double> response;
  response.reserve(75);
  for (double time = -50.0; time < 100.0; time += kBinWidthNs) {
    const double shifted = time - 23.0;
    double value = 0.0;
    if (shifted >= 0.0) {
      value = (1.0 - std::exp(-shifted / 1.5)) * std::exp(-shifted / 5.0);
    }
    response.push_back(value);
  }

  double sum = 0.0;
  for (const auto value : response) {
    sum += value;
  }
  if (sum > 0.0) {
    for (auto& value : response) {
      value /= sum;
    }
  }
  return response;
}

std::vector<int> HistogramTimes(const std::vector<double>& times)
{
  std::vector<int> hist(kNBins, 0);
  for (const auto time : times) {
    if (time < 0.0 || time >= kTimeMaxNs) {
      continue;
    }
    const int bin = static_cast<int>(time / kBinWidthNs);
    if (bin >= 0 && bin < kNBins) {
      ++hist[bin];
    }
  }
  return hist;
}

void WriteHistogram(const std::string& name, const std::vector<double>& values)
{
  TH1D hist(name.c_str(), name.c_str(), kNBins, 0.0, kTimeMaxNs);
  for (int bin = 0; bin < kNBins; ++bin) {
    hist.SetBinContent(bin + 1, values[bin]);
  }
  hist.Write();
}

void WriteHistogram(const std::string& name, const std::vector<int>& values)
{
  TH1D hist(name.c_str(), name.c_str(), kNBins, 0.0, kTimeMaxNs);
  for (int bin = 0; bin < kNBins; ++bin) {
    hist.SetBinContent(bin + 1, values[bin]);
  }
  hist.Write();
}
}

int main(int argc, char** argv)
{
  const std::string inputPath = argc > 1 ? argv[1] : "OutPut.root";
  const std::string outputPath = argc > 2 ? argv[2] : "waveforms.root";

  TFile input(inputPath.c_str(), "READ");
  if (input.IsZombie()) {
    std::cerr << "Could not open input ROOT file: " << inputPath << "\n";
    return 1;
  }

  auto* tree = dynamic_cast<TTree*>(input.Get("ph"));
  if (!tree) {
    std::cerr << "Input file does not contain a top-level TTree named 'ph'.\n";
    return 1;
  }

  int evt = 0;
  int process = 0;
  double time = 0.0;
  double energy = 0.0;
  tree->SetBranchAddress("evt", &evt);
  tree->SetBranchAddress("t", &time);
  tree->SetBranchAddress("e", &energy);
  tree->SetBranchAddress("process", &process);

  std::map<int, std::vector<double>> scintillationHits;
  std::map<int, std::vector<double>> cherenkovHits;

  TRandom3 rng(0);
  const auto entries = tree->GetEntries();
  for (Long64_t entry = 0; entry < entries; ++entry) {
    tree->GetEntry(entry);
    if (rng.Rndm() > QuantumEfficiency(energy)) {
      continue;
    }

    const double correctedTime = time + kTriggerTimeNs;
    if (process == 0) {
      scintillationHits[evt].push_back(correctedTime);
    } else if (process == 1) {
      cherenkovHits[evt].push_back(correctedTime);
    }
  }

  TFile output(outputPath.c_str(), "RECREATE");
  if (output.IsZombie()) {
    std::cerr << "Could not create output ROOT file: " << outputPath << "\n";
    return 1;
  }

  TTree eventTree("events", "Photon event summary");
  int eventId = 0;
  int triggered = 0;
  int nCherenkov = 0;
  int nScintillation = 0;
  eventTree.Branch("event", &eventId, "event/I");
  eventTree.Branch("triggered", &triggered, "triggered/I");
  eventTree.Branch("n_cherenkov", &nCherenkov, "n_cherenkov/I");
  eventTree.Branch("n_scintillation", &nScintillation, "n_scintillation/I");

  std::set<int> eventIdSet;
  for (const auto& [id, _] : scintillationHits) {
    eventIdSet.insert(id);
  }
  for (const auto& [id, _] : cherenkovHits) {
    eventIdSet.insert(id);
  }
  std::vector<int> eventIds(eventIdSet.begin(), eventIdSet.end());

  const auto spr = SinglePhotonResponse();
  const auto timestamp = static_cast<long long>(std::time(nullptr));

  int processed = 0;
  for (const auto id : eventIds) {
    const auto sciIt = scintillationHits.find(id);
    const auto cheIt = cherenkovHits.find(id);
    const auto sciHits = sciIt == scintillationHits.end() ? std::vector<double>{} : sciIt->second;
    const auto cheHits = cheIt == cherenkovHits.end() ? std::vector<double>{} : cheIt->second;

    const auto histSci = HistogramTimes(sciHits);
    const auto histChe = HistogramTimes(cheHits);

    std::vector<double> smeared(kNBins, 0.0);
    for (int bin = 0; bin < kNBins; ++bin) {
      const int photons = histSci[bin] + histChe[bin];
      if (photons > 0) {
        smeared[bin] = rng.Gaus(photons * kGainMean, std::sqrt(photons) * kGainSigma);
      }
    }

    std::vector<double> signal(kNBins, 0.0);
    for (int bin = 0; bin < kNBins; ++bin) {
      if (smeared[bin] == 0.0) {
        continue;
      }
      for (std::size_t sprBin = 0; sprBin < spr.size(); ++sprBin) {
        const int outBin = bin + static_cast<int>(sprBin);
        if (outBin >= kNBins) {
          break;
        }
        signal[outBin] += smeared[bin] * spr[sprBin];
      }
    }

    for (auto& value : signal) {
      value += rng.Gaus(0.0, kNoiseStd);
    }

    const auto peakIt = std::max_element(signal.begin(), signal.end());
    triggered = peakIt != signal.end() && *peakIt > kTriggerThreshold ? 1 : 0;

    if (triggered) {
      const auto triggerIt = std::find_if(signal.begin(), signal.end(),
                                         [](double value) { return value > kTriggerThreshold; });
      const int triggerIndex = static_cast<int>(triggerIt - signal.begin());
      const int targetIndex = static_cast<int>(kTriggerTimeNs / kBinWidthNs);
      const int shift = targetIndex - triggerIndex;
      std::vector<double> shifted(kNBins, 0.0);
      for (auto& value : shifted) {
        value = rng.Gaus(0.0, kNoiseStd);
      }
      for (int bin = 0; bin < kNBins; ++bin) {
        const int outBin = bin + shift;
        if (outBin >= 0 && outBin < kNBins) {
          shifted[outBin] = signal[bin];
        }
      }
      WriteHistogram("BRF_" + std::to_string(id) + "_" + std::to_string(timestamp), shifted);
    }

    WriteHistogram("WVF_" + std::to_string(id) + "_" + std::to_string(timestamp), signal);
    WriteHistogram("SCI_" + std::to_string(id) + "_" + std::to_string(timestamp), histSci);
    WriteHistogram("CHE_" + std::to_string(id) + "_" + std::to_string(timestamp), histChe);

    eventId = id;
    nScintillation = static_cast<int>(sciHits.size());
    nCherenkov = static_cast<int>(cheHits.size());
    eventTree.Fill();

    ++processed;
    std::cout << "Processed event " << processed << " (ID " << id << ")\n";
  }

  eventTree.Write();
  output.Close();
  input.Close();

  std::cout << "Wrote PMT response waveforms to " << outputPath << "\n";
  return 0;
}
