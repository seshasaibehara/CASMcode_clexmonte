#include "ZrOTestSystem.hh"
#include "casm/clexmonte/canonical/conditions.hh"
#include "casm/clexmonte/system/OccSystem.hh"
#include "gtest/gtest.h"
#include "testdir.hh"

using namespace test;

class MakeCanonicalConditionsTest : public test::ZrOTestSystem {};

TEST_F(MakeCanonicalConditionsTest, Test1) {
  using namespace CASM;
  using namespace CASM::monte;
  using namespace CASM::clexmonte;

  monte::VectorValueMap conditions =
      canonical::make_conditions(300.0, system_data->composition_converter,
                                 {{"Zr", 2.0}, {"O", 1.0}, {"Va", 1.0}});

  EXPECT_EQ(conditions.size(), 2);
  EXPECT_EQ(conditions.at("temperature")(0), 300.0);
  EXPECT_EQ(conditions.at("mol_composition").size(), 3);
  EXPECT_EQ(conditions.at("mol_composition")(0), 2.0);
  EXPECT_EQ(conditions.at("mol_composition")(1), 1.0);
  EXPECT_EQ(conditions.at("mol_composition")(2), 1.0);
}

TEST_F(MakeCanonicalConditionsTest, Test2) {
  using namespace CASM;
  using namespace CASM::monte;
  using namespace CASM::clexmonte;

  monte::VectorValueMap conditions = canonical::make_conditions_increment(
      10.0, system_data->composition_converter,
      {{"Zr", 0.0}, {"O", 0.1}, {"Va", -0.1}});

  EXPECT_EQ(conditions.size(), 2);
  EXPECT_EQ(conditions.at("temperature")(0), 10.0);
  EXPECT_EQ(conditions.at("mol_composition").size(), 3);
  EXPECT_EQ(conditions.at("mol_composition")(0), 0.0);
  EXPECT_EQ(conditions.at("mol_composition")(1), -0.1);
  EXPECT_EQ(conditions.at("mol_composition")(2), 0.1);
}

TEST_F(MakeCanonicalConditionsTest, Test3) {
  using namespace CASM;
  using namespace CASM::monte;
  using namespace CASM::clexmonte;

  monte::VectorValueMap conditions = canonical::make_conditions(
      300.0, system_data->composition_converter, {{"a", 0.5}});

  EXPECT_EQ(conditions.size(), 2);
  EXPECT_EQ(conditions.at("temperature")(0), 300.0);
  EXPECT_EQ(conditions.at("mol_composition").size(), 3);
  EXPECT_EQ(conditions.at("mol_composition")(0), 2.0);
  EXPECT_EQ(conditions.at("mol_composition")(1), 1.0);
  EXPECT_EQ(conditions.at("mol_composition")(2), 1.0);
}

TEST_F(MakeCanonicalConditionsTest, Test4) {
  using namespace CASM;
  using namespace CASM::monte;
  using namespace CASM::clexmonte;

  monte::VectorValueMap conditions = canonical::make_conditions_increment(
      10.0, system_data->composition_converter, {{"a", 0.05}});

  EXPECT_EQ(conditions.size(), 2);
  EXPECT_EQ(conditions.at("temperature")(0), 10.0);
  EXPECT_EQ(conditions.at("mol_composition").size(), 3);
  EXPECT_EQ(conditions.at("mol_composition")(0), 0.0);
  EXPECT_EQ(conditions.at("mol_composition")(1), -0.1);
  EXPECT_EQ(conditions.at("mol_composition")(2), 0.1);
}
