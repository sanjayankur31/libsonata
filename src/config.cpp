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

#include <fstream>
#include <memory>
#include <regex>
#include <streambuf>
#include <string>

#include <ghc/filesystem.hpp>  // for ghc::filesystem
#include <json.hpp>


namespace {
nlohmann::json _parseCircuitJson(const std::string& jsonStr) {
    using nlohmann::json;

    const auto jsonOrig = json::parse(jsonStr);
    auto jsonFlat = jsonOrig.flatten();
    auto manifest = jsonOrig["manifest"];

    std::map<std::string, std::string> variables;

    const std::regex regexVariable("\\$[a-zA-Z0-9_]*");

    // Find variables in manifest section
    for (auto it = manifest.begin(); it != manifest.end(); ++it) {
        const auto name = it.key();

        if (std::regex_match(name, regexVariable)) {
            if (variables.find(name) != variables.end())
                throw std::runtime_error("Duplicate variable `" + name + "`");

            variables[name] = it.value();
        } else {
            throw std::runtime_error("Invalid variable name `" + name + "`");
        }
    }

    {  // Expand variables dependent on other variables
        bool anyChange = true;
        constexpr size_t max_iterations = 5;
        size_t iteration = 0;

        while (anyChange && iteration < max_iterations) {
            anyChange = false;
            auto variablesCopy = variables;

            for (const auto& vI : variables) {
                const auto& vIKey = vI.first;
                const auto& vIValue = vI.second;

                for (auto& vJ : variablesCopy) {
                    auto& vJValue = vJ.second;
                    auto startPos = vJValue.find(vIKey);

                    if (startPos != std::string::npos) {
                        vJValue.replace(startPos, vIKey.length(), vIValue);
                        anyChange = true;
                    }
                }
            }

            variables = variablesCopy;
            iteration++;
        }

        if (iteration == max_iterations)
            throw std::runtime_error(
                "Reached maximum allowed iterations in variable expansion, "
                "possibly infinite recursion.");
    }

    // Expand variables in whole json
    for (auto it = jsonFlat.begin(); it != jsonFlat.end(); ++it) {
        if (!it.value().is_string())
            continue;

        auto valueStr = it.value().get<std::string>();
        auto& value = it.value();

        for (auto& var : variables) {
            auto& varName = var.first;
            auto& varValue = var.second;
            auto startPos = valueStr.find(varName);

            if (startPos != std::string::npos) {
                valueStr.replace(startPos, varName.length(), varValue);
                value = valueStr;
            }
        }
    }

    return jsonFlat.unflatten();
}

std::map<std::string, std::string> _fillComponents(const nlohmann::json& json) {
    const auto comps = json["components_dir"];
    std::map<std::string, std::string> output;

    for (auto it = comps.begin(); it != comps.end(); ++it)
        output[it.key()] = it.value();

    return output;
}

std::vector<bbp::sonata::CircuitConfig::SubnetworkFiles> _fillSubnetwork(
    const nlohmann::json& json,
    const std::string& network_type,
    const std::string& element_name,
    const std::string& type_name) {
    std::vector<bbp::sonata::CircuitConfig::SubnetworkFiles> output;

    const auto nodes = json["networks"][network_type];

    for (const auto& node : nodes) {
        bbp::sonata::CircuitConfig::SubnetworkFiles network;
        network.elements = node[element_name];
        network.types = node[type_name];
        output.push_back(network);
    }

    return output;
}
}  // namespace


namespace bbp {
namespace sonata {

struct CircuitConfig::Impl {
    Impl(const std::string& path) {
        std::ifstream file(path);

        if (file.fail())
            throw std::runtime_error("Could not open file `" + path + "`");

        std::string contents;

        file.seekg(0, std::ios::end);
        contents.reserve(file.tellg());
        file.seekg(0, std::ios::beg);

        contents.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        const auto json = _parseCircuitJson(contents);
        target_simulator = json["target_simulator"];
        component_dirs = _fillComponents(json);
        networkEdges = _fillSubnetwork(json, "edges", "edges_file", "edge_types_file");
        networkNodes = _fillSubnetwork(json, "nodes", "nodes_file", "node_types_file");
    }

    std::string target_simulator;
    std::map<std::string, std::string> component_dirs;
    std::vector<CircuitConfig::SubnetworkFiles> networkNodes;
    std::vector<CircuitConfig::SubnetworkFiles> networkEdges;
};

CircuitConfig::CircuitConfig(const std::string& path)
    : impl(new CircuitConfig::Impl(path)) {}

CircuitConfig::CircuitConfig(CircuitConfig&&) = default;
CircuitConfig::~CircuitConfig() = default;

std::string CircuitConfig::getTargetSimulator() const {
    return impl->target_simulator;
}

std::string CircuitConfig::getComponentPath(const std::string& name) const {
    const auto it = impl->component_dirs.find(name);
    if (it == impl->component_dirs.end())
        throw std::runtime_error("Could not find component '" + name + "'");

    return it->second;
}

const std::vector<CircuitConfig::SubnetworkFiles>& CircuitConfig::getNodes() const {
    return impl->networkNodes;
}
const std::vector<CircuitConfig::SubnetworkFiles>& CircuitConfig::getEdges() const {
    return impl->networkEdges;
}

}  // namespace sonata
}  // namespace bbp

// ****************************************************************************

