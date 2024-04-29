# Description

Chess game PGN parser which would taken the game in PGN format and would show the final state.
Although the program goes a bit beyond and implements few extentions so that it works pretty much like chess.com

# PGN spec

http://www.saremba.de/chessgml/standards/pgn/pgn-complete.htm#c5


# how to build

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j2
```

# how to run

```
cd build
./chess_replay ../basic.pgn
```

# how to run tests

```
cd build
./tests
```

# important notes

- some extended syntax mentioned on Wiki is supported even though not mentioned in the PGN standard
- program does not read headers to resume replay from specific place as the requirnements clearly stated that headers don't impact the state
- Implicit pawn captures moves like this 'ab'(files only) are not supported. On the other hand, "En passant" captures are supported without a need to add an explicit capture signal
