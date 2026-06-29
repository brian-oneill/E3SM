(omega-user-analysis)=

# Analysis

The Analysis module provides in-situ computation of analysis fields from
the ocean model state during simulation runtime. Analysis fields are
computed on-the-fly and written to output streams at user-specified
intervals, providing an alternative to extensive offline post-processing.

## Overview

In-situ analysis allows you to compute diagnostics and statistics during
the simulation rather than saving large amounts of data for post-processing.
This approach:
- Reduces storage requirements by computing and saving only derived quantities
- Eliminates the need for separate post-processing steps for common diagnostics
- Provides immediate access to analysis results
- Enables real-time monitoring of simulation behavior

The Analysis system uses a composable operator architecture where simple
operations (spatial reductions, temporal averaging) can be chained together
to produce complex analysis outputs.

## Configuration

Analysis groups are configured in the `omega.yml` configuration file under
the `Omega:Analysis:` section. Each analysis group can be enabled or
disabled independently.

### GlobalStats Group

The `GlobalStats` analysis group computes global spatial statistics for
specified fields. It supports spatial reductions (mean, min, max, standard
deviation) optionally combined with temporal averaging or discrete sampling.

#### Example Configuration

```yaml
Omega:
  Analysis:
    GlobalStats:
      Enable: true
      Fields:
        - NormalVelocity
        - Temperature
        - Salinity
        - LayerThickness
      SpatialStats:
        - Mean
        - Max
        - Min
        - StdDev
      ReductionPeriod:
        - 1day
        - 1month
      SampleFreq:
        - 6hour
      Filename: ocean.global.stats.$Y
      Stream:
        FileFreq: 1
        FileFreqUnits: years
        Precision: single
```

#### Configuration Parameters

- **Enable:** Required boolean (true/false). Enables or disables this
  analysis group.

- **Fields:** Required list of field names. Specifies which model fields
  to analyze. Field names must match fields defined in Omega (e.g.,
  Temperature, Salinity, NormalVelocity, LayerThickness).

- **SpatialStats:** Required list of spatial reduction operators to apply.
  Available operators include:
  - `Mean` - Global spatial mean
  - `Max` - Global spatial maximum
  - `Min` - Global spatial minimum
  - `StdDev` - Global spatial standard deviation

- **ReductionPeriod:** Optional list of time periods for temporal reduction.
  When specified, the analysis system computes time-averaged values over
  the specified intervals. Examples: `1hour`, `6hour`, `1day`, `1month`,
  `1year`. Each period creates a separate set of output fields.

- **SampleFreq:** Optional list of time frequencies for discrete sampling.
  When specified, the analysis system writes instantaneous snapshots of
  the spatial statistics at the specified intervals. Examples: `1hour`,
  `6hour`, `1day`.

- **Filename:** Required string. Template for output filenames. Supports
  time-based template variables:
  - `$Y` - simulation year
  - `$M` - simulation month
  - `$D` - simulation day
  - `$h` - simulation hour
  - `$m` - simulation minute
  - `$s` - simulation second

- **Stream:** Optional map of IOStream parameters to customize output
  behavior. If not provided, default stream parameters are used. Supported
  options include:
  - `FileFreq` - Frequency for creating new files when multiple time slices
    are included (integer)
  - `FileFreqUnits` - Units for FileFreq (e.g., `years`, `months`, `days`)
  - `Precision` - Output precision (`single` or `double`)
  - `IfExists` - Behavior if file exists (`replace`, `append`, `fail`)

#### Output Fields

For each combination of `(Field, SpatialStat)`, the GlobalStats group
creates analysis fields with names following the pattern:
- Temporal reduction: `FieldName_SpatialStat_TimeMeanPeriod`
  (e.g., `Temperature_Mean_TimeMean1day`)
- Discrete sampling: `FieldName_SpatialStat`
  (e.g., `Temperature_Mean`)

These fields are written to output streams at the specified frequencies.

#### Output Streams

The GlobalStats group automatically creates output streams based on the
configuration. Multiple streams may be created if multiple reduction periods
or sample frequencies are specified:
- One stream for each temporal reduction period
- One stream for discrete sampling (if SampleFreq is specified)

Each stream groups fields with the same output frequency.

## Usage Notes

### Temporal Reduction Period Constraints

When using temporal reduction with `ReductionPeriod`, the reduction period
must be evenly divisible into the restart interval. This ensures that
temporal averaging states can be properly checkpointed and restored. The
Analysis system validates this constraint during initialization and will
produce an error if violated.

### Field Availability

Analysis fields depend on model state fields being defined and available.
The Analysis system initializes after the model state and validates that
all requested fields exist. If a requested field is not found, an error
is produced during initialization.

### Intermediate Results

Some analysis operators require intermediate computations. For example,
computing the standard deviation requires first computing the mean. The
Analysis system automatically recognizes these dependencies and reuses
shared intermediate results efficiently. Users do not need to explicitly
configure intermediate operators.

## Example Use Cases

### Daily and Monthly Temperature Statistics

```yaml
Omega:
  Analysis:
    GlobalStats:
      Enable: true
      Fields: [Temperature]
      SpatialStats: [Mean, Min, Max]
      ReductionPeriod: [1day, 1month]
      Filename: ocean.temp.stats.$Y-$M
```

This configuration produces six output fields:
- `Temperature_Mean_TimeMean1day`, `Temperature_Min_TimeMean1day`,
  `Temperature_Max_TimeMean1day`
- `Temperature_Mean_TimeMean1month`, `Temperature_Min_TimeMean1month`,
  `Temperature_Max_TimeMean1month`

### Instantaneous Velocity Statistics Every 6 Hours

```yaml
Omega:
  Analysis:
    GlobalStats:
      Enable: true
      Fields: [NormalVelocity]
      SpatialStats: [Mean, StdDev]
      SampleFreq: [6hour]
      Filename: ocean.velocity.stats.$Y-$M-$D
```

This configuration produces two output fields sampled every 6 hours:
- `NormalVelocity_Mean`
- `NormalVelocity_StdDev`

## Future Extensions

The Analysis framework is designed for extensibility. Future versions will
include:
- Additional pre-defined analysis groups (AMOC stream function, eddy
  statistics, etc.)
- User-defined custom analysis groups with fully composable operator chains
- Additional operators for vertical operations, horizontal gradients,
  and custom transformations

For detailed information about the Analysis architecture, operator
composition, and dependency resolution, see the
[Analysis](#omega-dev-analysis) section of the Developer's Guide.
