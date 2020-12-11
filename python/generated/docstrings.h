/*
  This file contains docstrings for use in the Python bindings.
  Do not edit! They were automatically extracted by pybind11_mkdoc.
 */

#define __EXPAND(x)                                      x
#define __COUNT(_1, _2, _3, _4, _5, _6, _7, COUNT, ...)  COUNT
#define __VA_SIZE(...)                                   __EXPAND(__COUNT(__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1))
#define __CAT1(a, b)                                     a ## b
#define __CAT2(a, b)                                     __CAT1(a, b)
#define __DOC1(n1)                                       __doc_##n1
#define __DOC2(n1, n2)                                   __doc_##n1##_##n2
#define __DOC3(n1, n2, n3)                               __doc_##n1##_##n2##_##n3
#define __DOC4(n1, n2, n3, n4)                           __doc_##n1##_##n2##_##n3##_##n4
#define __DOC5(n1, n2, n3, n4, n5)                       __doc_##n1##_##n2##_##n3##_##n4##_##n5
#define __DOC6(n1, n2, n3, n4, n5, n6)                   __doc_##n1##_##n2##_##n3##_##n4##_##n5##_##n6
#define __DOC7(n1, n2, n3, n4, n5, n6, n7)               __doc_##n1##_##n2##_##n3##_##n4##_##n5##_##n6##_##n7
#define DOC(...)                                         __EXPAND(__EXPAND(__CAT2(__DOC, __VA_SIZE(__VA_ARGS__)))(__VA_ARGS__))

#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif


static const char *__doc_bbp_sonata_DataFrame = R"doc()doc";

static const char *__doc_bbp_sonata_DataFrame_data = R"doc()doc";

static const char *__doc_bbp_sonata_DataFrame_ids = R"doc()doc";

static const char *__doc_bbp_sonata_DataFrame_times = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader_Population = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader_Population_Population = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader_Population_data_units = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader_Population_get =
R"doc(Parameter ``node_ids``:
    limit the report to the given selection.

Parameter ``tstart``:
    return voltages occurring on or after tstart.
    tstart=nonstd::nullopt indicates no limit.

Parameter ``tstop``:
    return voltages occurring on or before tstop.
    tstop=nonstd::nullopt indicates no limit.

Parameter ``tstride``:
    indicates every how many timesteps we read data.
    tstride=nonstd::nullopt indicates that all timesteps are read.)doc";

static const char *__doc_bbp_sonata_ReportReader_Population_getDataUnits = R"doc(Return the unit of data.)doc";

static const char *__doc_bbp_sonata_ReportReader_Population_getIndex = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader_Population_getNodeIds = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader_Population_getSorted = R"doc(Return true if the data is sorted.)doc";

static const char *__doc_bbp_sonata_ReportReader_Population_getTimeUnits = R"doc(Return the unit of time)doc";

static const char *__doc_bbp_sonata_ReportReader_Population_getTimes = R"doc(Return (tstart, tstop, tstep) of the population)doc";

static const char *__doc_bbp_sonata_ReportReader_Population_nodes_ids = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader_Population_nodes_ids_sorted = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader_Population_nodes_pointers = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader_Population_pop_group = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader_Population_time_units = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader_Population_tstart = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader_Population_tstep = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader_Population_tstop = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader_ReportReader = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader_file = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader_getPopulationNames = R"doc(Return a list of all population names.)doc";

static const char *__doc_bbp_sonata_ReportReader_openPopulation = R"doc()doc";

static const char *__doc_bbp_sonata_ReportReader_populations = R"doc()doc";

static const char *__doc_bbp_sonata_SpikeReader =
R"doc(const SpikeReader file(filename); auto pops =
file.getPopulationNames(); for (const auto& data:
file[pops.openPopulation(0).get(Selection{12UL, 34UL, 58UL})) { NodeID
node_id; double timestamp; std::tie(node_id, timestamp) = data;
std::cout << "[" << timestamp << "] " << node_id << std::endl; })doc";

static const char *__doc_bbp_sonata_SpikeReader_Population = R"doc()doc";

static const char *__doc_bbp_sonata_SpikeReader_Population_Population = R"doc()doc";

static const char *__doc_bbp_sonata_SpikeReader_Population_Sorting = R"doc()doc";

static const char *__doc_bbp_sonata_SpikeReader_Population_Sorting_by_id = R"doc()doc";

static const char *__doc_bbp_sonata_SpikeReader_Population_Sorting_by_time = R"doc()doc";

static const char *__doc_bbp_sonata_SpikeReader_Population_Sorting_none = R"doc()doc";

static const char *__doc_bbp_sonata_SpikeReader_Population_filterNode = R"doc()doc";

static const char *__doc_bbp_sonata_SpikeReader_Population_filterTimestamp = R"doc()doc";

static const char *__doc_bbp_sonata_SpikeReader_Population_get = R"doc(Return reports for this population.)doc";

static const char *__doc_bbp_sonata_SpikeReader_Population_getSorting = R"doc(Return the way data are sorted ('none', 'by_id', 'by_time'))doc";

static const char *__doc_bbp_sonata_SpikeReader_Population_sorting = R"doc()doc";

static const char *__doc_bbp_sonata_SpikeReader_Population_spikes = R"doc()doc";

static const char *__doc_bbp_sonata_SpikeReader_Population_tstart = R"doc()doc";

static const char *__doc_bbp_sonata_SpikeReader_Population_tstop = R"doc()doc";

static const char *__doc_bbp_sonata_SpikeReader_SpikeReader = R"doc()doc";

static const char *__doc_bbp_sonata_SpikeReader_filename = R"doc()doc";

static const char *__doc_bbp_sonata_SpikeReader_getPopulationNames = R"doc(Return a list of all population names.)doc";

static const char *__doc_bbp_sonata_SpikeReader_openPopulation = R"doc()doc";

static const char *__doc_bbp_sonata_SpikeReader_populations = R"doc()doc";

#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif

