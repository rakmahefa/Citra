# Plan — Compatibilité Vulkan (ancien Citra, pilotes récents)

## 0. État initial

- Branche créée : `dev` (depuis `master`, commit `612b0fb`).
- Aucun `git push` effectué ; le dépôt distant reste `origin/master`.
- Copie de travail propre au départ.
- Système d'analyse : Linux x86_64, Mesa 26.2.2, headers Vulkan 1.4.357 (`VK_HEADER_VERSION 357`), loader 1.4.357, GPU attendu Intel Iris Plus (Ice Lake), driver `intel`/`intel_hasvk`.

## 1. Objectif

Rétablir la détection et le démarrage de la chaîne Vulkan sur Citra, sachant que :

- le repo date de 2024 et reste strictement sur l'API Vulkan 1.1 ;
- les pilotes/mesa récents exposent Vulkan 1.3/1.4 et peuvent différer dans les extensions reportées ;
- la chaîne complète doit être inspectée, du chargement de la bibliothèque à la présentation.

Le livrable immédiat est ce `plan.md`. Les corrections code sont volontairement phasées ; aucune modification source n'est imposée par ce document.

## 2. Inventaire de la chaîne Vulkan

Fichiers de configuration / détection (hors `renderer_vulkan`) :

- `CMakeLists.txt` + `src/video_core/CMakeLists.txt` + `src/citra_qt/CMakeLists.txt`
- `externals/cmake-modules/CitraHandleSystemLibs.cmake`
- `src/common/settings.h`, `src/common/settings.cpp`
- `src/video_core/video_core.cpp`
- `src/citra/citra.cpp`, `src/citra/emu_window/emu_window_sdl2_vk.cpp/h`
- `src/citra_qt/main.cpp`
- `src/citra_qt/util/graphics_device_info.cpp/h`
- `src/citra_qt/bootmanager.cpp/h`
- `src/citra_qt/configuration/configure_graphics.cpp`
- `src/citra_qt/configuration/configure_debug.cpp`

Cœur Vulkan :

- `src/video_core/renderer_vulkan/vk_platform.h/cpp` — chargement bibliothèque, création instance, surface WSI.
- `src/video_core/renderer_vulkan/vk_instance.h/cpp` — sélection du physical device, création du logical device, allocator VMA, format tables.
- `src/video_core/renderer_vulkan/renderer_vulkan.cpp/h` — renderer complet.
- `src/video_core/renderer_vulkan/vk_present_window.cpp/h`, `vk_swapchain.cpp/h` — surface/swapchain/présentation.
- `src/video_core/renderer_vulkan/vk_rasterizer*.cpp/h` — rasterizer Vulkan.
- `src/video_core/renderer_vulkan/vk_shader_util.*`, `vk_graphics_pipeline.*`, `vk_renderpass_cache.*`, `vk_pipeline_cache.*` — shaders/pipelines.
- `src/video_core/renderer_vulkan/vk_common.h` — définitions vulkan-hpp, `VK_ENABLE_BETA_EXTENSIONS`, dispatch dynamique.

## 3. Points de blocage identifiés

### 3.1 Détection sans GPU / diagnostics silencieux

