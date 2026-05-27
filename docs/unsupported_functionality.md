# UnsupportedFunctionality

The `README.md` summarizes the `UnsupportedFunctionality` category briefly. This document gives
the long form: the priorities that drive the category, the full taxonomy, the maturity argument,
and the promotion process.

## Why this category exists, and what it trades off

`ttsim` currently implements roughly 70% of the WH/BH architectural feature surface - essentially
every feature exercised by real workloads to date. The remaining ~30% is intentionally left as
`UnsupportedFunctionality`. This is not a backlog. Implementing previously-unsupported features
competes with several first-order engineering priorities:

- **Future-chip support.** Each new chip generation requires substantial simulator work, and that
  work pays off across an entire product line rather than a single feature. A feature request
  whose implementation does not carry forward to future chips is a materially worse candidate
  than one whose implementation is reusable - and this gap grows as a chip approaches end of
  life.
- **Simulator performance.** The current simulator is reasonably fast but well short of what
  real-workload productivity requires. Optimization is a first-order goal - and *every new feature
  implementation makes future optimization harder*. Some features have minor optimization impact;
  others can preclude multithreading without new communication channels or prevent efficient
  mapping to AVX-512 without slow-path carveouts. Adding features is not only opportunity cost
  against optimization work, it can be a permanent constraint on it.
- **Validation of existing functionality.** Coverage gaps in already-implemented features are
  typically a higher-value investment than new feature implementation, because they protect
  functionality already in use.
- **`tt-isa-documentation` completion.** Specification gaps remain. Filling them is essential
  for software portability, customer-facing documentation, and the simulator's own long-term
  correctness.
- **Performance simulator.** A performance-modeling sim for shipping silicon would help software
  optimize everything running on those chips. Its expected value to most software is greater than
  that of any individual `UnsupportedFunctionality` feature.

A growing factor on top of the above: **safety-certification readiness.** As customer
deployments of `ttsim` in safety-critical and regulated industries mature, each implemented
feature must eventually be brought up to certification-grade rigor - not merely "passing tests"
but documented test coverage, traceability to specification clauses, RTL cosimulation cross-
checks, and in some cases custom formal-verification harnesses. Per-feature qualification cost
is typically multiple times the implementation cost, and applies even to features that work
correctly today. Every feature carved off as `UnsupportedFunctionality` is a feature that does
not consume that qualification budget - a factor engineers without safety-critical experience
tend to substantially underestimate.

The cost of "yes" to an unsupported-feature request is measured against displacement of the work
above, not against zero.

## Categories of unsupported features

The `README.md` sketches a few of these; the full taxonomy follows. Most unsupported features
fall into more than one category at once.

1. **Debug, test, and manufacturing functionality.** Debug bus, scan chains, BIST, and similar
   bring-up or factory-test features. The simulator already offers superior alternatives
   (source-level inspection, custom diagnostics, deterministic replay).

2. **Features with known hardware bugs.** Implementing such a feature either reproduces the bug
   (entrenching it as architecturally required, blocking future silicon revisions from fixing it)
   or deviates from silicon (violating bit-exactness). Marking the feature unsupported avoids the
   dilemma.

3. **Features with extensive `UndefinedBehavior` or `UnpredictableValue` surface.** Implementing
   the defined portion requires also implementing accurate detection of every UB/UV case, which
   can multiply implementation cost over the defined portion alone. Rarely justified by usage.

4. **Features without an ISA specification.** No documented contract means reverse engineering
   and a de facto contract future silicon cannot safely change. Writing a spec is part of the cost
   of implementing a previously-unspecified feature - often the largest part.

5. **Features the original architects identify as incorrectly designed.** Some features shipped
   but are known by their designers to have been mis-specified. Implementing in `ttsim` would lock
   software into the incorrect design; marking unsupported encourages software to use alternative
   paths.

6. **Features where the simulator offers superior alternatives.** Debug visibility is the canonical
   example: in silicon, visibility is provided through specific hardware mechanisms; in `ttsim`,
   arbitrary visibility is available by modifying simulator source. Implementing the
   hardware-style mechanism adds cost without enabling any capability not already available
   through better means.

7. **Features where institutional knowledge has been lost.** When the original designer of a
   feature has left and no one currently understands its detailed behavior, implementation
   requires reverse engineering from RTL - expensive, error-prone, and producing an implementation
   no one can authoritatively review.

8. **Features with operational characteristics incompatible with the simulator's contract.**
   Stochastic rounding is representative: relies on random behavior difficult to reproduce
   deterministically, detailed behavior not fully understood, known hardware bugs make the feature
   of questionable practical value. Features whose value depends on non-deterministic or
   non-portable properties are difficult to serve well in `ttsim`.

