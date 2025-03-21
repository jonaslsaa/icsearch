# CLAUDE.md - Development Guidelines for icsearch

## Build/Test Commands
```
# Build the main executable
make

# Build the test suite
make test

# Run all tests
./test

# Lint the code
clang-format -i src/*.c src/*.h

# Check memory leaks
valgrind --leak-check=full ./main <input>
```

## Code Style Guidelines
- **C11 Standard**: Use C11 features, no external dependencies
- **Memory Bounds**: Use fixed-size arrays with explicit capacity limits
- **Naming**: snake_case for functions/variables, uppercase for constants
- **Modules**: Maintain clear separation between IC runtime and search logic
- **Types**: Use appropriate typedefs (enums for node types, structs for data)
- **Error Handling**: Return codes (-1 for errors, 0 for success); avoid assertions in release
- **Documentation**: Docstrings for functions, clear comments for complex logic
- **Testing**: Each module should have isolated unit tests in main_test.c
- **Imports**: Standard headers only, maintain consistent ordering
- **Gas Limit**: Always use a configurable gas limit to prevent infinite loops