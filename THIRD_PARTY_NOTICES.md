# Third-party software notices

ZZZ-OS contains third-party software. Those components remain under their
original licenses; the project-level GPL-2.0-only declaration does not replace
their copyright notices or license terms.

## Linux-derived code

Some architecture and driver files retain notices from the Linux kernel,
including files under `arch/arm/` and `drivers/`. These files are distributed
under GPL version 2 as stated in their file headers. Their original copyright
and license notices must be preserved.

License text: [`LICENSE`](LICENSE) (GPL version 2).

## Device Tree Compiler and libfdt

- Location: `tools/dtc/`
- Bundled version marker: `DTC 1.4.0-dirty`
- Upstream: <https://git.kernel.org/pub/scm/utils/dtc/dtc.git/>
- DTC: GPL-2.0-or-later
- libfdt: GPL-2.0-or-later OR BSD-2-Clause

The applicable notices are retained in the source files. The BSD alternative
for libfdt is reproduced in
[`LICENSES/BSD-2-Clause-libfdt.txt`](LICENSES/BSD-2-Clause-libfdt.txt).

## Lua

- Location: `user_proc/lua/`
- Version: 5.5.0
- Upstream: <https://www.lua.org/>
- License: MIT

The Lua copyright notice is retained in `user_proc/lua/src/lua.h`; its license
is reproduced in [`LICENSES/MIT-Lua.txt`](LICENSES/MIT-Lua.txt).

## sbase

- Location: `user_proc/sbase/`
- Upstream: <https://core.suckless.org/sbase/>
- License: MIT

See [`user_proc/sbase/LICENSE`](user_proc/sbase/LICENSE) for the copyright
holders and license text.

## dash

- Distributed file: `user_proc/dash`
- Corresponding upstream revision: `552c2b2`
- Upstream: <https://git.kernel.org/pub/scm/utils/dash/dash.git/>
- Primary license: BSD-3-Clause; see the component's complete copying notice
  for additional build-tool notices.

The copying notice is reproduced in
[`LICENSES/BSD-3-Clause-Dash.txt`](LICENSES/BSD-3-Clause-Dash.txt).

## DoomGeneric

- Distributed file: `user_proc/doom/doomgeneric-zzz`
- Corresponding source revision: `dcb7a8d`
- Source: <https://github.com/ozkl/doomgeneric/tree/dcb7a8d>
- License: GPL-2.0

The engine executable and its game data are separate works. The GPL covering
the DoomGeneric engine does not grant permission to distribute proprietary
Doom WAD data.

## DOOM 1.9 shareware WAD

- Location: `user_proc/doom/doomq.wad`
- Original name: `doom1.wad`
- Size: 4,196,020 bytes
- MD5: `f0cefca49926d00903cf57551d901abe`
- SHA-256: `1d7d43be501e67d927e415e0b8f3e29c3bf33075e859721816f652a526cac771`
- Copyright: © 1994 id Software
- License: proprietary shareware; free redistribution without charge

The hashes identify the bundled file as the unmodified DOOM 1.9 shareware
WAD. It is not open-source software and is not covered by GPL-2.0-only.
Do not modify it or charge for receiving or using it without the copyright
holder's prior permission. See
[`LICENSES/DOOM-Shareware.txt`](LICENSES/DOOM-Shareware.txt).

The deployment script installs the bundled file by default and still accepts
`--doom-wad <path>` or the `ZZZ_DOOM_WAD` environment variable as an
override.

## Maintainer note

When updating a component, record its exact upstream revision here, retain all
copyright notices, and review the new version's license before redistribution.