- `GetVulkanPhysicalDevices()` (`src/citra_qt/util/graphics_device_info.cpp`) utilise le constructeur headless `Vulkan::Instance instance{};`.
- Si `enumeratePhysicalDevices()` renvoie zéro, ou si une exception est levée, le code capture seulement `std::runtime_error` et renvoie une liste vide.
- L'UI (`src/citra_qt/main.cpp`) affiche un message générique : « GPU peut ne pas supporter Vulkan 1.1, ou pilote non à jour ».
- Conséquence : cause réelle masquée (bibliothèque introuvable, aucune permission `/dev/dri`, échec ICD, driver filtré par version d'API, etc.).

### 3.2 Version d'API stricte à 1.1

- `src/video_core/renderer_vulkan/vk_platform.h` : `constexpr u32 TargetVulkanApiVersion = VK_API_VERSION_1_1;`.
- Cette constante est utilisée :
  1. pour vérifier `available_version < TargetVulkanApiVersion` à la création de l'instance ;
  2. comme `application_info.apiVersion` ;
  3. pour vérifier `properties.apiVersion < TargetVulkanApiVersion` pour le device physique ;
  4. comme `vulkanApiVersion` du VMA allocator.
- Les pilotes récents créent toujours l'instance, mais la vérification stricte empêche d'exposer les fonctionnalités disponibles au-delà de 1.1 et ne distingue pas « absence de support 1.1 » d'une erreur de chargement.

### 3.3 Extensions optionnelles et portabilité

- `vk_instance.cpp::CreateDevice()` demande un nombre élevé d'extensions optionnelles (`VK_EXT_extended_dynamic_state`, `VK_EXT_custom_border_color`, `VK_EXT_fragment_shader_interlock`, `VK_KHR_portability_subset`, etc.).
- `add_extension()` journalise une absence comme warning, mais `VK_KHR_swapchain` reste l'extension « requise » de fait.
- `VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME` est demandé sur toutes plateformes si présent. Sur bureau Linux récent il ne devrait pas être activé sauf MoltenVK : même s'il n'est disponible, l'ajout conditionnel est géré, mais les structures `vk::PhysicalDevicePortabilitySubsetFeaturesKHR/PropertiesKHR` sont tout de même chaînées dans `getFeatures2/getProperties2`. Cela peut être fragile selon loader/ICD.
- Les structures de features 2/3 extended dynamic state sont chaînées dans `getFeatures2`, même si l'extension n'est pas prise en charge. C'est normalement toléré, mais constitue un point de fragilité avec des loaders récents.

### 3.4 Système de fenêtre Linux

- `vk_platform.cpp` définit simultanément `VK_USE_PLATFORM_WAYLAND_KHR` et `VK_USE_PLATFORM_XLIB_KHR`.
- `CreateSurface()` n'active que X11 ou Wayland selon `window_info.type`.
- `EmuWindow_SDL2_VK` (`src/citra/emu_window/emu_window_sdl2_vk.cpp`) crée la fenêtre SDL avec le flag `SDL_WINDOW_OPENGL`, et non `SDL_WINDOW_VULKAN`, ce qui est inadapté pour un chemin Vulkan pur et peut entraîner un handle de surface/runtime incohérent selon le backend SDL.
- Le chemin Qt (`src/citra_qt/bootmanager.cpp`) utilise `QWindow::VulkanSurface`, donc plus standard, mais dépend du support Qt/Vulkan du système.

### 3.5 Compilation / dépendances du code ancien

- `externals/cmake-modules/CitraHandleSystemLibs.cmake` prévoit des vulkan-headers/VMA vendored, mais ils ne sont pas présents dans cet instantané (`externals/vulkan-headers`, `externals/vma`).
- Par défaut `USE_SYSTEM_VULKAN_HEADERS=OFF`, donc la cible `vulkan-headers` pointe vers `./vulkan-headers/include` absent : configure/build impossible sans forcer `-DUSE_SYSTEM_VULKAN_HEADERS=ON`.
- `fmt::join` est utilisé dans `vk_instance.cpp:662` sans inclure `fmt/ranges.h`. Avec fmt récent (12), cette simple unité de compilation échoue avant même le runtime Vulkan.
- Le code cible donc à la fois un environnement ancien et un état de dépôt incomplet.

### 3.6 Présentation / swapchain

- `vk_swapchain.cpp::FindPresentFormat()` accède à `formats[0]` sans vérification de la taille du vecteur retourné par `getSurfaceFormatsKHR`.
- `VkInstance::GetPresentQueue()` renvoie le même queue que graphics ; acceptable pour les GPUs unique-queue, mais limite la compatibilité avec certains drivers multi-queue récents.
- L'absence de gestion explicite des erreurs `OUT_OF_DATE/SUBOPTIMAL` à la création de surface peut être masquée par `UNREACHABLE()`.

## 4. Stratégie recommandée

Ne pas considérer le 1.1 comme « une version cible unique à exiger au runtime », mais :

1. le conserver comme **minimum obligatoire** ;
2. laisser l'instance négocier la version la plus haute supportée par le loader (capée à 1.2/1.3 pour éviter de dépendre d'API non testées) ;
3. ne demander les extensions portabilité/EDS que si disponibles et sur la bonne plateforme ;
4. journaliser précisément la cause d'échec ;
5. découpler la détection (headless) du démarrage du renderer.

## 5. Plan par phases

### Phase 0 — Repro et diagnostics (sans changement source)

- Compiler avec les flags système :
  - `-DENABLE_VULKAN=ON`
  - `-DUSE_SYSTEM_LIBS=ON`
  - `-DUSE_SYSTEM_VULKAN_HEADERS=ON`
  - `-DUSE_SYSTEM_VMA=ON`
- Obtenir les logs Citra (`Log`, niveau Debug) et `vulkaninfo --summary`.
- Capturer l'erreur exacte au démarrage (`client`, `Window type`, `instance version`, `physical device`, extension manquante).
- Critère de sortie : un diagnostic textuel précis associé à la machine.

### Phase 1 — Correction de la détection headless

Fichiers : `src/citra_qt/util/graphics_device_info.cpp`, `src/citra_qt/main.cpp`.

- Étendre les exceptions capturées (`std::exception`, `vk::SystemError`) au lieu de `std::runtime_error` seul.
- Ajouter un journal détaillé : instance version, loader version, nombre de devices, nom, `apiVersion`.
- Éviter les assertions/Critical sur machine sans GPU pour la détection.
- Renvoyer la cause d'échec vers l'UI (texte enrichi, pas seulement message générique).
- Critère : aucune détection silencieuse ; choix correct de fallback OpenGL/Software.

### Phase 2 — Modernisation minimale du chargement de l'instance

Fichiers : `src/video_core/renderer_vulkan/vk_platform.h/cpp`.

