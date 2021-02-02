/*************************************************************************
 * Copyright (C) 2018-2020 Blue Brain Project
 *                         Jonas Karlsson <jonas.karlsson@epfl.ch>
 *                         Juan Hernando <juan.hernando@epfl.ch>
 *
 * This file is part of 'libsonata', distributed under the terms
 * of the GNU Lesser General Public License version 3.
 *
 * See top-level COPYING.LESSER and COPYING files for details.
 *************************************************************************/

#include <bbp/sonata/config.h>

#include <algorithm> // transform
#include <cassert>
#include <fstream>
#include <memory>
#include <regex>
#include <string>
#include <set>

#include <fmt/format.h>
#include <ghc/filesystem.hpp>
#include <json.hpp>

#include "population.hpp"


namespace {
using bbp::sonata::CircuitConfig;
using bbp::sonata::SonataError;


std::string readFile(const std::string& path) {
    std::ifstream file(path);

    if (file.fail()) {
        throw SonataError(fmt::format("Could not open file `{}`", path));
    }

    std::string contents;

    file.seekg(0, std::ios::end);
    contents.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    contents.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    return contents;
}

// to be replaced by std::filesystem once C++17 is used
namespace fs = ghc::filesystem;

class PathResolver
{
  public:
    PathResolver(const std::string& basePath)
        : _basePath(fs::absolute(fs::path(basePath))) {}

    std::string toAbsolute(const std::string& pathStr) const {
        const fs::path path(pathStr);
        const auto absolute = path.is_absolute() ? path : fs::absolute(_basePath / path);
        return absolute.lexically_normal().string();
    }

  private:
    const fs::path _basePath;
};


std::map<std::string, std::string> replaceVariables(std::map<std::string, std::string> variables) {
    constexpr size_t maxIterations = 10;

    bool anyChange = true;
    size_t iteration = 0;

    while (anyChange) {
        anyChange = false;
        auto variablesCopy = variables;

        for (const auto& vI : variables) {
            const auto& vIKey = vI.first;
            const auto& vIValue = vI.second;

            for (auto& vJ : variablesCopy) {
                auto& vJValue = vJ.second;
                const auto startPos = vJValue.find(vIKey);

                if (startPos != std::string::npos) {
                    vJValue.replace(startPos, vIKey.length(), vIValue);
                    anyChange = true;
                }
            }
        }
        variables = variablesCopy;

        if (++iteration == maxIterations) {
            throw SonataError(
                "Reached maximum allowed iterations in variable expansion, "
                "possibly infinite recursion.");
        }
    }

    return variables;
}

nlohmann::json expandVariables(const nlohmann::json& json,
                               const std::map<std::string, std::string>& vars) {
    auto jsonFlat = json.flatten();

    // Expand variables in whole json
    for (auto it = jsonFlat.begin(); it != jsonFlat.end(); ++it) {
        auto& value = it.value();
        if (!value.is_string()) {
            continue;
        }

        auto valueStr = value.get<std::string>();

        for (auto& var : vars) {
            const auto& varName = var.first;
            const auto& varValue = var.second;
            const auto startPos = valueStr.find(varName);

            if (startPos != std::string::npos) {
                valueStr.replace(startPos, varName.length(), varValue);
                value = valueStr;
            }
        }
    }

    return jsonFlat.unflatten();
}


std::map<std::string, std::string> fillComponents(const nlohmann::json& json,
                                                  const PathResolver& resolver) {
    const auto components = json.at("components");
    std::map<std::string, std::string> result;

    for (auto it = components.begin(); it != components.end(); ++it) {
        result[it.key()] = resolver.toAbsolute(it.value());
    }

    return result;
}

struct SubnetworkFiles {
    std::string elements;
    std::string types;
};

std::vector<SubnetworkFiles> fillSubnetwork(nlohmann::json& networks,
                                            const std::string& prefix,
                                            const PathResolver& resolver) {
    std::vector<SubnetworkFiles> output;

    const std::string component = prefix + "s";
    const std::string elementsFile = prefix + "s_file";
    const std::string typesFile = prefix + "_types_file";

    const auto iter = networks.find(component);
    if (iter == networks.end()) {
        return output;
    }

    for (const auto& node : *iter) {
        auto h5File = resolver.toAbsolute(node.at(elementsFile));
        auto csvFile = node.at(typesFile).is_null() ? std::string()
                                                    : resolver.toAbsolute(node.at(typesFile));
        output.emplace_back(SubnetworkFiles{h5File, csvFile});
    }

    return output;
}

const char* _defaultSpikesFileName = "spikes.h5";

using Variables = std::map<std::string, std::string>;

Variables readVariables(const nlohmann::json& json) {
    Variables variables;

    if (json.find("networks") == json.end()) {
        return variables;
    }

    const auto manifest = json["manifest"];

    const std::regex regexVariable(R"(\$[a-zA-Z0-9_]*)");

    for (auto it = manifest.begin(); it != manifest.end(); ++it) {
        const auto name = it.key();

        if (std::regex_match(name, regexVariable)) {
            variables[name] = it.value();
        } else {
            throw SonataError(fmt::format("Invalid variable `{}`", name));
        }
    }

    return variables;
}

nlohmann::json parseSonataJson(const std::string& contents) {
    const auto json = nlohmann::json::parse(contents);

    const auto vars = replaceVariables(readVariables(json));
    return expandVariables(json, vars);
}


template <typename Population>
std::map<std::string, SubnetworkFiles> resolvePopulations(
    const std::vector<SubnetworkFiles>& networkNodes) {
    using PopulationStorage = bbp::sonata::PopulationStorage<Population>;
    std::map<std::string, SubnetworkFiles> result;

    for (const auto& network : networkNodes) {
        const auto population = PopulationStorage(network.elements, network.types);
        for (auto name : population.populationNames()) {
            result[name] = network;
        }
    }
    return result;
}

template <typename Population>
std::set<std::string> listPopulations(const std::vector<SubnetworkFiles>& networkNodes) {
    std::set<std::string> result;
    const auto names = resolvePopulations<Population>(networkNodes);
    std::transform(names.begin(),
                   names.end(),
                   std::inserter(result, result.end()),
                   [](std::pair<std::string, SubnetworkFiles> p) { return p.first; });
    return result;
}
}  // namespace


