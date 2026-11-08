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