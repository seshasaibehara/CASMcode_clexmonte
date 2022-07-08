#include "casm/casm_io/json/InputParser_impl.hh"
#include "casm/clexmonte/canonical/conditions.hh"
#include "casm/clexmonte/canonical/io/InputData.hh"
#include "casm/clexmonte/canonical/io/json/InputData_json_io.hh"
#include "casm/clexmonte/canonical/run.hh"
#include "casm/clexmonte/canonical/sampling_functions.hh"
#include "casm/clexmonte/clex/ClexData.hh"
#include "casm/clexmonte/clex/io/json/State_json_io.hh"
#include "casm/clexmonte/system/OccSystem.hh"
#include "casm/clexulator/Clexulator.hh"
#include "casm/clexulator/NeighborList.hh"
#include "casm/clexulator/io/json/SparseCoefficients_json_io.hh"
#include "casm/composition/CompositionConverter.hh"
#include "casm/crystallography/io/BasicStructureIO.hh"
#include "casm/monte/MethodLog.hh"
#include "casm/monte/results/io/json/jsonResultsIO_impl.hh"
#include "casm/monte/state/FixedConfigGenerator.hh"
#include "casm/monte/state/StateSampler.hh"
#include "casm/system/RuntimeLibrary.hh"
#include "gtest/gtest.h"
#include "testdir.hh"

// // Test0
// #include "casm/clexmonte/system/OccSystem.hh"
// #include "casm/clexmonte/system/io/json/OccSystem_json_io.hh"
// #include "casm/clexmonte/system/sampling_functions.hh"
// #include "casm/monte/Conversions.hh"
// #include "casm/monte/checks/CompletionCheck.hh"
// #include "casm/monte/events/OccCandidate.hh"
// #include "casm/monte/methods/canonical.hh"

using namespace CASM;

