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
    }
}

TEST_CASE("SimulationConfig") {
    SECTION("Simple") {
        const auto config = SimulationConfig::fromFile("./data/config/simulation_config.json");
    }
}
