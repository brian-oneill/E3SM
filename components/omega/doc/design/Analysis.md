<!--- OMEGA Analysis Module requirements and design ------------------------->

(omega-design-analysis)=

# Analysis

## 1 Overview

The Omega Analysis module provides in-situ computation of desired analysis fields from the ocean model state. Analysis fields are computed on-the-fly during simulation runtime and written to output streams at user-specified intervals, providing an alternative to extensive offline post-processing. The framework is built on a composable operator architecture where operators can be chained together to produce analysis outputs. This approach enables user flexibility, avoids the proliferation of hard-coded analysis groups, and supports future extensibility without architecture changes.

## 2 Requirements

### 2.1 Requirement: Composable operator framework

The Analysis system depends on simple, composable operators where each operator performs a single, well-defined transformation. This enables:
- New analysis outputs via configuration rather than new code
- Testing of individual operations in isolation
- Reuse of common operations (spatial and temporal averaging, reductions, binary operations) across analysis computations

### 2.2 Requirement: Availablility of all model variables

All simulation variables produced by the model and available for I/O in Omega must be available to the Analysis module. Variables produced by the Analysis system should also be available for further Analysis computation.

### 2.3 Requirement: Field access via dependency declaration

Operators must declare their input field dependencies at construction time. During initialization, the orchestrator resolves dependencies and provides operators with persistent pointers/references to input fields (from simulation model fields or upstream operators). Operators retain these references and access fields directly during compute().

### 2.4 Requirement: Operator registration and factory

New operators must be registerable via a factory pattern, without changes to the core analysis architecture. Operators self-register during initialization, and the parser queries the factory for operator by name. This facilitates future extensibility; new operators integrate into the analysis framework without modifying orchestration code.

### 2.5 Requirement: Multi-output operators

Operators must be able to produce multiple output fields. This allows, for example, operators to simultaneously return separable components of a vector field, or the components of a spatial gradient.

### 2.6 Requirement: Computation caching

When multiple output streams or analysis fields depend on the same intermediate result, that result must be computed once per timestep and cached. Timestamp-based cache validation prevents stale results.

### 2.7 Requirement: Time operators

Time-based operations (mean, min, max over a period) must be regular operators within the analysis framework, enabling composition with spatial operations. Time period specification should be flexible (not limited to hard-coded groups).

### 2.8 Requirement: Stream integration

Analysis fields must be integrated into the Omega output stream framework. Configurable output stream parameters (filename, precision, period, etc.) must be provided for fields produced by the analysis system. Fields will be written to output with associated metadata.

### 2.9 Requirement: Polaris compatibility

Output from the Analysis module must be compatible with Polaris for post-processing.

### 2.10 Requirement: Requested initial analysis capability

Initial delivery of the Analysis system will supply operators necessary for computing a specified set of Analysis groups:
- Global stats: global reduction to mean, min, max, and standard deviation of configurable
- High frequency: layer extraction of configurable variables
- AMOC: stream function for Atlantic meridional overturning circulation
- Eddy stats

## 3 Algorithmic Formulation

### 3.1 Operator Composition and Dependency Resolution

The Analysis system represents computations as a directed acyclic graph (DAG) where nodes are operators and edges represent data dependencies. A single Analysis field computation is defined by a string name that may expand into multiple operators forming a chain.

#### 3.1.1 Operator dependencies

Each operator $\mathcal{O}_i$ produces one or more output fields and requires zero or more input fields:

$$ \{\mathcal{O}_i^{\text{out},1}, \mathcal{O}_i^{\text{out},2}, \ldots\} = f_i(\mathcal{I}_{i,1}, \mathcal{I}_{i,2}, \ldots, \mathcal{I}_{i,k}) $$

where each input $\mathcal{I}_{i,j}$ is either:
- A simulation field from the model (terminal node, no incoming operator edge)
- An output of another operator $\mathcal{O}_j$ (creating dependency edge $\mathcal{O}_j \to \mathcal{O}_i$)

**Operator chains:** A single Analysis field name $a$ may parse into an ordered sequence of operators:

