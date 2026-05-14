cmake_minimum_required(VERSION 3.16)

project(AmBeTrigSim LANGUAGES CXX)

find_package(Geant4 REQUIRED ui_all vis_all)
find_package(ROOT REQUIRED COMPONENTS Core Tree MathCore)

add_executable(ambe_sim
  ambe_sim.cc
  src/ActionInitialization.cc
  src/DetectorConstruction.cc
  src/PrimaryGeneratorAction.cc
  src/RootPrimarySource.cc
  src/SteppingAction.cc
)

target_include_directories(ambe_sim PRIVATE include)
target_compile_features(ambe_sim PRIVATE cxx_std_17)
target_compile_definitions(ambe_sim PRIVATE
  AMBE_DEFAULT_GDML_PATH="${CMAKE_SOURCE_DIR}/AmBeHousing.gdml"
)
target_link_libraries(ambe_sim PRIVATE ${Geant4_LIBRARIES} ROOT::Core ROOT::Tree ROOT::MathCore)

file(COPY macros DESTINATION ${CMAKE_BINARY_DIR})
