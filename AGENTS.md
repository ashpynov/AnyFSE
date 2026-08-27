# Instructions for Agents

## Build

- Treat `.vscode/tasks.json` as the source of truth for all build commands and Visual Studio environment setup.
- Before building, read the current `.vscode/tasks.json`; do not guess or hardcode the Visual Studio installation path, MSBuild command,
  configuration, platform, or targets.
- On Windows, run build tasks through the shell defined by `windows.options.shell.executable`. Prepend the values from
  `windows.options.shell.args` to the selected task command so that the Visual Studio developer environment is activated first.
- Choose the task whose `label` matches the required artifact and configuration. Use its `command` and `args` without changing their meaning.
- Run every task listed in `dependsOn` before the selected task, preserving the dependency order and using each referenced task's own `command` and `args`.
- Run build commands from the repository root unless the selected task explicitly specifies another working directory.
- If the required task or environment setup is missing or cannot be executed, report the exact task label and failure instead of silently
  substituting another build command.

## C++ Style

- Source lines up to 160 characters are acceptable. Do not wrap readable code to satisfy a narrower limit; split lines that exceed 160 characters.
- Move string literals that represent paths, registry keys or values, protocol names, executable names, service names, and similar identifiers
  into the appropriate `Constants` module instead of embedding them at call sites.
- Outside the registry module itself, perform registry access through `Tools::Registry`; do not call the Windows registry APIs directly.
- Respect the short namespace aliases exposed by project modules. For example, use `Registry::` for `Tools::Registry` when the alias from
  `Tools/Registry.hpp` is available.
- Avoid introduce `using namespace` or individual `using` declarations except local single place case. Prefer a short namespace alias such as
  `namespace c = AnyFSE::App::Constants;` and access constants as `c::Name`.