$$ a \xmapsto{\text{parse}} \{\mathcal{O}_1, \mathcal{O}_2, \ldots, \mathcal{O}_m\} $$

where intermediate operators produce fields consumed by subsequent operators in the chain, and only the final operator $\mathcal{O}_m$ writes to the output stream.

**Shared intermediates:** When multiple Analysis fields require the same intermediate result, the dependency resolver identifies structurally equivalent operators via signature matching:

$$ \text{sig}(\mathcal{O}) = (\text{type}(\mathcal{O}), \{\mathcal{I}_1, \mathcal{I}_2, \ldots\}) $$

Two operators with identical signatures are merged into a single node in the DAG, preventing redundant computation.

#### 3.1.2 Depenency graph

**Algorithm**: $\texttt{AnalysisOrchestrator::buildDependencyGraph}$

Input: Set of requested Analysis field $\mathcal{A} = {a_1, a_2, \ldots, a_n}$ from all output streams

Output: Directed acyclic graph $\mathcal{G} = (\mathcal{V}, \mathcal{E})$ where $\mathcal{V}$ are operator nodes, $\mathcal{E}$ are data dependency edges, with topological ordering $\pi : \mathcal{V} \to \mathbb{N}$

**Phase 1**: Parse and expand operator chains
1.  Initialize: $\mathcal{V} \leftarrow \emptyset$, $\mathcal{E} \leftarrow \emptyset$, $\Sigma \leftarrow \emptyset$ (signature cache)
2.  **For** each analysis field $a \in \mathcal{A}$:
    - Parse string into chain of operators: ${\mathcal{O}_1, \ldots, \mathcal{O}_m} \leftarrow \texttt{AnalysisOrchestrator::parseOperatorChain}(a)$
    - **For** $i = 1$ to $m$:
        - Compute signature: $s \leftarrow \text{sig}(\mathcal{O}_i)$
        - **If** $s \in \Sigma$ (operator already exists): 
            - Retrieve existing node: $v \leftarrow \Sigma[s]$
            - **If** $i = m$ (final operator): Add $a$ to $v$'s output list 
        - **Else** (create new node): 
            - Create node: $v \leftarrow \text{OperatorNode}(\mathcal{O}_i)$:
            - **If** $i = m$ (final operator): add output for node $v$ to stream for $a$, set alarm period 
            - **Else** (intermediate operator): no stream output, computed on-demand when downstream alarm rings
            - Add to graph: $\mathcal{V} \leftarrow \mathcal{V} \cup \{v\}$
            - Cache signature: $\Sigma \leftarrow \Sigma \cup \{(s, v)\}$

**Phase 2**: Resolve dependencies
1. **For** each operator node $v \in \mathcal{V}$:
    - Let $\mathcal{I}(v) = \{\mathcal{I}_1, \ldots, \mathcal{I}_n\}$ be input fields for $v$
    - **For** each required input $\mathcal{I}_j \in \mathcal{I}(v)$:

        - **If** $\mathcal{I}_j$ is a simulation field from the model: - Terminal dependency, no edge needed

        - **Else if** $\exists~ u \in \mathcal{V}$ such that $\mathcal{I}_j \in \text{outputs}(u)$: 
            - Add dependency edge: $\mathcal{E} \leftarrow \mathcal{E} \cup {(u, v)}$ 
            - Propagate alarm: If $v.\text{alarm} \neq \texttt{null}$: $u.\text{alarm} \leftarrow \max{\text{freq}}(u.\text{alarm}, v.\text{alarm})$ 
        (producer must compute at least as often as consumer)

        - **Else** (field not found) ERROR: Required field $\mathcal{I}_j$ not found

**Phase 3**: Validate acyclicity
1. Detect cycles using depth-first search with recursion stack
    - Apply cycle detection algorithm:
