# cemu8
This is a chip8 emulator made in C.

## Building
To build, run
```
make
```

## Executing
To execute
```
./chip8 <rom-path
```

## Next steps
- [x] Add 60Hz clock
- [x] Add graphics
- [x] Add input handling
- [ ] Add joystick support?
- [ ] Windows port?


## Notes
- Tested with [chip8 test suite](https://github.com/Timendus/chip8-test-suite) <3
- Graphics and input with [raylib](https://github.com/raysan5/raylib) <3
- Beware of some usage of black magic with [X macros](https://en.wikipedia.org/wiki/X_macro) <33
- Coroutines with [snorkel](https://github.com/garipew/snorkel)