- Charger `libvulkan.so.1` puis `libvulkan.so`, et consigner la bibliothèque effectivement chargée.
- Vérifier que `vkGetInstanceProcAddr` est résolu avant `init`.
- Cap de version :
  - garder `MinimumVulkanApiVersion = VK_API_VERSION_1_1` ;
  - demander `instance_api_version = min(loader_version, VK_API_VERSION_1_2)` (ou 1.3 après validation).
- Ne pas traiter un échec `vkEnumeratePhysicalDevices` comme assertion fatale.
- Critère : instance headless créée sur Mesa moderne, au moins un physical device reporté si GPU accessible.

### Phase 3 — Device creation robuste

Fichiers : `src/video_core/renderer_vulkan/vk_instance.h/cpp`.

- Créer une structure `VulkanCapabilities` pour rassembler versions/extensions reportées.
- Porter `portability subset` dans la chaîne uniquement si `VK_KHR_portability_subset` est disponible et sur Apple/MoltenVK ; sinon unlink.
- Porter EDS 1/2/3 uniquement si l'extension correspondante est activée ; éviter les chaînes toujours présentes.
- Rénover les messages d'erreur : « extension requise manquante », « queue graphics absente », « version device < 1.1 ».
- Maintenir les blacklists driver actuelles (Qualcomm, ARM, Nvidia, MoltenVK) et ajouter `intel_hasvk` si tests le requièrent.
- Critère : `CreateDevice()` retourne un `unique_device` valide sans `VK_ERROR_EXTENSION_NOT_PRESENT` sur pilotes récents.

### Phase 4 — Surface/swapchain modernes (Linux)

Fichiers : `src/video_core/renderer_vulkan/vk_platform.cpp`, `vk_present_window.cpp/h`, `vk_swapchain.cpp/h`, `src/citra/emu_window/emu_window_sdl2_vk.cpp`.

- Correctif SDL : utiliser `SDL_WINDOW_VULKAN` (et conserver X11/Wayland).
- Détecter Wayland vs X11 à l'exécution de façon fiable ; journaliser le backend choisi.
- Swapchain : vérifier `surface_formats` non vide, gérer `OUT_OF_DATE` lors de la création.
- Définir une file présentation distincte si disponible (sinon fallback graphics).
- Critère : fenêtre Vulkan s'ouvre et présente un frame sur session X11 et Wayland.

### Phase 5 — Rendu et shaders

Fichiers : `renderer_vulkan.cpp`, pipeline/shader helpers, `vk_rasterizer*`.

- Vérifier que le moteur de shader SPIR-V produit du SPIR-V compatible avec le device Vulkan effectif.
- Conserver la possibilité GLSL fallback pour debug.
- Vérifier les feature flags utilisés (geometry shader, anisotropy, logic op) sont bien négociés et non supposés.
- Critère : un jeu simple démarre sans validation errors.

### Phase 6 — Toolchain et dépendances modernes

Fichiers : `CMakeLists.txt`, `externals/CMakeLists.txt`, `externals/cmake-modules/CitraHandleSystemLibs.cmake`, `vk_instance.cpp`.

- Rendre explicite l'obligation des dépendances Vulkan :
  - `USE_SYSTEM_VULKAN_HEADERS=ON` documenté ;
  - VMA système ou bundled réintégré ;
  - `fmt/ranges.h` inclus pour `fmt::join`.
- Sanitiser l'environnement de build pour ne pas masquer les erreurs Vulkan derrière fmt/Catch2/Cryptopp manquants.
- Critère : configure + build réussis sur toolchain récent sans edits manuels.

### Phase 7 — Validation

- Test headless : instance + enumeration + create device (smoke test).
- Test UI : détection devices, sélection Vulkan.
- Test runtime : boot d'un ROM minimal, présentation, swapchain resize/fullscreen.
- Vérifier les drivers : Intel (hasvk), Mesa (ANV si supporté), Nvidia propriétaire, AMD RADV.
- Capturer logs sans erreurs `UNREACHABLE`, `VK_ERROR_*`.

## 6. Risques et points à surveiller

- Le passage à une version d'API plus haute que 1.1 doit être justifié par les tests réels ; sinon la complexité augmente pour peu de gain.
- L'Intel Ice Lake dépend du driver `intel_hasvk` sur Mesa récente ; si ce driver n'est pas installé, aucun changement code ne suffira.
- Le même exécutable peut rencontrer des ICD différents selon les variables d'environnement (`VK_DRIVER_FILES`, `VK_ICD_FILENAMES`), ce qui peut masquer les vrais problèmes.
- Les extensions bêta/alpha (`VK_EXT_*_dynamic_state*`) sont souvent la cause de crashes sur drivers récents.
- Le dépôt est incomplet pour un build par défaut (headers/vma manquants) ; il faut clarifier build de référence.

## 7. Prochaine étape immédiate recommandée

1. Capturer les logs exacts de la tentative actuelle sur la machine concernée.
2. Appliquer les phases 1 et 2 d'abord (diagnostics + instance robuste), puis vérifier que la détection fonctionne.
3. Ensuite seulement modifier la création du device et la présentation.
4. Garder tout sur la branche `dev`, sans push jusqu'à validation.