namespace bbp {
namespace sonata {

struct CircuitConfig::Impl {
    PathResolver resolver;

    std::string target_simulator;
    std::string node_sets_file;
    std::map<std::string, std::string> components;
    std::vector<SubnetworkFiles> networkNodes;
    std::vector<SubnetworkFiles> networkEdges;

    Impl(const std::string& contents, const std::string& basePath)
        : resolver(basePath) {
        const auto json = parseSonataJson(contents);

        try {
            target_simulator = json.at("target_simulator");
        } catch (nlohmann::detail::out_of_range&) {
        }

        if (json.find("node_sets_file") != json.end()) {
            node_sets_file = resolver.toAbsolute(json["node_sets_file"]);
        }

        if (json.find("networks") == json.end()) {
            throw SonataError("Error parsing config: `networks` not specified");
        }

        auto networks = json.at("networks");
        components = fillComponents(json, resolver);
        networkNodes = fillSubnetwork(networks, "node", resolver);
        networkEdges = fillSubnetwork(networks, "edge", resolver);
    }
};

CircuitConfig::CircuitConfig(const std::string& contents, const std::string& basePath)
    : impl(new CircuitConfig::Impl(contents, basePath)) {}

CircuitConfig::CircuitConfig(CircuitConfig&&) = default;
CircuitConfig::~CircuitConfig() = default;

CircuitConfig CircuitConfig::fromFile(const std::string& path) {
    return CircuitConfig(readFile(path), fs::path(path).parent_path());
}

std::string CircuitConfig::getTargetSimulator() const {
    return impl->target_simulator;
}

std::string CircuitConfig::getNodeSetsPath() const {
    return impl->node_sets_file;
}

std::set<std::string> CircuitConfig::listNodePopulations() const {
    return listPopulations<NodePopulation>(impl->networkNodes);
}

template <typename Population>
Population getPopulation(const std::vector<SubnetworkFiles>& network, const std::string name) {
    const auto names = resolvePopulations<Population>(network);
    const auto it = names.find(name);
    if (it == names.end()) {
        throw SonataError(fmt::format("Could not find population '{}'", name));
    }

    return Population(it->second.elements, it->second.types, name);
}

NodePopulation CircuitConfig::getNodePopulation(const std::string& name) const {
    return getPopulation<NodePopulation>(impl->networkNodes, name);
}

std::set<std::string> CircuitConfig::listEdgePopulations() const {
    return listPopulations<EdgePopulation>(impl->networkEdges);
}

EdgePopulation CircuitConfig::getEdgePopulation(const std::string& name) const {
    return getPopulation<EdgePopulation>(impl->networkEdges, name);
}

std::set<std::string> CircuitConfig::listComponents() const {
    std::set<std::string> result;
    std::transform(impl->components.begin(), impl->components.end(),
                   std::inserter(result, result.end()),
                   [](std::pair<std::string, std::string> p){ return p.first; });
    return result;
}

std::string CircuitConfig::getComponent(const std::string& name) const {
    const auto it = impl->components.find(name);
    if (it == impl->components.end()) {
        throw SonataError(fmt::format("Could not find component '{}'", name));
    }

    return it->second;
}

struct SimulationConfig::Impl {
    PathResolver resolver;

