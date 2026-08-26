# Repository Guidelines

## Project Structure & Module Organization

This repository implements the ECHOSAR-Net UAV search-and-rescue simulation using OMNeT++ 6.2 and INET 4.5.4.

- `src/app/`: C++ application modules, NED definitions, headers, and shared UDP port constants.
- `src/messages/`: OMNeT++ message schemas (`.msg`); generated message sources are build artifacts.
- `simulations/`: network topology, three `.ini` configurations (`omnetpp.ini`
  holds `[General]`, `experiment.ini` the scientific scenario), obstacle data
  and generated results. `simulations/validation/` keeps the smoke-test
  configuration next to the data only those tests use.
- `analysis/`: Python post-processing derived from `.sca` result files.
  `analysis/tables/` and `analysis/figures/` receive the attendance/loss
  workbook and its two PDF figures; both are regenerable and ignored by Git.
  Detailed statistical reporting was removed for now — see Git history.
- `docs/`: normative implementation directive and its concise index. External
  material stays in `docs/references/`.
- `run.sh`: standard entry point for command-line and GUI simulation runs.

Parameter values live only in the configuration files under `simulations/`.
Documentation cites the keys; it must not repeat the values.

Keep protocol behavior in `src/`, experiment configuration in `simulations/`, and interpretation or plotting logic in `analysis/`.

## Build, Test, and Development Commands

Copy `.env.example` to `.env` and adjust the workspace path before running tools. The top-level `Makefile` also expects `INET_ROOT` to point to the local INET installation.

```bash
make makefiles                    # regenerate src/Makefile with opp_makemake
make                              # debug build
make clean                        # remove debug build products
make analysis-tests               # analyser unit tests
make experiment                   # 30 paired runs per arm, then analysis
make robustness-experiment        # separate non-confirmatory matrix
make reproduce                    # build → tests → paired experiment
make alert-sheet                  # attendance/loss workbook and figures from existing results
./run.sh --build -c MainExperiment_BaOn -r 0
./run.sh --gui                    # run interactively in Qtenv
./run.sh -c MainExperiment_BaOff -r 0   # run control seed zero
```

`MainExperimentBase` is declared `abstract = true` and cannot be run directly:
use `MainExperiment_BaOff` or `MainExperiment_BaOn`. The analysis fails
when the two arms differ by anything other than `baEnabled`.

Run commands through the configured `opp_env` environment when OMNeT++ tools are not already on `PATH`.

## Coding Style & Naming Conventions

Match the existing C++ style: four-space indentation, braces on the next line for functions, `camelCase` variables and methods, and `PascalCase` module/message types. Keep code inside the `echosar` namespace. Name paired OMNeT++ files consistently, for example `DroneApp.{h,cc,ned}`. Use uppercase names for shared constants such as `TEAM_UPDATE_PORT`.

For Python, follow PEP 8, use four spaces, and prefer `snake_case`. Preserve concise comments that explain simulation intent or metric semantics.

## Testing Guidelines

The Python analyser has a `unittest` suite. Validate simulation changes by
rebuilding and running the deterministic smoke-test targets with Cmdenv. Check
the exit status, simulation logs, and generated scalars under
`simulations/results/`. For metric changes, run `analysis/reports/alert_sheet.py`. Figures carry no
embedded title: ABNT places the caption above and the source below, both
written in LaTeX. Test more than one seed when behavior is stochastic.

## Commit & Pull Request Guidelines

History follows Conventional Commit prefixes such as `feat:`, `fix:`, `refactor:`, `test:`, `style:`, and `stats:`. Keep subjects imperative, concise, and focused on one change.

Pull requests should explain the behavioral or experimental impact, list configurations and seeds tested, and identify parameter or metric changes. Include plots or Qtenv screenshots when results or topology visuals change, and update `docs/` when assumptions or literature-backed parameters change.

## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

When the user types `/graphify`, use the installed graphify skill or instructions before doing anything else.

Rules:
- For codebase questions, first run `graphify query "<question>"` when graphify-out/graph.json exists. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- Dirty graphify-out/ files are expected after hooks or incremental updates; dirty graph files are not a reason to skip graphify. Only skip graphify if the task is about stale or incorrect graph output, or the user explicitly says not to use it.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
