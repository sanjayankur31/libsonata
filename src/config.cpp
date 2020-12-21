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

#include <cassert>
#include <fstream>
#include <memory>
#include <regex>
#include <string>

#include <fmt/format.h>
#include <ghc/filesystem.hpp>
#include <json.hpp>

namespace {
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

namespace fs = ghc::filesystem;

class PathResolver
{
  public:
    PathResolver(const std::string& basePath)
        : _basePath(fs::path(basePath)) {}

    std::string toAbsolute(const std::string& pathStr) const {
        const fs::path path(pathStr);
        const auto absolute = path.is_absolute() ? path : fs::absolute(path / _basePath);
        return absolute.lexically_normal().string();
    }

  private:
    const fs::path _basePath;
};


std::map<std::string, std::string> _replaceVariables(std::map<std::string, std::string> variables) {
    constexpr size_t maxIterations = 10;

    bool anyChange = true;
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
        ++iteration;
        variables = variablesCopy;
    }

    if (iteration == maxIterations) {
        throw SonataError(
            "Reached maximum allowed iterations in variable expansion, "
            "possibly infinite recursion.");
    }

    return variables;
}

nlohmann::json _expandVariables(const nlohmann::json& json,
                                const std::map<std::string, std::string>& vars) {
    auto jsonFlat = json.flatten();

    // Expand variables in whole json
    for (auto it = jsonFlat.begin(); it != jsonFlat.end(); ++it) {
        auto& value = it.value();
        if (!value.is_string())
            continue;

        auto valueStr = value.get<std::string>();

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


std::map<std::string, std::string> _fillComponents(const nlohmann::json& json,
                                                   const PathResolver& resolver) {
    const auto comps = json.at("components");
    std::map<std::string, std::string> output;

    for (auto it = comps.begin(); it != comps.end(); ++it)
        output[it.key()] = resolver.toAbsolute(it.value());

    return output;
}

using bbp::sonata::CircuitConfig;

std::vector<CircuitConfig::SubnetworkFiles> _fillSubnetwork(nlohmann::json::reference networks,
                                                            const std::string& prefix,
                                                            const PathResolver& resolver) {
    std::vector<CircuitConfig::SubnetworkFiles> output;

    const std::string component = prefix + "s";
    const std::string elementsFile = prefix + "s_file";
    const std::string typesFile = prefix + "_types_file";

    auto iter = networks.find(component);
    if (iter == networks.end())
        return output;

    for (const auto& node : *iter) {
        output.emplace_back(
            CircuitConfig::SubnetworkFiles{resolver.toAbsolute(node.at(elementsFile)),
                                           "" /*resolver.toAbsolute(node.at(typesFile))*/});
    }

    return output;
}

const char* _defaultSpikesFileName = "spikes.h5";

std::map<std::string, std::string> _readVariables(const nlohmann::json& json) {
    auto manifest = json["manifest"];

    std::map<std::string, std::string> variables;

    const std::regex regexVariable(R"(\$[a-zA-Z0-9_]*)");

    // Find variables in manifest section
    for (auto it = manifest.begin(); it != manifest.end(); ++it) {
        const auto name = it.key();

        if (std::regex_match(name, regexVariable)) {
            assert(variables.find(name) == variables.end());
            variables[name] = it.value();
        } else {
            throw SonataError(fmt::format("Invalid variable `{}`", name));
        }
    }

    return variables;
}

nlohmann::json parseSonataJson(const std::string& contents) {
    const auto json = nlohmann::json::parse(contents);

    const auto vars = _replaceVariables(_readVariables(json));
    return _expandVariables(json, vars);
}

}  // namespace


namespace bbp {
namespace sonata {

struct CircuitConfig::Impl {
    std::string target_simulator;
    std::map<std::string, std::string> component_dirs;
    std::vector<CircuitConfig::SubnetworkFiles> networkNodes;
    std::vector<CircuitConfig::SubnetworkFiles> networkEdges;
    PathResolver resolver;

    Impl(const std::string& contents, const std::string& basePath)
        : resolver(basePath) {
        const auto json = parseSonataJson(contents);

        try {
            target_simulator = json.at("target_simulator");
        } catch (nlohmann::detail::out_of_range&) {
        }

        if (json.find("networks") == json.end())
            throw SonataError("Error parsing config: `networks` not specified");

        auto networks = json.at("networks");
        component_dirs = _fillComponents(json, resolver);
        networkEdges = _fillSubnetwork(networks, "edge", resolver);
        networkNodes = _fillSubnetwork(networks, "node", resolver);
    }
};

CircuitConfig::CircuitConfig(const std::string& contents, const std::string& basePath)
    : impl(new CircuitConfig::Impl(contents, basePath)) {}

CircuitConfig::CircuitConfig(CircuitConfig&&) = default;
CircuitConfig::~CircuitConfig() = default;

CircuitConfig CircuitConfig::fromFile(const std::string& path) {
    std::string contents = readFile(path);
    std::string basePath{'.'};  // XXX: use real basepath!
    return CircuitConfig(contents, basePath);
}

std::string CircuitConfig::getTargetSimulator() const {
    return impl->target_simulator;
}

std::string CircuitConfig::getComponentPath(const std::string& name) const {
    const auto it = impl->component_dirs.find(name);
    if (it == impl->component_dirs.end()) {
        throw SonataError(fmt::format("Could not find component '{}'", name));
    }

    return it->second;
}

const std::vector<CircuitConfig::SubnetworkFiles>& CircuitConfig::getNodes() const {
    return impl->networkNodes;
}

const std::vector<CircuitConfig::SubnetworkFiles>& CircuitConfig::getEdges() const {
    return impl->networkEdges;
}

struct SimulationConfig::Impl {
    PathResolver _resolver;
    std::string networkConfig;
    std::string nodeSets;
    fs::path outputDir;
    std::string spikesFile;
    std::map<std::string, std::string> reportFilepaths;

    Impl(const std::string& contents, const std::string& basePath)
        : _resolver(basePath) {
        const auto json = parseSonataJson(contents);

        if (json.find("network") == json.end())
            throw SonataError("Error parsing simulation config: network not specified");

        networkConfig = _resolver.toAbsolute(json.at("network"));

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
    std::string contents = readFile(path);
    std::string basePath{'.'};  // XXX: use real basepath!

    return SimulationConfig(contents, basePath);
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
    const auto i = _impl->reportFilepaths.find(name);
    if (i == _impl->reportFilepaths.end()) {
        throw SonataError(fmt::format("Unknown report: `{}`", name));
    }
    return i->second;
}

}  // namespace sonata
}  // namespace bbp