namespace {
namespace fs = ghc::filesystem;
const std::string _defaultSpikesFileName("spikes.h5");

std::map<std::string, std::string> _readVariables(const nlohmann::json& json) {
    auto manifest = json["manifest"];

    std::map<std::string, std::string> variables;

    const std::regex regexVariable("\\$[a-zA-Z0-9_]*");

    // Find variables in manifest section
    for (auto it = manifest.begin(); it != manifest.end(); ++it) {
        const auto name = it.key();

        if (std::regex_match(name, regexVariable)) {
            if (variables.find(name) != variables.end())
                throw std::runtime_error("Duplicate variable `" + name + "`");

            variables[name] = it.value();
        } else {
            throw std::runtime_error("Invalid variable name `" + name + "`");
        }
    }

    return variables;
}

std::map<std::string, std::string> _replaceVariables(std::map<std::string, std::string> variables) {
    bool anyChange = true;
    constexpr size_t maxIterations = 5;
    size_t iteration = 0;

    while (anyChange && iteration < maxIterations) {
        anyChange = false;
        auto variablesCopy = variables;

        for (const auto& vI : variables) {
            const auto& vIKey = vI.first;
            const auto& vIValue = vI.second;

            for (auto& vJ : variablesCopy) {
                auto& vJValue = vJ.second;
                auto startPos = vJValue.find(vIKey);

                if (startPos != std::string::npos) {
                    vJValue.replace(startPos, vIKey.length(), vIValue);
                    anyChange = true;
                }
            }
        }

        variables = variablesCopy;
        iteration++;
    }

    if (iteration == maxIterations)
        throw std::runtime_error(
            "Reached maximum allowed iterations in variable expansion, "
            "possibly infinite recursion.");

    return variables;
}

nlohmann::json _expandVariables(const nlohmann::json& json,
                                const std::map<std::string, std::string>& vars) {
    auto jsonFlat = json.flatten();

    // Expand variables in whole json
    for (auto it = jsonFlat.begin(); it != jsonFlat.end(); ++it) {
        if (!it.value().is_string())
            continue;

        auto valueStr = it.value().get<std::string>();
        auto& value = it.value();

        for (auto& var : vars) {
            auto& varName = var.first;
            auto& varValue = var.second;
            auto startPos = valueStr.find(varName);

            if (startPos != std::string::npos) {
                valueStr.replace(startPos, varName.length(), varValue);
                value = valueStr;
            }
        }
    }

    return jsonFlat.unflatten();
}

nlohmann::json parseSonataJson(const std::string& uri) {
    // Reading the input file into a string
    std::ifstream file(uri);
    if (file.fail())
        throw std::runtime_error("Could not open file `" + uri + "`");

    // Parsing
    const auto json = nlohmann::json::parse(file);

    // Parsing manifest and expanding all variables
    const auto vars = _replaceVariables(_readVariables(json));
    return _expandVariables(json, vars);
}
class PathResolver
{
  public:
    PathResolver(const std::string& basePath)
        : _basePath(fs::path(basePath).parent_path()) {}

    std::string toAbsolute(const std::string& path) const;

  private:
    const fs::path _basePath;
};

std::string PathResolver::toAbsolute(const std::string& pathStr) const {
    const fs::path path(pathStr);
    const auto absolute = path.is_absolute() ? path : fs::absolute(path / _basePath);
    return absolute.lexically_normal().string();
}
}  // namespace

namespace bbp {
namespace sonata {

namespace fs = ghc::filesystem;

struct SimulationConfig::Impl {
    std::string networkConfig;
    std::string nodeSets;
    fs::path outputDir;
    std::string spikesFile;
    std::map<std::string, std::string> reportFilepaths;

    Impl(const std::string& path)
        : _resolver(path) {
        const auto json = parseSonataJson(path);

        try {
            networkConfig = _resolver.toAbsolute(json.at("network"));
        } catch (nlohmann::detail::exception& e) {
            // Check if this configuration is a circuit configuration as well.
            // Otherwise report an error about the missing network field.
            if (json.find("networks") == json.end())
                throw std::runtime_error("Error parsing simulation config: network not specified");
            networkConfig = path;
        }

        if (json.find("node_sets_file") != json.end())
            nodeSets = _resolver.toAbsolute(json["node_sets_file"]);

        try {
            const auto& output = json.at("output");
            outputDir = fs::path(_resolver.toAbsolute(output.at("output_dir")));

            auto reference = output.find("spikes_file");
            if (reference == output.end())
                spikesFile = _resolver.toAbsolute((outputDir / _defaultSpikesFileName).string());
            else {
                const std::string filename = *reference;
                spikesFile = _resolver.toAbsolute((outputDir / filename).string());
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
                        reportFilepaths[name] = _resolver.toAbsolute(
                            (outputDir / filename).string());
                    } else {
                        reportFilepaths[name] = _resolver.toAbsolute(
                            (outputDir / fs::path(name + ".h5")).string());
                    }
                }
            }
        } catch (nlohmann::detail::exception& e) {
            throw std::runtime_error(
                (std::string("Error parsing simulation config: ") + e.what()).c_str());
        }
    }

  private:
    PathResolver _resolver;
};

SimulationConfig::SimulationConfig(const std::string& source)
    : _impl(new SimulationConfig::Impl(source)) {}

SimulationConfig::SimulationConfig(SimulationConfig&&) = default;
SimulationConfig& SimulationConfig::operator=(SimulationConfig&&) = default;
SimulationConfig::~SimulationConfig() = default;

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
    for (const auto& item : _impl->reportFilepaths)
        names.push_back(item.first);
    return names;
}

std::string SimulationConfig::getCompartmentReportFilepath(const std::string& name) const {
    const auto i = _impl->reportFilepaths.find(name);
    if (i == _impl->reportFilepaths.end())
        throw std::runtime_error("Unknown report: " + name);
    return i->second;
}
}  // namespace sonata
}  // namespace bbp
