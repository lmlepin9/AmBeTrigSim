cmake_minimum_required(VERSION 3.16)

project(AmBeTrigSim LANGUAGES CXX)

find_package(Geant4 REQUIRED ui_all vis_all)
find_package(ROOT REQUIRED COMPONENTS Core Tree MathCore Hist RIO)

add_executable(ambe_sim
  ambe_sim.cc
  src/ActionInitialization.cc
  src/DetectorConstruction.cc
  src/PmtHitWriter.cc
  src/PrimaryGeneratorAction.cc
  src/RunAction.cc
  src/RootPrimarySource.cc
  src/SteppingAction.cc
)

target_include_directories(ambe_sim PRIVATE include)
target_compile_features(ambe_sim PRIVATE cxx_std_17)
target_compile_definitions(ambe_sim PRIVATE
  AMBE_DEFAULT_GDML_PATH="${CMAKE_SOURCE_DIR}/AmBeHousing.gdml"
)
target_link_libraries(ambe_sim PRIVATE ${Geant4_LIBRARIES} ROOT::Core ROOT::Tree ROOT::MathCore ROOT::RIO)

add_executable(pmt_response
  pmt_response/PmtResponse.cc
)

target_compile_features(pmt_response PRIVATE cxx_std_17)
target_link_libraries(pmt_response PRIVATE ROOT::Core ROOT::Tree ROOT::Hist ROOT::RIO)

file(COPY macros DESTINATION ${CMAKE_BINARY_DIR})
