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
./run.sh --build                  # build, then run BasicTest in Cmdenv
./run.sh --gui                    # run interactively in Qtenv
./run.sh -c BasicTest -r 2        # run a specific configuration and seed
python3 analysis/process_results.py
```

Run commands through the configured `opp_env` environment when OMNeT++ tools are not already on `PATH`.

## Coding Style & Naming Conventions

Match the existing C++ style: four-space indentation, braces on the next line for functions, `camelCase` variables and methods, and `PascalCase` module/message types. Keep code inside the `echosar` namespace. Name paired OMNeT++ files consistently, for example `SimpleDroneApp.{h,cc,ned}`. Use uppercase names for shared constants such as `TEAM_UPDATE_PORT`.

For Python, follow PEP 8, use four spaces, and prefer `snake_case`. Preserve concise comments that explain simulation intent or metric semantics.

## Testing Guidelines

There is no standalone unit-test framework. Validate changes by rebuilding and running deterministic seeds with Cmdenv. Check the exit status, simulation logs, and generated scalars under `simulations/results/`. For metric changes, run `analysis/process_results.py` and inspect outputs in `analysis/figures/`. Test more than one seed when behavior is stochastic.

## Commit & Pull Request Guidelines

History follows Conventional Commit prefixes such as `feat:`, `fix:`, `refactor:`, `test:`, `style:`, and `stats:`. Keep subjects imperative, concise, and focused on one change.

Pull requests should explain the behavioral or experimental impact, list configurations and seeds tested, and identify parameter or metric changes. Include plots or Qtenv screenshots when results or topology visuals change, and update `docs/` when assumptions or literature-backed parameters change.