TEST(canonical_fullrun_test, Test1) {
  // Copy test input data to a temperorary directory
  fs::path test_data_dir = test::data_dir("clexmonte") / "Clex_ZrO_Occ";
  fs::path clexulator_src_relpath = fs::path("basis_sets") /
                                    "bset.formation_energy" /
                                    "ZrO_Clexulator_formation_energy.cc";
  fs::path eci_relpath = "formation_energy_eci.json";
  fs::path prim_relpath = "prim.json";

  test::TmpDir tmp_dir;
  tmp_dir.do_not_remove_on_destruction();
  fs::create_directories(tmp_dir / clexulator_src_relpath.parent_path());
  fs::copy_file(test_data_dir / clexulator_src_relpath,
                tmp_dir / clexulator_src_relpath);
  fs::copy_file(test_data_dir / eci_relpath, tmp_dir / eci_relpath);
  fs::copy_file(test_data_dir / prim_relpath, tmp_dir / prim_relpath);

  // Set Clexulator compilation options
  //   ex: g++ -O3 -Wall -fPIC --std=c++17 -I/path/to/include
  std::string default_clexulator_compile_options =
      //
      // uses $CASM_CXX, else default="g++"
      RuntimeLibrary::default_cxx().first + " " +
      //
      // uses $CASM_CXXFLAGS, else default="-O3 -Wall -fPIC --std=c++17"
      RuntimeLibrary::default_cxxflags().first + " " +
      //
      // uses -I$CASM_INCLUDEDIR,
      //   else -I$CASM_PREFIX/include,
      //   else tries to find "ccasm" or "casm" executable on PATH and looks
      //     for standard include paths relative from there,
      //   else fails with "/not/found"
      include_path(RuntimeLibrary::default_casm_includedir().first);

  // Set Clexulator shared object compilation options
  //   ex: g++ -shared -L/path/to/lib -lcasm_global -lcasm_crystallography
  //     -lcasm_clexulator -lcasm_monte
  std::string default_clexulator_so_options =
      //
      // uses $CASM_CXX, else default="g++"
      RuntimeLibrary::default_cxx().first + " " +
      //
      // uses $CASM_SOFLAGS, else default="-shared"
      RuntimeLibrary::default_soflags().first + " " +
      //
      // uses -L$CASM_LIBDIR,
      //   else -L$CASM_PREFIX/lib,
      //   else tries to find "ccasm" or "casm" executables on PATH and looks
      //     for libcasm at standard relative paths from there,
      //   else fails with "-L/not/found"
      link_path(RuntimeLibrary::default_casm_libdir().first) + " " +
      //
      // requires libcasm_clexulator:
      "-lcasm_clexulator ";

  // Create an output directory
  fs::path output_dir_relpath = "output";
  fs::create_directories(tmp_dir / output_dir_relpath);

  // Error message
  std::runtime_error error_if_invalid{
      "Error reading canonical Monte Carlo JSON input"};

  // ### Construct system data

  // - Construct prim
  jsonParser prim_json(tmp_dir / prim_relpath);
  std::shared_ptr<xtal::BasicStructure const> shared_prim =
      std::make_shared<xtal::BasicStructure const>(read_prim(prim_json, TOL));

  // - Construct composition::CompositionConverter
  std::vector<std::string> components = {"Zr", "Va", "O"};

  Eigen::VectorXd origin;
  origin.resize(3);
  origin << 2.0, 2.0, 0.0;

  Eigen::MatrixXd end_members;
  end_members.resize(3, 1);
  end_members.col(0) << 2.0, 0.0, 2.0;

  composition::CompositionConverter composition_converter(components, origin,
                                                          end_members);

  // - Construct clexulator::PrimNeighborList
  std::shared_ptr<clexulator::PrimNeighborList> prim_neighbor_list;

  // - Construct clexulator::Clexulator
  fs::path clexulator_src = tmp_dir / clexulator_src_relpath;
  std::string clexulator_name = clexulator_src.stem();
  fs::path clexulator_dirpath = clexulator_src.parent_path();
  std::string clexulator_compile_options = default_clexulator_compile_options;
  std::string clexulator_so_options = default_clexulator_so_options;
  std::shared_ptr<clexulator::Clexulator> clexulator =
      std::make_shared<clexulator::Clexulator>(clexulator::make_clexulator(
          clexulator_name, clexulator_dirpath, prim_neighbor_list,
          clexulator_compile_options, clexulator_so_options));

  // - Construct clexulator::SparseCoefficients
  jsonParser eci_json(tmp_dir / eci_relpath);
  InputParser<clexulator::SparseCoefficients> eci_parser(eci_json);
  report_and_throw_if_invalid(eci_parser, CASM::log(), error_if_invalid);

  clexulator::SparseCoefficients const &eci = *eci_parser.value;

  // - Construct ClexData for formation energy cluster expansion
  clexmonte::ClexData formation_energy_clex_data(prim_neighbor_list, clexulator,
                                                 eci);

  // - Construct system data
  std::shared_ptr<clexmonte::OccSystem> system_data =
      std::make_shared<clexmonte::OccSystem>(
          shared_prim,  // std::shared_ptr<xtal::BasicStructure const> const &
          composition_converter,  // composition::CompositionConverter const &
          formation_energy_clex_data  // ClexData const &
      );

  // ### Construct the state generator

  // - Specify the supercell transformation_matrix_to_super
  Eigen::Matrix3l transformation_matrix_to_super;
  transformation_matrix_to_super.col(0) << 10, 0, 0;
  transformation_matrix_to_super.col(1) << 0, 10, 0;
  transformation_matrix_to_super.col(2) << 0, 0, 10;

  // - Construct an initial configuration (use default DoF values)
  clexmonte::Configuration initial_configuration =
      clexmonte::make_default_configuration(*system_data,
                                            transformation_matrix_to_super);

  // - Construct a configuration generator
  auto config_generator = notstd::make_unique<
      monte::FixedConfigGenerator<clexmonte::Configuration>>(
      initial_configuration);

  // - Construct initial conditions
  monte::ValueMap initial_conditions = clexmonte::canonical::make_conditions(
      300.0,                  // temperature (K)
      composition_converter,  // composition converter
      {{"Zr", 2.},            // composition values (#/unit cell)
       {"O", 2. / 6.},
       {"Va", 10. / 6.}});

  // - Construct conditions increment
  monte::ValueMap conditions_increment =
      clexmonte::canonical::make_conditions_increment(
          10.0,                   // temperature (K)
          composition_converter,  // composition converter
          {{"Zr", 0.0},           // composition values (#/unit cell)
           {"O", 0.01},
           {"Va", -0.01}});

  // - Specify number of states (includes initial conditions)
  Index n_states = 11;

  // - Specify if dependent runs
  //   (if true, use final configuration at previous state as the
  //   initial configuration for the next state)
  bool dependent_runs = true;

  // - Specify if any conditions should be treated as "dependent"
  //   - For example, instead of setting composition as a independent
  //     condition, "mol_composition" could be a calculated from
  //     the generated configuration.
  monte::StateSamplingFunctionMap<clexmonte::Configuration>
      dependent_conditions;

  // - Construct the state generator
  monte::IncrementalConditionsStateGenerator<clexmonte::Configuration>
      state_generator(std::move(config_generator), initial_conditions,
                      conditions_increment, n_states, dependent_runs,
                      dependent_conditions);

  // ### Construct sampling functions
  monte::StateSamplingFunctionMap<clexmonte::Configuration> sampling_functions =
      clexmonte::canonical::make_sampling_functions(system_data);

  // - Add custom sampling functions if desired...
  // monte::StateSamplingFunction<Configuration> f {
  //     "potential_energy", // sampler name
  //     "Potential energy of the state (normalized per primitive cell)", //
  //     description 1,  // number of components in "potential_energy"
  //     [system_data](monte::State<Configuration> const &state) {
  //       return state.properties.at("potential_energy");
  //     });
  // sampling_functions.emplace(f.name, f);

  // ### Construct monte::SamplingParams
  monte::SamplingParams sampling_params;

  // - Sample by step, pass, or time
  sampling_params.sample_mode = monte::SAMPLE_MODE::BY_PASS;

  // - Sample linearly or logarithmically
  //
  // Default=SAMPLE_METHOD::LINEAR
  //
  // For SAMPLE_METHOD::LINEAR, take the n-th sample when:
  //
  //    sample/pass = round( begin + (period / samples_per_period) * n )
  //           time = begin + (period / samples_per_period) * n
  //
  // For SAMPLE_METHOD::LOG, take the n-th sample when:
  //
  //    sample/pass = round( begin + period ^ ( (n + shift) /
  //                      samples_per_period ) )
  //           time = begin + period ^ ( (n + shift) / samples_per_period )
  //
  sampling_params.sample_method = monte::SAMPLE_METHOD::LINEAR;
  sampling_params.begin = 0.0;
  sampling_params.period = 1.0;
  sampling_params.samples_per_period = 1.0;
  sampling_params.shift = 0.0;

  // - What sampling functions to sample
  sampling_params.sampler_names = std::vector<std::string>(
      {"temperature", "mol_composition", "param_composition",
       "formation_energy_corr", "formation_energy", "potential_energy"});

  // - Store configurations at sampling time
  sampling_params.do_sample_trajectory = false;

  // ### Construct monte::CompletionCheckParams
  monte::CompletionCheckParams completion_check_params;

  // - Set monte::CutoffCheckParams
  completion_check_params.cutoff_params.min_count = std::nullopt;
  completion_check_params.cutoff_params.max_count = 100;
  completion_check_params.cutoff_params.min_sample = std::nullopt;
  completion_check_params.cutoff_params.max_sample = std::nullopt;

  // - Set requested precision for convergence
  auto &requested_precision = completion_check_params.requested_precision;
  set_value(requested_precision, sampling_functions, "formation_energy", 0.001);
  set_value(requested_precision, sampling_functions, "formation_energy_corr",
            0.01);
  set_value_by_component_name(requested_precision, sampling_functions,
                              "mol_composition", "O", 0.01);

  // - Set other completion check parameters or use defaults
  // completion_check_params.confidence = 0.95; // default=0.95
  // completion_check_params.check_begin = 10; // default=10
  completion_check_params.check_frequency = 10;  // default=1

  // ### Construct monte::jsonResultsIO
  fs::path output_dir = tmp_dir / output_dir_relpath;
  bool write_trajectory = true;
  bool write_observations = true;
  monte::jsonResultsIO<clexmonte::Configuration> results_io(
      output_dir,          // fs::path,
      sampling_functions,  // monte::StateSamplingFunctionMap<clexmonte::Configuration>
      write_trajectory,   // bool
      write_observations  // bool
  );

  // ### Construct MTRand
  MTRand random_number_generator;

  // ~~~~ Run ~~~~

  // Create state sampler
  // - This object holds sampling functions, tracks the number of steps &
  //   passes, determines when samples are due, takes samples and holds data
  // - Use its member functions `increment_step` and `sample_data_if_due` to
  //   collect samples according to `sampling_params`
  monte::StateSampler<clexmonte::Configuration> state_sampler(
      sampling_params, sampling_functions);

  // Create CompletionCheck method
  // - This object checks for min/max cutoffs and automatic convergence
  monte::CompletionCheck completion_check(completion_check_params);

  // Create monte::MethodLog
  monte::MethodLog method_log;
  method_log.logfile_path = tmp_dir / output_dir_relpath / "status.json";
  method_log.log_frequency = 60;  // seconds

  clexmonte::canonical::run(
      system_data,              // std::shared_ptr<OccSystem>
      state_generator,          // clexmonte::canonical::state_generator_type &
      state_sampler,            // monte::StateSampler<config_type> &
      completion_check,         // monte::CompletionCheck &
      results_io,               // clexmonte::canonical::results_io_type &
      random_number_generator,  // MTRand
      method_log);
}