9. **Features discouraged in favor of a documented modern replacement.** Some features execute
   correctly in silicon but represent patterns the simulator team and the architecture's designers
   prefer software not to use - typically modes retained for backward compatibility but with
   better modern alternatives. Marking such features `UnsupportedFunctionality` gives software a
   chance to revisit the usage at write time rather than discover the cost later through
   performance or portability problems.

10. **Conditions that look more like a software bug than an intentional usage.** A bit mask of
    zero where any sensible mask is nonzero, an index unaligned where alignment is the documented
    convention and the low bits are silently masked off, a parameter value producing a roundabout
    no-op. The silicon executes deterministically, but the most plausible explanation for software
    emitting the pattern is a typo, off-by-one, or misunderstanding. Catching it at the offending
    line is more valuable than letting it propagate silently into a downstream correctness or
    debugging cost; this kind of point-of-occurrence bug detection is a substantial part of
    `ttsim`'s value.

11. **Behaviors that are implementation artifacts future silicon may want to change.** Current
    silicon does something well-defined - a wrap, a mask, an effective no-op - but the behavior is
    an artifact of current implementation rather than a designed contract. Admitting it would
    foreclose changes a future generation may want: addressing newly-added state currently masked
    away, honoring a parameter currently ignored, or trapping/faulting to flag the usage error at
    runtime on silicon. Flagging now keeps the architectural option open under the
    [Simulator Behavior Contract](../README.md#simulator-behavior-contract)'s standard evidence
    bar; an unnecessary check is cheap to relax later, while an admitted pattern is expensive to
    remove.

The unifying property is that the expected value of supporting the feature as written does not
justify the cost given the alternatives available - whether the cost is implementation effort,
masked software bugs, foreclosed silicon evolution, simulator performance, or other.

## The maturity argument and the deprecation signal

`UnsupportedFunctionality` is not just a current-state simulator label - it is a forward-looking
signal about which silicon features warrant continued investment.

If workload patterns shift to exercise a previously-unused feature, the evidence base supports
promoting it; the category is revisitable. But features that remain `UnsupportedFunctionality` for
extended periods without promotion requests accumulate evidence in the opposite direction - they
become documented proof that the feature is genuinely low-value. Over chip generations, that
evidence is input to silicon-design decisions: features used by no workload over a chip's
supported lifetime are candidates for removal from future chips, where the area, power, and
verification cost saved benefits everyone.

This framing should be applied with discipline. The point of the category is not to make
deprecation decisions easier in the abstract; it is to ensure that when those decisions are made,
they rest on documented, accumulated evidence rather than guesswork.

## How to request promotion to `UnimplementedFunctionality`

If software has a use case requiring a feature currently marked `UnsupportedFunctionality`, the
path to evaluation is to file a GitHub issue providing the following information:

1. **Specific workload.** Identify the workload (real benchmark, real optimization target, real
   test case) that requires the feature. Hypothetical needs do not qualify.

2. **Concrete benefit.** Quantify the benefit: performance improvement on a named workload,
   capability previously unavailable, customer requirement. Order-of-magnitude estimates are
   acceptable.

3. **Alternatives evaluated.** Document the alternatives considered and why each is unsatisfactory.
   The bar is to show that the unsupported feature is genuinely the lowest-cost path to the
   outcome, not just the first path considered.

4. **Cost acknowledgment.** Implementation cost is typically 1-4 person-days per small feature
   plus directed test coverage; larger features take significantly longer, especially when
   specification work is also required. The request is asking that this cost be paid, displacing
   other planned work.

5. **Specification status.** Confirm whether an ISA-level specification exists. Where one does
   not, the `ttsim` team will need to write it as part of the implementation. Specification
   proposals from outside the team are not accepted as a substitute: specification work requires
   the same RTL-reading, silicon-probing, and verification capabilities that simulator work
   requires, and specifications produced without those capabilities tend to be inadequate in
   subtle ways that compound over time.

6. **Absence of blocking characteristics.** Confirm the feature does not fall into one of the
   above categories in ways implementation would not resolve. A feature known to be incorrectly
   designed remains a poor candidate even when a use case exists; in such cases the right action
   is usually to fix the use case rather than the simulator.

Meeting the criteria does not guarantee implementation; the criteria are a minimum bar for
evaluation rather than automatic decline. Requests not meeting these criteria are declined with a
reference to this document.

Uniform criteria are protective for both the requester (clear path to evaluation) and the
maintainer (one standard applied to all requests, rather than re-litigating each case).