    std::string networkConfig;
    std::string nodeSets;
    fs::path outputDir;
    std::string spikesFile;
    std::map<std::string, std::string> reportFilepaths;

    Impl(const std::string& contents, const std::string& basePath)
        : resolver(basePath) {
        const auto json = parseSonataJson(contents);

        if (json.find("network") == json.end()) {
            throw SonataError("Error parsing simulation config: network not specified");
        }

        networkConfig = resolver.toAbsolute(json.at("network"));

        if (json.find("node_sets_file") != json.end()) {
            nodeSets = resolver.toAbsolute(json["node_sets_file"]);
        }

        try {
            const auto& output = json.at("output");
            outputDir = fs::path(resolver.toAbsolute(output.at("output_dir")));

            auto reference = output.find("spikes_file");
            if (reference == output.end()) {
                spikesFile = resolver.toAbsolute((outputDir / _defaultSpikesFileName).string());
            } else {
                const std::string filename = *reference;
                spikesFile = resolver.toAbsolute((outputDir / filename).string());
            }

            const auto reports = json.find("reports");
            // Can't use range-based for
            if (reports != json.end()) {
                for (auto report = reports->begin(); report != reports->end(); ++report) {
                    // Consider only reports with module "membrane_report"
                    reference = report->find("module");
                    if (reference == report->end() || *reference != "membrane_report") {
                        continue;
                    }

                    const auto& name = report.key();
                    reference = report->find("file_name");
                    if (reference != report->end()) {
                        const std::string filename = *reference;
                        reportFilepaths[name] = resolver.toAbsolute(
                            (outputDir / filename).string());
                    } else {
                        reportFilepaths[name] = resolver.toAbsolute(
                            (outputDir / fs::path(name + ".h5")).string());
                    }
                }
            }
        } catch (nlohmann::detail::exception& e) {
            throw SonataError(fmt::format("Error parsing simulation config: {}", e.what()));
        }
    }
};

SimulationConfig::SimulationConfig(const std::string& source, const std::string& basePath)
    : _impl(new SimulationConfig::Impl(source, basePath)) {}

SimulationConfig::SimulationConfig(SimulationConfig&&) = default;
SimulationConfig& SimulationConfig::operator=(SimulationConfig&&) = default;
SimulationConfig::~SimulationConfig() = default;

SimulationConfig SimulationConfig::fromFile(const std::string& path) {
    return SimulationConfig(readFile(path), fs::path(path).parent_path());
}

std::string SimulationConfig::getNetworkConfig() const {
    return _impl->networkConfig;
}

std::string SimulationConfig::getNodeSetFilepath() const {
    return _impl->nodeSets;
}

std::string SimulationConfig::getSpikesFilepath() const {
    return _impl->spikesFile;
}

std::vector<std::string> SimulationConfig::getCompartmentReportNames() const {
    std::vector<std::string> names;
    for (const auto& item : _impl->reportFilepaths) {
        names.push_back(item.first);
    }
    return names;
}

std::string SimulationConfig::getCompartmentReportFilepath(const std::string& name) const {
    const auto it = _impl->reportFilepaths.find(name);
    if (it == _impl->reportFilepaths.end()) {
        throw SonataError(fmt::format("Unknown report: `{}`", name));
    }
    return it->second;
}

}  // namespace sonata
}  // namespace bbp