# Repository Guidelines

## Project Structure & Module Organization

This repository implements the ECHOSAR-Net UAV search-and-rescue simulation using OMNeT++ 6.2 and INET 4.5.4.

- `src/app/`: C++ application modules, NED definitions, headers, and shared UDP port constants.
- `src/messages/`: OMNeT++ message schemas (`.msg`); generated message sources are build artifacts.
- `simulations/`: network topology, `omnetpp.ini`, obstacle data, launcher, and generated results.
- `analysis/`: Python post-processing and plots derived from `.sca` result files.
- `docs/`: scenario and parameter references, including literature traceability.
- `run.sh`: standard entry point for command-line and GUI simulation runs.

Keep protocol behavior in `src/`, experiment configuration in `simulations/`, and interpretation or plotting logic in `analysis/`.

## Build, Test, and Development Commands

Copy `.env.example` to `.env` and adjust the workspace path before running tools. The top-level `Makefile` also expects `INET_ROOT` to point to the local INET installation.

```bash
make makefiles                    # regenerate src/Makefile with opp_makemake
make                              # debug build
make clean                        # remove debug build products
./run.sh --build                  # build, then run Validation_Direct in Cmdenv
./run.sh --gui                    # run interactively in Qtenv
./run.sh -c Validation_BaOn -r 0  # run a specific configuration and seed
python3 analysis/process_results.py
```

Run commands through the configured `opp_env` environment when OMNeT++ tools are not already on `PATH`.

## Coding Style & Naming Conventions

Match the existing C++ style: four-space indentation, braces on the next line for functions, `camelCase` variables and methods, and `PascalCase` module/message types. Keep code inside the `echosar` namespace. Name paired OMNeT++ files consistently, for example `DroneApp.{h,cc,ned}`. Use uppercase names for shared constants such as `TEAM_UPDATE_PORT`.

For Python, follow PEP 8, use four spaces, and prefer `snake_case`. Preserve concise comments that explain simulation intent or metric semantics.

## Testing Guidelines

There is no standalone unit-test framework. Validate changes by rebuilding and running deterministic seeds with Cmdenv. Check the exit status, simulation logs, and generated scalars under `simulations/results/`. For metric changes, run `analysis/process_results.py` and inspect outputs in `analysis/figures/`. Test more than one seed when behavior is stochastic.

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
