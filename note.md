# Note — État et commandes de build

## Résumé

Cette branche (`dev`) modernise suffisamment Citra pour compiler et exécuter avec un toolchain récent et des drivers/pilotes récents, sans casser la chaîne Vulkan.

Points clés :

- Branche `dev`, aucun push initial jusqu'à cette PR.
- Vulkan : détection headless plus lisible, version négociée (minimum 1.1, cible 1.2 actuelle), surface/swapchain plus robustes, flag SDL Vulkan.
- Compatibilité C++/API modernes : fmt, vulkan-hpp callbacks signatures, Teakra, fmt::join, FFmpeg, OpenAL, Boost.Asio (io_context), Qt6.
- Build `citra-qt` passe avec les dépendances système disponibles.

## Dépendance Xbyak

La révision actuelle de Xbyak (v7.44) déclenche `Xbyak::Error bad mem size` à l'exécution.
Cette branche utilise la révision suivante pour un fonctionnement stable :

- Xbyak : commit `05bdd062f0678359981431360ad8a53c0192f0fd` (mars 2024)

## Commandes de build (Arch Linux)

Dépendances système attendues : Qt6, Boost, Mesa, vulkan-intel/vulkan-headers, sdl2-compat, glslang, spirv-tools, openal, soundtouch, faad2, ffmpeg, zstd, enet...

Compilation (commande type) :

```bash
cmake .. \
  -DENABLE_VULKAN=ON \
  -DUSE_SYSTEM_QT=ON \
  -DUSE_SYSTEM_BOOST=ON \
  -DUSE_SYSTEM_LIBS=ON \
  -DDISABLE_SYSTEM_CATCH2=ON \
  -DDISABLE_SYSTEM_XBYAK=ON \
  -DDISABLE_SYSTEM_DYNARMIC=ON \
  -DDISABLE_SYSTEM_CRYPTOPP=ON \
  -DDISABLE_SYSTEM_INIH=ON \
  -DDISABLE_SYSTEM_ENET=ON \
  -DDISABLE_SYSTEM_LIBUSB=OFF \
  -DDISABLE_SYSTEM_JSON=ON \
  -DDISABLE_SYSTEM_CPP_HTTPLIB=ON \
  -DDISABLE_SYSTEM_LODEPNG=ON \
  -DUSE_SYSTEM_SDL2=ON \
  -DUSE_SYSTEM_SOUNDTOUCH=ON \
  -DENABLE_CUBEB=OFF \
  -DBUILD_TESTING=OFF \
  -DENABLE_WEB_SERVICE=OFF \
  -DCMAKE_CXX_FLAGS="-I/usr/include/qt6/QtGui/6.11.2/QtGui -Wno-error=deprecated-declarations"

cmake --build . --target citra-qt -j 4
```

Certains externals manquants ont été clonés localement (`fmt`, `vulkan-headers`, `sirit`, `glslang`, `vma`, `enet`, etc.).

## État du runtime

- Vulkan : détection et lancement OK.
- OpenGL : lancement OK.
- Message Qt/GTK `This plugin supports grabbing the mouse only for popup windows` : inoffensif.
## Externals vendorisés (branche feature/screen-layout-fusion)

Le snapshot initial (archive ZIP du master citra-emu de mars 2024) ne contenait
pas le contenu des sous-modules git. Toutes les dépendances manquantes ont été
vendorisées directement dans `externals/` (sources plates, sans sous-modules),
avec des révisions épinglées :

| Dépendance | Source | Révision |
|---|---|---|
| boost | azahar-emu/ext-boost (snapshot élagué : headers + serialization/iostreams) | `6a85c31` |
| catch2 | catchorg/Catch2 | `914aeec` |
| cpp-jwt | arun11299/cpp-jwt | `4a970bc` |
| cryptopp / cryptopp-cmake | weidai11 + abdes | tags `CRYPTOPP_8_9_0` |
| cubeb (+ cmake/sanitizers-cmake) | mozilla/cubeb + arsenm/sanitizers-cmake | `dcd7cdd` / `aab6948` |
| dds-ktx | septag/dds-ktx | `c3ca8fe` |
| discord-rpc | azahar-emu/discord-rpc | `049826c` |
| dynarmic (+ mcl, robin-map, zydis, zycore, oaknut imbriqués) | azahar-emu/dynarmic (frontend A32 conservé) | `a466015` |
| enet | lsalzman/enet | `2662c0d` |
| fmt | fmtlib/fmt | 11.1.4 (copie validée localement) |
| glslang | KhronosGroup/glslang | `fc9889c` |
| libressl | azahar-emu/ext-libressl-portable | `88b8e41` |
| library-headers (FFmpeg 6.1) | headers des paquets libav*-dev Ubuntu 24.04 | — |
| nihstro | neobrain/nihstro | `f4d8659` |
| oaknut | merryhime/oaknut | `6b1d57e` |
| openal-soft | azahar-emu/openal-soft | `e399840` |
| sirit (+ SPIRV-Headers imbriqué) | azahar-emu/sirit + KhronosGroup/SPIRV-Headers | `37d49d2` / `aa6cef1` |
| soundtouch | azahar-emu/soundtouch | `9ef8458` |
| teakra | wwylele/teakra | `3d697a1` |
| vma | GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator | `c788c52` |
| vulkan-headers | KhronosGroup/Vulkan-Headers | `409c16b` |
| xbyak | herumi/xbyak — **pin critique** (v7.44 = bug runtime « bad mem size », cf. section Xbyak) | `05bdd06` (mars 2024) |
| zstd | facebook/zstd | `f8745da` |
| faad2, inih, libusb, lodepng, sdl2 (contenus imbriqués), libadrenotools, libyuv (Android) | divers | pins azahar |

