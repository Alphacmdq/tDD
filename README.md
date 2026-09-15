# The Diaper Decider — M5StickS3

No IDE needed. GitHub compiles, your browser flashes.

## Repo layout (keep it exactly like this)
```
.github/workflows/build.yml      ← used on GitHub
.gitlab-ci.yml                   ← used on GitLab
diaper_decider_stick/diaper_decider_stick.ino
README.md
```
Only one of the two CI files is used, depending on where you host it. The other is ignored.
Arduino insists the `.ino` file and its folder share a name.

## 1. Build
**GitHub:** create a repo, upload these files, push to `main`. Actions tab → "Build firmware" runs by itself (~5–8 min the first time). When it's green, open the run → **Artifacts** → download → unzip → `diaper_decider_stick.merged.bin`.

**GitLab (self-hosted):** same upload. The pipeline runs from `.gitlab-ci.yml` if the instance has a Docker runner with internet access. CI/CD → Pipelines → the job → **Download artifacts** on the right → `out/diaper_decider_stick.merged.bin`. If the pipeline fails on `core install` or `curl`, the runner can't reach the internet — use a personal GitHub repo instead.

Every time you edit the text lists and push, a fresh `.bin` appears.

## 2. Flash (in Chrome or Edge — Web Serial needed)
1. Put the device in download mode: hold the side reset/power button ~2 s until the green LED inside blinks, then release.
2. Plug in USB-C.
3. Open https://espressif.github.io/esptool-js/
4. Baud 460800 → **Connect** → pick the port that just appeared.
5. Add file: `diaper_decider_stick.merged.bin`, flash address **0x0**.
6. **Program**. When it says done, unplug and hold the power button ~2 s to start it.

If the port doesn't show up in the browser, the company laptop is probably blocking USB serial devices — then the flashing step needs another PC (any personal laptop with Chrome; a phone won't do, Web Serial isn't available there).

## 3. Use
- **Button A** (front): decide. ~20 s show, then the name, then the reason.
- **Button B** (side): show the tally for 2.5 s.
- After 90 s idle the device powers off. Hold the power button ~2 s to turn it back on.

## 4. Edit
Everything you'd want to change is in section 1 of the sketch: odds, timings, text lists.
Built-in fonts have no "…" glyph — use `...`.
