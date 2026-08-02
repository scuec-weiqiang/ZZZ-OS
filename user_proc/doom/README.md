# DoomGeneric test program

`doomgeneric-zzz` is built from DoomGeneric revision `dcb7a8d`:

<https://github.com/ozkl/doomgeneric/tree/dcb7a8d>

The engine is licensed under GPL version 2. See the repository-level
`LICENSE` and `THIRD_PARTY_NOTICES.md`.

The bundled `doomq.wad` is the unmodified DOOM 1.9 shareware WAD. It is
proprietary game data distributed under its own no-charge shareware terms,
not under the engine's GPL. See `LICENSES/DOOM-Shareware.txt`.

You can override it with another legally obtained WAD:

```sh
export ZZZ_DOOM_WAD=/path/to/doom.wad
make install
```

Without an override, the deployment script installs the bundled shareware WAD.
