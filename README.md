# sfml-chess
A vanilla Chess game created using C++ and SFML.

## Details

- Both players are controlled manually; no chess engine is integrated (yet).
- Pawn promotion results in automatic Queen (for now).
- En passant and castling are implemented.
- Board flipping is available.
- Legal move and check highlighting for easier gameplay.
- Different piece themes to choose from.
- Supports multiple board colors including a dynamic RGB mode.

---

### Gameplay

<p align="center">
  <img src="https://github.com/Attaulhaleem/sfml-chess/blob/main/docs/capture_1.png" width="35%">
&nbsp; &nbsp; &nbsp; &nbsp;
  <img src="https://github.com/Attaulhaleem/sfml-chess/blob/main/docs/capture_2.png" width="35%">
</p>

<p align="center">
  <img src="https://github.com/Attaulhaleem/sfml-chess/blob/main/docs/capture_3.png" width="35%">
&nbsp; &nbsp; &nbsp; &nbsp;
  <img src="https://github.com/Attaulhaleem/sfml-chess/blob/main/docs/capture_4.png" width="35%">
</p>

---

### Controls

- `Q` or `Esc` quits the game.
- `F` flips the board.
- `H` hides the pieces.
- `P` changes the piece theme.
- `B` changes the board to a random color.
- `R` turns on dynamic RGB color mode.
- `N` toggles notation.
- `A` changes notation alignment.
- `T` turns on threats (for debugging).
- `L` turns on labels (for debugging).

---

### Installation

The SFML library can be linked with a Visual Studio Project using [these](https://www.sfml-dev.org/tutorials/2.5/start-vc.php) instructions. After linking SFML, add the source files to your project and run.
