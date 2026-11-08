# TODO — Implémentation compatibilité Vulkan

## Phase 1 — Détection headless/logs
- [x] Enrichir `GetVulkanPhysicalDevices` : exceptions `std::exception`/unknown, logs détaillés (nb devices, apiVersion device).
- [x] Améliorer le message UI dans `main.cpp` (ICD manquant / /dev/dri / log).
- [ ] Éviter assertions fatales lors de la détection sans GPU (à valider au runtime).

## Phase 2 — Chargement instance / version
- [x] Logger la bibliothèque vulkan (`OpenLibrary` archive déjà les tentatives).
- [x] Minimum 1.1 conservé + version négociée plafonnée à 1.2 (`PreferredVulkanApiVersion`).
- [ ] Gérer plus explicitement l'échec d'`enumeratePhysicalDevices` (pas de crash, à valider).
- [x] Adapter la vérification `properties.apiVersion` via `MinimumVulkanApiVersion`.

## Phase 3 — Création robuste du logical device
- [ ] Chaîner portability subset uniquement si disponible (analyse, à refactor).
- [ ] Chaîner EDS 1/2/3 uniquement si extensions disponibles (analyse, à refactor).
- [ ] Rénover les messages d'erreur device.
- [x] Conserver blacklists drivers (inchangé, confirmé).

## Phase 4 — Surface / swapchain / SDL / WSI Linux
- [x] Utiliser `SDL_WINDOW_VULKAN` au lieu de `SDL_WINDOW_OPENGL`.
- [x] Vérifier `surface_formats` non vide + fallback au premier format.
- [x] Gérer present modes vide (fallback FIFO).
- [ ] Logger le backend WSI choisi.
- [ ] File présentation distincte si disponible.

## Phase 5 — Rendu / shaders / features
- [ ] Vérifier features négociées et fallback GLSL.
- [ ] Tests runtime de base (nécessite GPU/access).

## Phase 6 — Build modernisation
- [x] Inclure `fmt/ranges.h` pour `fmt::join`.
- [x] Moderniser les signatures callbacks debug pour vulkan-hpp récent (`DebugUtils`/`DebugReport`).
- [ ] Documenter `USE_SYSTEM_VULKAN_HEADERS=ON` et `USE_SYSTEM_VMA=ON` (README/build docs).
- [x] Réduire les incompatibilités fmt v12 restantes (formatter enum en const).

## Phase 7 — Validation
- [ ] Smoke test détection (pas exécuté: pas de GPU dans cet environnement).
- [ ] Boot ROM minimal.
- [ ] Tests drivers principaux.