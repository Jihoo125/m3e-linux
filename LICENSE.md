MIT License

Copyright (c) 2026 Jihoo125

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---

## Third-party components

The MIT license above applies to Materialized Linux's own source code.
Third-party components retain their own licenses:

- `third_party/src/material-color-utilities` - Google's Material Color
  Utilities reference sources, Apache License 2.0, retained as the
  verification reference for `libm3`.
- wlroots - MIT license, vendored and pinned (decision D-002).
- The live ISO assembles Debian 13 packages (Linux, systemd, GRUB, Qt,
  PipeWire, NetworkManager, BlueZ, UPower, UDisks2, PackageKit, Firefox ESR,
  Inter, Noto, and others); each package is distributed under its own
  license.