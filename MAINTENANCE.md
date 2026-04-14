
## CUDA Simulation Results (2026-04-14)

### Law 27: Instinct Evaluation is Pure Overhead
- ROLE (hardcoded collect) beats INSTINCT 2.23x (normal), 2.79x (scarcity), 1.08x (abundance)
- HYBRID (instinct + role fallback) costs 37% over pure ROLE due to evaluation branching
- instinct-c library should be used as **safety override**, not primary decision path
- Recommended pattern: ROLE primary → INSTINCT override for SURVIVE/FLEE only

### Design Implications
1. Minimize instinct types checked per cycle — only evaluate when energy/threat crosses threshold
2. Use compile-time role assignment, not runtime instinct arbitration
3. HOARD instinct (slow movement) costs 50% collection rate — use sparingly
4. CURIOUS instinct (random exploration) costs collection time — only when idle
5. FLEE instinct (opposite movement) actively counterproductive for food collection

### nvcc Bug
- `kernel<<<grid,blk>>>()` (zero-arg launch) fails: nvcc parses `>>()` as right-shift
- Fix: add dummy parameter or space: `kernel<<<grid,blk>>> ()`