$$ \text{hasCycle}(\mathcal{G}) = \begin{cases} \texttt{true} & \text{if } \exists \text{ path } v_1 \to v_2 \to \cdots \to v_n \to v_1 \ \texttt{false} & \text{otherwise} \end{cases} $$
    - **If** cycle detected:
        - ERROR: Circular dependency detected
    - **Else** (no cycle)
        Graph $\mathcal{G} = (\mathcal{V}, \mathcal{E})$ is valid DAG

**Phase 4**: Topological sort
1. Compute topological ordering $\pi : \mathcal{V} \to {0, 1, \ldots, |\mathcal{V}|-1}$ using Kahn's algorithm:
    - Initialize in-degree for all nodes
        - $\text{inDegree}(v) \leftarrow |{u \in \mathcal{V} : (u,v) \in \mathcal{E}}|$ for all $v \in \mathcal{V}$
    - Initialize ready queue with all source nodes
        - $Q \leftarrow {v \in \mathcal{V} : \text{inDegree}(v) = 0}$ (ready queue)
    - Initialize output structures:
        - $\text{sorted} \leftarrow []$, $\text{order} \leftarrow 0$

    - **While** $Q \neq \emptyset$:
        - Remove $v$ from $Q$
        - Assign topological position 
            - $\pi(v) \leftarrow \text{order}$
            - $\text{order} \leftarrow \text{order} + 1$
            - $\text{sorted}.\texttt{append}(v)$
        - For each outgoing edge $(v, w) \in \mathcal{E}$:
            $\text{inDegree}(w) \leftarrow \text{inDegree}(w) - 1$
            If $\text{inDegree}(w) = 0$: $Q \leftarrow Q \cup {w}$

    - **If** $|\text{sorted}| \neq |\mathcal{V}|$:
    ERROR: Cycle exists (should not reach here if Phase 3 succeeded)

2. Return: $\mathcal{G} = (\mathcal{V}, \mathcal{E})$ with ordering $\pi$

### 3.2 Operator Factory and Registration

The operator factory provides a runtime registry that maps operator names, (e.g. global_min, time_mean), to constructor functions. This enables:
- **Decentralized registration**: Operator register themselves via static initialization
- **Dynamic instantiation**: The parser creates operators by name without hard-coded switch statements
- **Extensibility**: New operators can be added without modifying orchestrator code.

#### 3.2.1 Registration

**Algorithm**: $\texttt{REGISTER\_ANALYSIS\_OPERATOR(Type, Name)}$

Operators self-register via macro before $\texttt{main()}$ executes.

1. Create static lambda that executes at program startup
2. Lambda calls $\texttt{AnalysisOperatorFactory::registerOperator(Name, constructorFunc)}$
3. Registry stores: $\texttt{map[Name]} \rightarrow \texttt{constructorFunc}$
4. Validate $\texttt{Name}$ is unique (error if duplicate)

#### 3.2.2 Factory operator creation

**Algorithm**: $\texttt{AnalysisOperatorFactory::createOp}$

**Input**: Operator type name, instance name, configuration file
**Output**: Unique pointer to operator instance

1. Lookup constructor in registry
2. Invoke constructor: $\texttt{Op} \leftarrow \texttt{constructorFunc(InstanceName, Config)}$
3. Validate: $\texttt{Op.operatorType() == TypeName}$
4. Return $\texttt{Op}$

### 3.3 Runtime Dispatch

The main Analysis computational loop executed every time step. Topological sort ensures we process nodes in dependency order (outer loop). Recursive pulling ensures upstream dependencies are fresh when needed (inner loop). Caching prevents redundant work.


**Algorithm**: $\texttt{AnalysisOrchestrator::computeAll}$

**Input**: Topologically sorted operator list, current timestamp
**Output**: Updated Analysis fields in cache

1. **For** each $\texttt{Op} \in \texttt{TopologicalOrder}$:
    - **If** $\texttt{Op.Alarm.isRinging}$:
        - $\texttt{computeRecursive(Op, TimeStamp)}$
        - $\texttt{Op.Alarm.reset()}$

