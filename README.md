# Lametta

This repository is for the language documentation and poc-transpiler.

For language documentation, check [here](docs/syntax.md).

# For building
* Make sure a recent version of Python3 is installed.
* Use `./buildw.sh`
* The binaries should generate in build/

# For development
The workspace is designed for VSCode with Dev Containers, but you can also just run `./build.sh` for an initial setup/build and have it populate the venv that way.

Sadly VSCode can't detect CMake running from a python venv, so you have to install a compatible version (good luck 🙃).

After VSCode finds CMake (inside the dev container, if you use it, or on the host system), you can import the generated preset from the build directory and dependencies should resolve.

If you want to contribute, please also isntall pre-commit with:
```bash
pip install pre-commit && pre-commit install
```
It should run automagically, but you can run `pre-commit run --all-files` to make sure everything's fine.