# Fartex

A butt-themed distortion effect (VST3) built with JUCE 8.

- Click the butt to make it fart.
- Loud hits in the incoming audio also set it off (see TRIGGER).
- The more DRIVE, the angrier the farts.

Controls: DRIVE, TONE, MIX, OUTPUT, FART LVL, TRIGGER.

You do **not** need to install anything on your PC. GitHub builds the plugin for you on its own servers.

---

## How to get the plugin (no installs)

1. **Unzip** this zip. Open the `Fartex` folder inside it.
   You should see `CMakeLists.txt`, `README.md`, a `Source` folder and a `.github` folder.

2. **Make a private GitHub repo.**
   On github.com click **+** (top right) > **New repository**, name it anything (e.g. `fartex`), choose **Private**, then **Create repository**.

3. **Drag all the files in.**
   On the new repo page click **uploading an existing file**. Select everything inside the `Fartex` folder
   (`CMakeLists.txt`, `README.md`, `Source`, **and the `.github` folder**) and drag it all into the browser.
   Wait for the uploads to finish, then click **Commit changes**.
   - Check afterwards that the repo shows `.github/workflows/build.yml`. If `.github` is missing, the build won't start.

4. **Open the Actions tab.**
   Committing starts the build automatically. Click **Actions** at the top of your repo, then click the
   running job called **Build Windows VST3**. It takes roughly 5-10 minutes the first time.
   (If nothing is running: click **Build Windows VST3** on the left, then **Run workflow**.)
   A green check means it worked. A red X means it failed: open it and send the error text to me.

5. **Download the artifact.**
   Scroll to the bottom of the finished run to **Artifacts** and click **Fartex-VST3-Windows**.
   Unzip the download. Inside is a folder called `Fartex.vst3`.

6. **Copy the `.vst3` into your VST3 folder.**
   Copy the whole `Fartex.vst3` folder to:

   `C:\Program Files\Common Files\VST3\`

   Windows will ask for admin permission. Click **Continue**.

7. **Rescan in FL Studio.**
   Open FL Studio > **Options** > **Manage plugins** > click **Find installed plugins** (use "Verify plugins" if asked).
   Fartex then shows up in the plugin list under **Installed > Effects**. Add it to a mixer slot.

---

## Good to know

- Artifacts are kept for 90 days. To rebuild any time, go to Actions > Build Windows VST3 > **Run workflow**.
- Any change you commit to the repo (for example editing a file in `Source`) rebuilds the plugin automatically.
- The plugin is built with the C++ runtime baked in, so no Visual C++ Redistributable is needed.
- Windows may show a SmartScreen/antivirus warning for an unsigned plugin you built yourself. That is normal.

## Building locally (optional, you don't need this)

    cmake -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release

Result: `build\Fartex_artefacts\Release\VST3\Fartex.vst3`