2. $\texttt{ComputeRecursive}()$
    - **If** a fresh field has already been computed,
    $\texttt{Op.FieldComputed AND Op.LastComputed == TimeStamp}$:
        - $\texttt{Return}$
    - **For** each upstream operator this operator depends on,
    $\texttt{UpstreamOp} \in \texttt{Op.Dependencies}$:
        - Recursively compute up the chain,
        $\texttt{ComputeRecursive(UpstreamOp, TimeStamp)}$
    - Compute the operator: $\texttt{Op.compute(TimeStamp)}$
    - Set compute flag and timestamp: 
        - $\texttt{Op.LastComputed} \leftarrow \texttt{TimeStamp}$
        - $\texttt{Op.FieldComputed} \leftarrow \texttt{True}$

## 4 Design

### 4.1 Data types and parameters

#### 4.1.1 Configuration

#### 4.1.2 Classes

##### AnalysisOperator

The **AnalysisOperator** class is a base class from which specific operators are derived. Data arrays for the output fields are allocated as members of the derived class.

```c++
class AnalysisOperator {
 public:
   virtual ~AnalysisOperator() = default;

   /// Return name for this operator type
   const std::string getOperatorType();

   /// Return unique name for this instance of the operator type, contains
   /// concatenated strings of upstream operator Names
   const std::string getName() const = 0;

   /// Return names of fields required by this operator
   const std::vector<std::string> getInputFieldNames();

   /// Return names of output fields produced by this operator
   const std::vector<std::string> getOutputFieldNames();

   /// Initialize operator: create and register output fields in Field map
   virtual void initialize(const Config *Options,
                           const HorzMesh *Mesh,
                           const VertCoord *VCoord) = 0;

   /// Perform computation of Analysis fields. Data arrays of input field
   /// retrieved from Field map using input field names. Writes to
   /// operator-owned output arrays
   virtual void compute(const TimeInstant& ts) = 0;

 protected:
   std::string OperatorTypeName;
   std::string InstanceName;
   std::vector<std::string> InputNames;
   std::vector<std::string> OutputNames;

 
   TimeInstant LastComputed;
   bool FieldComputed;
};
```

##### Example derived operator
**GlobalMaxOp**
```c++
class GlobalMaxOp : public AnalysisOperator {
 public:
   /// Constructor
   /// \param[in] Name    Unique name for this operator instance
   /// \param[in] Options Configuration for this operator
   GlobalMaxOp(const std::string &Name, const Config &Options);

   /// Destructor
   ~GlobalMaxOp() override = default;

   /// Initialize operator: validate input, create output field with matching type
   ///
   /// \param[in] Options Configuration options
   /// \param[in] Mesh    Horizontal mesh
   /// \param[in] VCoord  Vertical coordinate
   void initialize(const Config *Options,
                   const HorzMesh *Mesh,
                   const VertCoord *VCoord) override;

   /// Compute global maximum
   ///
   /// Retrieves input field with string of input name and uses globalMaxVal
   /// to compute the maximum across all MPI ranks. Stores result in output 
   /// array with the same data type as the input.
   /// \param[in] ts Current simulation time
   void compute(const TimeInstant &ts) override;

 private:

   // Member data
   const HorzMesh *Mesh;                    ///< Horizontal mesh
   std::string OutputFieldName;             ///< Name of output field
   
   /// Output data storage - holds exactly one array type matching input
   std::variant<Array1DR4, Array1DR8, Array1DI4, Array1DI8> OutputData;
};
```

##### AnalysisOperatorFactory
Factory for creating AnalysisOperator instances by name. Operators register themselves via $\texttt{REGISTER\_ANALYSIS\_OPERATOR}$ macro. The parser uses the factory to instantiate operator chains from configuration.
```c++
class AnalysisOperatorFactory{
 public:
   using CreatorFunc = std::function<std::unique_ptr<AnalysisOperator>(
       const std::string &Name, const Config &Options)>;

   /// Register an operator type
   static void registerOperator(const std::string &Type, CreatorFunc Creator);

   /// Create an operator instance
   static std::unique_ptr<AnalysisOperator> create(const std::string &Type,
                                                   const std::string &Name,
                                                   const Config &Options);

   /// Query available operators (for validation, error messages)
   static std::vector<std::string> availableOperators();

   /// Check if operator type is registered
   static bool hasOperator(const std::string &Type);

 private:
   // Static map of registered operators
   static std::map<std::string, CreatorFunc> Registry;
};

// Convenience macro for operator registration
// Usage: REGISTER_ANALYSIS_OPERATOR(GlobalMinOp, "global_min");
#define REGISTER_ANALYSIS_OPERATOR(Type, Name) \
  namespace Omega { \
  static bool registered_##Type = []() { \
    AnalysisOperatorFactory::registerOperator(Name, \
      [](const std::string& n, const Config& c) { \
        return std::make_unique<Type>(n, c); \
      }); \
    return true; \
  }(); \
  }
```


