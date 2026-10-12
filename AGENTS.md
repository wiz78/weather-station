# Project contribution rules

- Prefer ESPHome YAML for declarative configuration, but avoid putting C++ logic in YAML lambdas. Keep lambdas to short calls or small glue expressions; move reusable or multi-step behavior into C++ source files.
- Keep C++ files organized by responsibility. Avoid unrelated classes in one source file and split files before they become difficult to navigate or maintain.
- Keep declarations in focused headers and implementations in matching source files.
