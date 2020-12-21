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

#pragma once

#include <memory>  // std::unique_ptr
#include <string>
#include <vector>

#include "common.h"


namespace bbp {
namespace sonata {


/** Read access to a SONATA circuit config file.
 */
class SONATA_API CircuitConfig
{
  public:
    struct SubnetworkFiles {
        std::string elements;
        std::string types;
    };

    /** Load SONATA circuit config JSON
     *
     * @param std::string contents of a circuit config JSON file
     * @throw std::runtime_error if file is not found or invalid
     */
    CircuitConfig(const std::string& contents);

    CircuitConfig(CircuitConfig&&);
    CircuitConfig(const CircuitConfig& other) = delete;
    ~CircuitConfig();

    /** Open a SONATA circuit config from a path to JSON */
    static CircuitConfig fromFile(const std::string& path);

    /** Return the target simulator */
    std::string getTargetSimulator() const;

    /** Return the directory of a component in the components_dir given its name
     *
     * @param name component name
     * @throw std::runtime_error if component not found
     */
    std::string getComponentPath(const std::string& name) const;

    /** Return the list of network nodes */
    const std::vector<SubnetworkFiles>& getNodes() const;

    /** Return the list of network edges */
    const std::vector<SubnetworkFiles>& getEdges() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};


/** Read access to a SONATA simulation config file.
 */
class SONATA_API SimulationConfig
{
  public:
    /** Open a SONATA simulation config JSON */
    SimulationConfig(const std::string& contents);

    SimulationConfig(SimulationConfig&&);
    SimulationConfig& operator=(SimulationConfig&&);

    ~SimulationConfig();

    /** Open a SONATA simulation config from a path to JSON */
    static SimulationConfig fromFile(const std::string& path);

    /** @return the path to the circuit configuration JSON. */
    std::string getNetworkConfig() const;

    /** @return the path to the node set file. */
    std::string getNodeSetFilepath() const;

    /** @return the path to spikes .h5 file. */
    std::string getSpikesFilepath() const;

    /** @return the names of the compartment reports with membrame_report as
        the module name. */
    std::vector<std::string> getCompartmentReportNames() const;

    /** @return the file path to a copartment report by name.
     *  @throw std::runtime_error is the report doesn't exist *
     */
    std::string getCompartmentReportFilepath(const std::string& name) const;

  private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

}  // namespace sonata
}  // namespace bbp