##### AnalysisOrchestrator

The **AnalysisOrchestrator** manages analysis computation across all output streams.
Responsibilities:
- Parse configuration into operator instances
- Initialize all operators
- Build dependency graph
- Dipatch compute calls during runtime

```c++
class AnalysisOrchestrator{
 public:
   /// Constructor
   AnalysisOrchestrator(const Config &Options,
                        Clock *ModelClock,
                        const HorzMesh *Mesh,
                        const VertCoord *VCoord);

   /// Called each timestep: check alarms, compute triggered diagnostics
   /// For each ringing alarm, compute operatores in dependency order
   /// (upstream first). Operators use caching to avoid redundant work
   void computeAll();

   /// Finalize and release resources
   void finalize();

 private:
   /// Internal representation of an operator node in the dependency graph
   struct OperatorNode {
      std::unique_ptr<AnalysisOperator> Op;       ///< The operator instance
      std::vector<OperatorNode*> Upstream;        ///< Operators this depends on
      std::string StreamName;                     ///< Output stream name
      Alarm ComputeAlarm;                         ///< When to compute this operator
      
      OperatorNode() = default;
      
      // Move-only (contains unique_ptr)
      OperatorNode(OperatorNode&&) = default;
      OperatorNode& operator=(OperatorNode&&) = default;
      OperatorNode(const OperatorNode&) = delete;
      OperatorNode& operator=(const OperatorNode&) = delete;
   };

   /// Parse a configuration into an operator instance
   std::unique_ptr<AnalysisOperator> parseOperatorChain(
       const std::string &AnalysisOperatorChain,
       const Config &Options);

   /// Initialize all operators for all Analysis streams
   void initializeOperators();

   /// Build dependency graph for all Analysis fields
   /// Must be called AFTER initializeOperators() so all output Fields exist
   void buildDependencyGraph();

   /// Topological sort for evaluation order
   std::vector<OperatorNode*> topologicalSort();

   /// Find which operator produces a given field name
   OperatorNode* findOperatorProducing(const std::string& fieldName);

   /// Create alarms for each stream based on their period
   void createAlarms();

   // Member data
   std::vector<OperatorNode> Operators;        ///< All operators for all streams
   std::vector<OperatorNode*> SortedOperators; ///< Topological sort of Operators
   Clock *ModelClock;                          ///< Simulation clock
   const HorzMesh *Mesh;                       ///< Horizontal mesh
   const VertCoord *VCoord;                    ///< Vertical coordinate
   Config Options;                             ///< Full configuration

};
```
### 4.2 Methods

## 5 Verification and Testing

### 5.1 Test: Individual operator correctness

For each operator type, construct a small test mesh with analytic field values. Call compute() and verify output against known-answer solution.

### 5.2 Test: Time operators accumulation
Verify that time-averaging operators correctly accumulate over specified periods

### 5.3 Test: Operator factory and registration
Verify that all operators register correctly and can be instantiated via the factory.

### 5.4 Test: Dependency resolution and cycle detection
Create configurations with various dependency patterns and verify correct handling.

### 5.5 Test: Parser handling
Verify that the parser properly handles valid operator chain strings and produces error messages for invalid strings

### 5.6 Integration test: End-to-end global stats
Test: Complete system test exercising all components from configuration parsing through NetCDF output for global stats.
