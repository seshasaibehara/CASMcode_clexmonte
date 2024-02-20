#ifndef CASM_clexmonte_definitions
#define CASM_clexmonte_definitions

#include <map>
#include <string>

#include "casm/monte/BasicStatistics.hh"
#include "casm/monte/definitions.hh"

namespace CASM {

namespace composition {
class CompositionCalculator;
class CompositionConverter;
}  // namespace composition

namespace clexmonte {
struct Configuration;
struct System;
}  // namespace clexmonte

namespace clexmonte {

typedef System system_type;
typedef Configuration config_type;
typedef monte::BasicStatistics statistics_type;
typedef monte::State<config_type> state_type;
struct Conditions;

// ### Sampling ###

typedef monte::StateSamplingFunction state_sampling_function_type;
typedef monte::ResultsAnalysisFunction<config_type, statistics_type>
    results_analysis_function_type;
typedef monte::SamplingFixtureParams<config_type, statistics_type>
    sampling_fixture_params_type;

typedef monte::RunManagerParams run_manager_params_type;
template <typename EngineType>
using run_manager_type =
    monte::RunManager<config_type, statistics_type, EngineType>;
typedef monte::Results<config_type, statistics_type> results_type;
typedef monte::ResultsIO<results_type> results_io_type;

// ### State generation ###

struct RunData;

class StateGenerator;
typedef StateGenerator state_generator_type;
class IncrementalConditionsStateGenerator;

class ConfigGenerator;
typedef ConfigGenerator config_generator_type;
class FixedConfigGenerator;

struct StateModifyingFunction;
using StateModifyingFunctionMap = std::map<std::string, StateModifyingFunction>;

typedef std::function<Eigen::VectorXd(std::vector<Eigen::VectorXd> const &)>
    CorrCalculatorFunction;

}  // namespace clexmonte
}  // namespace CASM

#endif
