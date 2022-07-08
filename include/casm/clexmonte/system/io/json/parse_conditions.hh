#ifndef CASM_clexmonte_system_parse_conditions
#define CASM_clexmonte_system_parse_conditions

#include "casm/casm_io/container/json_io.hh"
#include "casm/casm_io/json/InputParser_impl.hh"
#include "casm/clexmonte/misc/eigen.hh"
#include "casm/clexmonte/system/make_conditions.hh"
#include "casm/composition/CompositionConverter.hh"
#include "casm/composition/io/json/CompositionConverter_json_io.hh"
#include "casm/monte/definitions.hh"

namespace CASM {
namespace clexmonte {

/// \brief Parse temperature scalar value
void parse_temperature(InputParser<monte::ValueMap> &parser);

/// \brief Parse "mol_composition" or "param_composition" and store as
///     "mol_composition" vector values
template <typename SystemType>
void parse_mol_composition(InputParser<monte::ValueMap> &parser,
                           std::shared_ptr<SystemType> const &system_data);

/// \brief Parse "mol_composition" or "param_composition" and store as
///     "mol_composition" vector values (increment)
template <typename SystemType>
void parse_mol_composition_increment(
    InputParser<monte::ValueMap> &parser,
    std::shared_ptr<SystemType> const &system_data);

// --- Inline definitions ---

/// \brief Parse temperature scalar value
///
/// If successfully parsed, `parser->value` will contain a
/// monte::ValueMap with:
/// - scalar_values["temperature"]: (size 1)
///
/// If unsuccesfully parsed, `parser.valid() == false`.
///
/// Expected:
///
///   "temperature": number (required)
///     Temperature in K.
///
/// \param parser InputParser, which must have non-empty value
///
inline void parse_temperature(InputParser<monte::ValueMap> &parser) {
  if (parser.value == nullptr) {
    throw std::runtime_error(
        "Error in parse_temperature: parser must have non-empty value");
  }
  parser.require(parser.value->scalar_values["temperature"], "temperature");
}

/// \brief Parse "mol_composition" or "param_composition" and store as
/// "mol_composition" vector values
///
/// If successfully parsed, `parser->value` will contain a
/// monte::ValueMap with:
/// - vector_values["mol_composition"]: (size = system components size)
///
/// If unsuccesfully parsed, `parser.valid() == false`.
///
/// Expected:
///
///   "mol_composition": dict (optional)
///     Composition in number per primitive cell. A dict, where the keys are
///     the component names, and values are the number of that component per
///     primitive cell. All components in the system must be included. Must sum
///     to number of sites per prim cell.
///
///   "param_composition": array of number or dict (optional)
///     Parametric composition, in terms of the chosen composition axes. Will
///     be converted to `"mol_composition"`. A dict, where the keys are the
///     axes names ("a", "b", etc.), and values are the corresponding parametric
///     composition value. All composition axes must be included.
///
///
/// Requires:
/// - get_composition_converter(SystemType const &system_data);
template <typename SystemType>
void parse_mol_composition(InputParser<monte::ValueMap> &parser,
                           std::shared_ptr<SystemType> const &system_data) {
  if (parser.value == nullptr) {
    throw std::runtime_error(
        "Error in parse_mol_composition: parser must have non-empty value");
  }
  std::map<std::string, double> input;
  std::string option;
  try {
    if (parser.self.contains("mol_composition")) {
      option = "mol_composition";
    } else if (parser.self.contains("param_composition")) {
      option = "param_composition";
    } else {
      parser.error.insert(
          "Missing one of \"mol_composition\" or \"param_composition\"");
    }
    parser.optional(input, option);
    parser.value->vector_values["mol_composition"] =
        make_mol_composition(get_composition_converter(*system_data), input);
  } catch (std::exception &e) {
    std::stringstream msg;
    msg << "Error: could not construct composition from option '" << option
        << "'.";
    parser.insert_error(option, e.what());
  }
}

/// \brief Parse "mol_composition" or "param_composition" and store as
///     "mol_composition" vector values (increment)
///
/// If successfully parsed, `parser->value` will contain a
/// monte::ValueMap with:
/// - vector_values["mol_composition"]: (size = system components size)
///
/// If unsuccesfully parsed, `parser.valid() == false`.
///
/// Expected:
///
///   "mol_composition": dict (optional)
///     Composition increment in number per primitive cell. A dict, where the
///     keys are the component names, and values are the number of that
///     component per primitive cell. All components in the system must be
///     included. Must sum to zero.
///
///   "param_composition": array of number or dict (optional)
///     Parametric composition, in terms of the chosen composition axes. Will
///     be converted to `"mol_composition"`. A dict, where the keys are the
///     axes names ("a", "b", etc.), and values are the corresponding parametric
///     composition value. All composition axes must be included.
///
///
/// Requires:
/// - get_composition_converter(SystemType const &system_data);
template <typename SystemType>
void parse_mol_composition_increment(
    InputParser<monte::ValueMap> &parser,
    std::shared_ptr<SystemType> const &system_data) {
  if (parser.value == nullptr) {
    throw std::runtime_error(
        "Error in parse_mol_composition_increment: parser must have non-empty "
        "value");
  }
  std::map<std::string, double> input;
  std::string option;
  try {
    if (parser.self.contains("mol_composition")) {
      option = "mol_composition";
    } else if (parser.self.contains("param_composition")) {
      option = "param_composition";
    } else {
      parser.error.insert(
          "Missing one of \"mol_composition\" or \"param_composition\"");
    }
    parser.optional(input, option);
    parser.value->vector_values["mol_composition"] =
        make_mol_composition_increment(get_composition_converter(*system_data),
                                       input);
  } catch (std::exception &e) {
    std::stringstream msg;
    msg << "Error: could not construct composition increment from option '"
        << option << "'.";
    parser.insert_error(option, e.what());
  }
}

}  // namespace clexmonte
}  // namespace CASM

#endif
