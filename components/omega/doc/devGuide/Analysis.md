(omega-dev-analysis)=

# Analysis

The Analysis module provides in-situ computation of analysis fields from
the ocean model state during simulation runtime. Analysis fields are
computed on-the-fly and written to output streams at user-specified
intervals. User-level configuration is described in the
[User Guide](#omega-user-analysis). This document describes the
implementation architecture, classes, and developer interfaces.

## Architecture Overview

The Analysis system is built on a composable operator architecture where
each operator performs a single, well-defined transformation. Operators
can be chained together to produce complex analysis outputs. The system
represents computations as a directed acyclic graph (DAG) where nodes are
operators and edges represent data dependencies.

### Key Components

- **AnalysisOperator**: Abstract base class for all operators. Each operator
  transforms one or more input fields into output fields.
- **AnalysisOpFactory**: Factory class for creating operator instances with
  runtime type dispatch based on field metadata.
- **OperatorNode**: Internal representation of a node in the dependency graph,
  containing an operator instance and its dependency/alarm metadata.
- **AnalysisGroup**: Abstract base class for bundled analysis configurations
  (e.g., GlobalStats).
- **Analysis**: Top-level orchestrator managing the operator graph, dependency
  resolution, and alarm-based computation scheduling.

## Header Files

To use Analysis functionality, include the appropriate header:
```c++
#include "analysis/Analysis.h"           // Top-level orchestrator
#include "analysis/AnalysisOperator.h"   // Base operator class
#include "analysis/AnalysisOpFactory.h"  // Operator factory
#include "analysis/AnalysisGroup.h"      // Analysis group base class
```

## Initialization and Usage

### Module Initialization

The Analysis module must be initialized after HorzMesh, VertCoord, and
TimeStepper have been initialized:

```c++
// Initialize Analysis module (registers operators, creates default instance)
Analysis::init();
```

This static method:
- Registers all base analysis operators with the factory
- Retrieves pointers to default mesh, vertical coordinate, and clock
- Creates the default Analysis instance from configuration

### Runtime Computation

During the time-stepping loop, call `computeAll()` on each timestep to
trigger operators whose alarms are ringing:

```c++
Analysis *DefAnalysis = Analysis::getDefault();
DefAnalysis->computeAll();
```

The `computeAll()` method:
- Iterates through all registered operators
- For each operator with a ringing alarm, triggers recursive computation
- Uses timestamp-based caching to avoid redundant computation
- Upstream dependencies are computed on-demand before downstream consumers

### Finalization

At the end of the simulation:
```c++
Analysis::finalize();
```

## Operator Architecture

### AnalysisOperator Base Class

All analysis operators derive from `AnalysisOperator`, which provides the
common interface and manages field dependencies, caching, and computation
scheduling.

```c++
class AnalysisOperator {
 public:
   /// Return the operator type name
   const std::string getOperatorType();

   /// Return unique instance name for this operator
   const std::string getName();

   /// Return names of input fields required by this operator
   const std::vector<std::string> getInputFieldNames();

   /// Return names of output fields produced by this operator
   const std::vector<std::string> getOutputFieldNames();

   /// Check if output field is already computed for given timestamp
   bool isCacheValid(const TimeInstant &TimeStamp);

   /// Initialize: store mesh/env pointers needed by compute()
   virtual void initialize(const MachEnv *InEnv,
                           const HorzMesh *Mesh,
                           const VertCoord *VCoord,
                           Config Options);

   /// Set period alarm for temporal reduction operators
   virtual void setPeriodAlarm(Alarm *Alarm);

   /// Perform computation - pure virtual, must be implemented by derived class
   virtual void compute(const TimeInstant &TimeStamp) = 0;

 protected:
   std::string OperatorTypeName;
   std::string InstanceName;
   std::vector<std::string> InputNames;
   std::vector<std::string> OutputNames;
   TimeInstant LastComputed;
   bool FieldComputed;
};
```

### Implementing a New Operator

To create a new analysis operator:

1. Define a templated class derived from `AnalysisOperator`:

```c++
template<typename ArrayT>
class MyNewOp : public AnalysisOperator {
 public:
   using ScalarT = typename ArrayT::non_const_value_type;

   // Constructor: set input/output names, create Field and data array
   MyNewOp(const std::vector<std::string> &UpstreamNames, Config Options);

   // Implement computation
   void compute(const TimeInstant &TimeStamp) override;

 private:
   const HorzMesh *Mesh;
   MPI_Comm Comm;
   typename Array1D<ScalarT>::type OutputData;
};
```

2. Implement the constructor to create output Fields and register them:

```c++
template<typename ArrayT>
MyNewOp<ArrayT>::MyNewOp(const std::vector<std::string> &UpstreamNames,
                         Config Options)
    : AnalysisOperator() {

   OperatorTypeName = "MyNewOp";
   InputNames = UpstreamNames;
   InstanceName = InputNames[0] + "_MyNewOp";
   OutputNames.push_back(InstanceName);

   // Allocate output array
   OutputData = Array1D<ScalarT>::type("MyNewOpOutput", 1);

   // Create and register output Field
   Metadata Metadata;
   Field::create(InstanceName, "Description of output", "units",
                 "MyNewOpGroup", 0, OutputData, Metadata);
}
```

3. Implement the `compute()` method:

```c++
template<typename ArrayT>
void MyNewOp<ArrayT>::compute(const TimeInstant &TimeStamp) {

   // Retrieve input field from registry
   Field *InputField = Field::get(InputNames[0]);
   ArrayT InputArray;
   InputField->getData(InputArray);

   // Perform computation
   ScalarT Result = /* ... computation using InputArray ... */;

   // Write to output array
   auto OutputHost = Kokkos::create_mirror_view(OutputData);
   OutputHost(0) = Result;
   Kokkos::deep_copy(OutputData, OutputHost);
}
```

4. Register the operator with the factory in `Analysis::registerAllBaseAnalysisOperators()`:

```c++
AnalysisOpFactory::registerAllArrayVariants<MyNewOp>("MyNewOp");
```

### Operator Registration and Factory

The factory pattern enables runtime type dispatch without hard-coded switch
statements. The `AnalysisOpFactory` maintains a registry mapping operator
type names to constructor functions.

Registration of all array type variants for a templated operator:
```c++
AnalysisOpFactory::registerAllArrayVariants<SpatialMaxOp>("SpatialMax");
```

This expands to register all combinations of scalar type (I4/I8/R4/R8),
rank (1-5), and memory location (Device/Host/Both).

Creating an operator instance at runtime:
```c++
std::unique_ptr<AnalysisOperator> Op = AnalysisOpFactory::createOp(
    "SpatialMax",           // operator type name
    {"Temperature"},        // upstream field names
    Options                 // Config object with parameters
);
```

The factory inspects the upstream Field metadata and selects the matching
templated specialization automatically.

## Operator Composition and Dependencies

### Operator Chain Syntax

Operator chains are specified as underscore-delimited strings where each
component represents an operator:

```
FieldName_Op1_Op2_Op3
```

Examples:
- `Temperature_SpatialMax` - spatial maximum of Temperature
- `NormalVelocity_SpatialMean_TimeMean1day` - 1-day time average of the
  spatial mean of NormalVelocity

The `Analysis::parseChainAndBuildOps()` method splits on underscores and
creates operators for each component that doesn't already exist as a Field.

### Dependency Resolution

The `Analysis` class maintains a vector of `OperatorNode` structures:

```c++
struct OperatorNode {
   std::unique_ptr<AnalysisOperator> Op;  // Operator instance (owned)
   std::vector<OperatorNode*> Upstreams;  // Upstream dependencies (non-owning)
   std::vector<std::string> StreamNames;  // Output stream names
   std::vector<Alarm*> ComputeAlarms;     // Compute alarms (non-owning)
};
```

Dependencies are resolved by matching operator input field names against
other operators' output field names. The `buildOperatorDependencies()`
method populates the `Upstreams` vectors after all operators have been
registered.

### Shared Intermediates

When multiple operator chains require the same intermediate result, the
Analysis system automatically shares the computation. The `parseChainAndBuildOps()`
method checks if a Field already exists before creating a new operator,
enabling natural reuse of intermediate results.

For example, both `Temperature_SpatialMean` and `Temperature_SpatialStdDev`
require the spatial mean as an intermediate. The system creates only one
`SpatialMean` operator that is shared between the two chains.

## Alarm-Based Scheduling

### Alarm Model

Each `OperatorNode` contains a vector of non-owning alarm pointers. An
operator computes when any of its alarms rings.

**Terminal operators** (those writing to output streams):
- Borrow a raw pointer to the stream's write alarm
- The IOStream owns the alarm

**Temporal reduction operators**:
- Require two alarms: accumulation and output
- Accumulation alarm: triggers sample accumulation (owned by Analysis)
- Output alarm: triggers finalization and write (borrowed from stream)

**Intermediate operators**:
- Receive alarm pointers propagated from downstream consumers
- Compute on-demand when any downstream alarm rings

### Alarm Propagation

The `Analysis::propagateAlarmsUpstream()` method iteratively propagates
alarms from terminal nodes to all upstream dependencies:

```c++
void Analysis::propagateAlarmsUpstream() {
   bool Changed = true;
   while (Changed) {
      Changed = false;
      for (auto &Node : OpNodes) {
         for (auto *Upstream : Node->Upstreams) {
            for (auto *Alarm : Node->ComputeAlarms) {
               if (!Upstream->hasAlarm(Alarm)) {
                  Upstream->ComputeAlarms.push_back(Alarm);
                  Changed = true;
               }
            }
         }
      }
   }
}
```

### Recursive Computation with Caching

The `computeAll()` method uses recursive computation with timestamp-based
caching:

```c++
void Analysis::computeAll() {
   TimeInstant CurrentTime = ModelClock->getCurrentTime();

   for (auto &Node : OpNodes) {
      if (anyAlarmRinging(Node->ComputeAlarms)) {
         computeRecursive(Node.get(), CurrentTime);
      }
   }
}

void Analysis::computeRecursive(OperatorNode *Node, TimeInstant Time) {
   // Check cache
   if (Node->Op->isCacheValid(Time)) {
      return;  // Already computed this timestep
   }

   // Compute upstream dependencies first
   for (auto *Upstream : Node->Upstreams) {
      computeRecursive(Upstream, Time);
   }

   // Compute this operator
   Node->Op->compute(Time);
}
```

## AnalysisGroup Interface

`AnalysisGroup` is the abstract base class for bundled analysis configurations.
Derived classes (e.g., `GlobalStats`) parse their group-specific configuration
and construct the appropriate operator chains.

```c++
class AnalysisGroup {
 public:
   virtual ~AnalysisGroup() = default;

   std::string getName();

   /// Create IOStream objects for this group's output
   void createAnalysisGroupStreams(const std::string &GroupName,
                                   Config &AnalysisGroupOptions,
                                   Analysis *AnalysisMgr);

 protected:
   struct OpChainInfo {
      std::string ChainStr;      // Operator chain string
      std::string FreqStr;       // Output frequency
      bool IsTimeReduction;      // Temporal reduction vs discrete sample
   };

   struct StreamParams {
      std::map<std::string, std::string> Params;
      Config toConfig() const;
   };

   std::string GroupName;
   std::vector<OpChainInfo> OpChainInfos;
};
```

## Built-in Operators

### Spatial Reduction Operators

- **SpatialMaxOp**: Computes global maximum using `globalMaxVal()`
- **SpatialMinOp**: Computes global minimum using `globalMinVal()`
- **SpatialMeanOp**: Computes global mean using `globalSum()`
- **SpatialStdDevOp**: Computes global standard deviation (requires
  SpatialMean as intermediate)

All spatial operators leverage the global reduction functions described in
[Reductions](#omega-dev-reductions).

### Temporal Operators

- **TimeMeanOp**: Accumulates samples over a time period and computes
  the mean at period end. Requires two alarms: accumulation and output.

## Integration with IOStreams

Analysis fields are integrated into the IOStream framework. The
`AnalysisGroup::createAnalysisGroupStreams()` method:

1. Groups operator chains by output frequency and type
2. Creates IOStream objects for each group
3. Associates operator output fields with streams
4. Validates temporal reduction periods against restart interval

Output fields appear in NetCDF files with their full operator chain name
(e.g., `Temperature_Mean_TimeMean1day`) and associated metadata.

## Helper Functions

Creating Config objects inline for operator parameters:

```c++
// Helper to create key-value pairs
auto Param1 = opParam("Period", "1day");
auto Param2 = opParam("Layer", 10);

// Build Config from params
Config OpConfig = makeOpConfig(Param1, Param2);
```

Parsing frequency strings:
```c++
// Parse "1day" into ["1", "days"]
std::vector<std::string> Parts = parseFreqStr("1day");
```

## Implementation Notes

The current v1 implementation uses a simplified dependency resolution approach:
- Operator chains are parsed left-to-right
- Nodes are appended in natural dependency order
- Dependencies are resolved post-hoc by name matching
- Full signature-based deduplication and formal topological sort are planned
  for future versions

For complete architectural details, mathematical formulations, and testing
specifications, see the [design document](../design/Analysis.md).