Adaptations sources rendues nécessaires par ces versions :

- `src/audio_core/lle/lle.cpp` : l'API teakra actuelle possède la mémoire DSP
  côté hôte (`UserConfig::dsp_memory`, `GetDspMemory()` renvoyant un pointeur).
  Le tableau `std::array<u8, Memory::DSP_RAM_SIZE>` est désormais membre de
  `DspLle::Impl` et transmis à Teakra ; l'interface publique Citra est inchangée.
- `src/audio_core/openal_sink.cpp` : callbacks OpenAL `noexcept` (openal-soft ≥ 1.24).
- `src/citra_qt/bootmanager.cpp`, `camera/camera_util.cpp`,
  `configuration/configure_camera.cpp` : retour aux API Qt compatibles 6.2→6.12
  (`QImage::mirrored`, `QCheckBox::stateChanged`) au lieu des API exclusives
  Qt ≥ 6.9 (`flipped`, `checkStateChanged`), pour compiler à la fois avec le Qt
  6.6.0 téléchargé (Windows/macOS) et le Qt 6.4 système d'Ubuntu 24.04.
- `externals/CMakeLists.txt` : `ENABLE_OPT=OFF` pour glslang (pas de
  SPIRV-Tools vendorisés, l'optimiseur SPIR-V n'est pas utilisé).
- `CMakeLists.txt` : le contrôle « submodules present » ne s'exécute que si
  `.gitmodules` existe (deps vendorisées).
- `CMakeModules/DownloadExternals.cmake` : sélection `clang_arm64` pour le Qt
  téléchargé sur Apple Silicon.
- `CMakeModules/BundleTarget.cmake` : `--no-compiler-runtime` retiré de
  windeployqt afin que le runtime VC soit inclus dans l'archive portable.

Build de validation local (Debian 13, gcc 14, Qt 6.8.3, Qt/GL via sysroot) :
`citra`, `citra-room` et `citra-qt` compilent et `citra-qt` démarre
(QT_QPA_PLATFORM=offscreen).

## CI GitHub Actions (`.github/workflows/ci.yml`)

- Déclencheurs : push sur n'importe quelle branche, pull request, tag `v*`,
  et déclenchement manuel (workflow_dispatch).
- Job `build` (matrice windows-latest / ubuntu-24.04 / macos-15) :
  - **Windows x64** : MSVC + Ninja, Qt 6.6.0 officiel téléchargé à la
    configuration (aqt), packaging windeployqt en POST_BUILD (in-place),
    archive zip portable avec DLL Qt, runtime VC et plugins.
  - **Linux x64** : Qt 6 système (Ubuntu 24.04), SDL2 vendorisée statique,
    target `bundle` → AppImages `citra-qt`, `citra` et `citra-room` via
    linuxdeploy (+ plugin qt/checkrt).
  - **macOS Apple Silicon** : Qt 6.6.0 clang_arm64 téléchargé, renderer OpenGL,
    packaging macdeployqt + codesign ad-hoc, zip du .app.
- Artefacts : upload sur chaque push/PR (`citra-windows-x64`,
  `citra-linux-x64`, `citra-macos-arm64`).
- Job `release` : sur push d'un tag `v*`, télécharge tous les artefacts et
  crée une GitHub Release (notes générées + description du layout Fusion).
  Exemple : `git tag v1.0.0-fusion && git push origin v1.0.0-fusion`.
- Options de build : `-DENABLE_TESTS=OFF -DCITRA_WARNINGS_AS_ERRORS=OFF
  -DENABLE_LTO=OFF` ; macOS ajoute `-DENABLE_OPENGL=ON -DENABLE_VULKAN=OFF`.
- Non couvert pour l'instant : APK Android (les deps Android — libyuv,
  libadrenotools — sont vendorisées et prêtes), Windows ARM64.

