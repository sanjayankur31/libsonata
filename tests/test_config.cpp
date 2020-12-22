#include <catch2/catch.hpp>

#include <bbp/sonata/config.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>


using namespace bbp::sonata;

TEST_CASE("CircuitConfig") {
    SECTION("Simple") {
        const auto config = CircuitConfig::fromFile("./data/config/circuit_config.json");

        CHECK(config.getTargetSimulator() == "NEURON");
        CHECK(config.getNodeSetsPath() == "./node_sets.json");
        CHECK(config.listComponents() == 
              std::set<std::string>{"biophysical_neuron_models_dir", "morphologies_dir"});

        CHECK_THROWS_AS(config.getComponent("DoesNotExist"), SonataError);
    }

    SECTION("Exception") {
        CHECK_THROWS_AS(CircuitConfig::fromFile("/file/does/not/exist"), SonataError);

        { // Missing 'networks'
            auto contents = R"({ "manifest": {} })";
            CHECK_THROWS_AS(CircuitConfig(contents, "./"), SonataError);
        }

        { // Self recursion
            auto contents = R"({
              "manifest": { "$DIR": "$DIR" },
              "networks": {}
            })";
            CHECK_THROWS_AS(CircuitConfig(contents, "./"), SonataError);
        }

        { // mutual recursion
            auto contents = R"({
              "manifest": {
                "$FOO": "$BAR",
                "$BAR": "$FOO"
              },
              "networks": {}
            })";
            CHECK_THROWS_AS(CircuitConfig(contents, "./"), SonataError);
        }

        { // Invalid variable name
            auto contents = R"({
              "manifest": {
                "$FOO[]": "InvalidVariableName"
              },
              "networks": {}
            })";
            CHECK_THROWS_AS(CircuitConfig(contents, "./"), SonataError);
        }
    }
}

TEST_CASE("SimulationConfig") {
    SECTION("Simple") {
        const auto config = SimulationConfig::fromFile("./data/config/simulation_config.json");
    }

    SECTION("Exception") {
        CHECK_THROWS_AS(SimulationConfig::fromFile("/file/does/not/exist"), SonataError);
    }
}
