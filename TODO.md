# TODO List

## New features
- **Command manager**: Every change on a property (from the UI or from the engine) would be sent to a command manager instead of modifying the property directly. We could decide when the command is executed, create an history list, record the commands to execute automated tests, ...
---

## Improvements
- **Add tests for texture slice streaming**: Be able to check if texture virtualization still work using the commit commissar.

---

## Bug fixes
- **Remove GenerateHashes and make a script instead**: On Android Studio, we don't have the garanty (and we shouldn't) to be able to compile desktop applications, GenerateHashes executable is not guaranteed to exist. Using a python (or cmake) script should be better. Note that we can add `find_package(Python3 REQUIRED)` in cmake file.

---